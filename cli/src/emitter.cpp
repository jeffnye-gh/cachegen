// --------------------------------------------------------------------
// FILE:    emitter.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "emitter.h"
#include "diag_codes.h"
#include "msg.h"
#include "rtl_agent.h"
#include "rtl_build.h"
#include "rtl_cache.h"
#include "rtl_mem.h"
#include "rtl_pkg.h"
#include "rtl_system.h"
#include "rtl_tb.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using nlohmann::json;

namespace cgen
{

namespace {

// the file name part of a path, so no absolute path reaches an
// emitted file. R-11.
std::string base_of(const std::string &p)
{
  const size_t k = p.find_last_of("/\\");
  return k == std::string::npos ? p : p.substr(k + 1);
}

} // namespace

// --------------------------------------------------------------------
std::string Emitter::camel(const std::string &n)
{
  std::string o;
  bool up = true;
  for(char c : n) {
    if(c == '_') { up = true; continue; }
    o += up ? char(toupper(c)) : c;
    up = false;
  }
  return o;
}

// --------------------------------------------------------------------
SvFile Emitter::file(const std::string &name, SvFile::Kind k) const
{
  SvFile f(name, sys_, src_, k);
  if(k == SvFile::Kind::Sv && !pfx_snake_.empty()) {
    f.set_prefix(pfx_snake_, pfx_camel_);
  }
  return f;
}

// --------------------------------------------------------------------
bool Emitter::mkpath(const std::string &rel)
{
  std::error_code ec;
  fs::create_directories(fs::path(out_) / rel, ec);
  if(ec) {
    diags_.error(out_, rel, code::emit_mkdir,
                 "cannot create the output directory " +
                 msg->tq((fs::path(out_) / rel).generic_string()) +
                 ", " + ec.message());
    return false;
  }
  return true;
}

// --------------------------------------------------------------------
bool Emitter::put(const std::string &rel, const SvFile &f)
{
  const fs::path p = fs::path(out_) / rel;

  std::ofstream os(p.generic_string(), std::ios::binary);
  if(!os) {
    diags_.error(out_, rel, code::emit_write,
                 "cannot open " + msg->tq(p.generic_string()) +
                 " for writing");
    return false;
  }
  const std::string body = f.content();
  os.write(body.data(), std::streamsize(body.size()));
  os.close();

  written_.push_back(rel);
  return true;
}

// --------------------------------------------------------------------
// Gather the link definitions once, then build one context per node.
// --------------------------------------------------------------------
bool Emitter::build_nodes(const Model &m, const Loader &loader)
{
  // Each link definition keeps the file and the pointer it sits at,
  // so a stage reading one of its fields can record the read against
  // the place it came from. R-6b.
  for(const Loader::File &f : loader.files()) {
    if(!f.doc.contains("links") || !f.doc["links"].is_object()) continue;
    for(auto it = f.doc["links"].begin();
        it != f.doc["links"].end(); ++it) {
      LinkRef r;
      r.body = &it.value();
      r.file = f.disp;
      r.ptr  = "/links/" + it.key();
      links_[it.key()] = r;
    }
  }

  bool ok = true;
  for(const Model::Node &n : m.nodes) {
    if(!n.resolved) {
      skipped_.push_back({ n.name,
                           "the node did not resolve, see the "
                           "diagnostics" });
      continue;
    }
    NodeCtx c;
    std::string why;
    if(!c.build(m, n, n.body, links_, why)) {
      diags_.error(n.file, n.path, code::emit_unsupported,
                   "node " + msg->tq(n.name) +
                   " cannot be emitted, " + why);
      skipped_.push_back({ n.name, why });
      ok = false;
      continue;
    }

    // ----------------------------------------------------------------
    // T-11. THE DECLARED TIMING HAS TO BE BUILDABLE. The check is on
    // the nodes whose control is a pipeline, because those are the
    // ones read_latency_cycles and tag_compare_stage shape. A
    // blocking control does not read either field and the emitter
    // does not invent a range for it.
    // ----------------------------------------------------------------
    if(c.pipelined()) {
      std::string field;
      const std::string bad = c.timing_why(field);
      if(!bad.empty()) {
        diags_.error(n.file, n.path + "/timing/" + field,
                     field == "tag_compare_stage"
                       ? code::t11_tag_stage
                       : code::t11_read_latency,
                     "node " + msg->tq(n.name) + ", " + bad);
        skipped_.push_back({ n.name, bad });
        ok = false;
        continue;
      }
    }

    nodes_[n.name] = c;
  }
  return ok;
}

// --------------------------------------------------------------------
// The packages every node shares: the TileLink constants and one per
// link definition.
// --------------------------------------------------------------------
void Emitter::emit_shared(const Model &m)
{
  const std::string dir = sys_ + "/rtl";
  if(!mkpath(dir)) return;

  bool any_tl = false;
  for(auto &kv : links_) {
    if(kv.second.body != nullptr &&
       kv.second.body->value("protocol", std::string()) == "tilelink") {
      any_tl = true;
    }
  }

  if(any_tl) {
    SvFile f = file(RtlPkg::tl_pkg_name() + ".sv");
    RtlPkg::tilelink(f);
    if(put(dir + "/" + RtlPkg::tl_pkg_name() + ".sv", f)) {
      shared_.push_back(RtlPkg::tl_pkg_name() + ".sv");
    }
  }

  // one package per link, in name order so two runs agree, R-11
  for(auto &kv : links_) {
    LinkSig s;
    std::string why;
    if(kv.second.body == nullptr) continue;
    if(!s.build(*kv.second.body, why, kv.second)) continue;
    const std::string nm = RtlPkg::link_pkg(kv.first) + ".sv";
    SvFile f = file(nm);
    RtlPkg::link(f, kv.first, s);
    if(put(dir + "/" + nm, f)) shared_.push_back(nm);
  }

  (void)m;
}

// --------------------------------------------------------------------
// One node: its package, its modules, its testbench, its build.
// --------------------------------------------------------------------
void Emitter::emit_node(const NodeCtx &c)
{
  const std::string rtl = c.name() + "/rtl";
  const std::string tb  = c.name() + "/tb";

  // every package member this node declares carries its name, so
  // four node packages can share one compilation scope
  pfx_snake_ = c.name() + "_";
  pfx_camel_ = camel(c.name());
  if(!mkpath(rtl)) return;
  if(!mkpath(tb))  return;

  std::vector<std::string> rtl_files;
  std::vector<std::string> tb_files;

  auto emit_rtl = [&](const std::string &nm, SvFile &f) {
    if(put(rtl + "/" + nm, f)) rtl_files.push_back("rtl/" + nm);
  };
  auto emit_tb = [&](const std::string &nm, SvFile &f) {
    if(put(tb + "/" + nm, f)) tb_files.push_back("tb/" + nm);
  };

  // ------------------------------------------------------------------
  // the package
  // ------------------------------------------------------------------
  {
    SvFile f = file(c.pkg() + ".sv");
    RtlPkg::node(f, c);
    emit_rtl(c.pkg() + ".sv", f);
  }

  if(c.is_agent()) {
    SvFile f = file(c.mod() + ".sv");
    RtlAgent::top(f, c);
    emit_rtl(c.mod() + ".sv", f);
  } else if(c.is_memory()) {
    for(const NodeCtx::Iface &i : c.ifaces()) {
      SvFile f = file(c.mod((i.name + "_slv").c_str()) + ".sv");
      RtlMem::slave(f, c, i);
      emit_rtl(c.mod((i.name + "_slv").c_str()) + ".sv", f);
    }
    SvFile f = file(c.mod() + ".sv");
    RtlMem::top(f, c);
    emit_rtl(c.mod() + ".sv", f);
  } else {
    for(const NodeCtx::Iface &i : c.ifaces()) {
      if(i.master) {
        SvFile f = file(c.mod((i.name + "_mst").c_str()) + ".sv");
        RtlCache::master(f, c, i);
        emit_rtl(c.mod((i.name + "_mst").c_str()) + ".sv", f);
      } else {
        SvFile f = file(c.mod((i.name + "_slv").c_str()) + ".sv");
        RtlCache::slave(f, c, i);
        emit_rtl(c.mod((i.name + "_slv").c_str()) + ".sv", f);
      }
    }
    { SvFile f = file(c.mod("meta_array") + ".sv");
      RtlCache::meta_array(f, c);
      emit_rtl(c.mod("meta_array") + ".sv", f); }
    { SvFile f = file(c.mod("data_array") + ".sv");
      RtlCache::data_array(f, c);
      emit_rtl(c.mod("data_array") + ".sv", f); }
    { SvFile f = file(c.mod("ctrl") + ".sv");
      RtlCache::ctrl(f, c);
      emit_rtl(c.mod("ctrl") + ".sv", f); }
    { SvFile f = file(c.mod("bank") + ".sv");
      RtlCache::bank(f, c);
      emit_rtl(c.mod("bank") + ".sv", f); }
    if(c.nonblocking() && c.slaves().size() == 1) {
      SvFile f = file(c.mod("mshr") + ".sv");
      RtlCache::mshr(f, c);
      emit_rtl(c.mod("mshr") + ".sv", f);
    }
    { SvFile f = file(c.mod() + ".sv");
      RtlCache::top(f, c);
      emit_rtl(c.mod() + ".sv", f); }
  }

  // ------------------------------------------------------------------
  // the unit testbench. An agent drives rather than answers, so its
  // testbench is the other way round: a responder on its link and
  // the command port worked from the test.
  // ------------------------------------------------------------------
  if(c.is_agent() && !c.masters().empty()) {
    const NodeCtx::Iface &i = *c.masters().front();
    { SvFile f = file(c.mod("tb_slv") + ".sv");
      RtlTb::tb_slv(f, c, i);
      emit_tb(c.mod("tb_slv") + ".sv", f); }
    { SvFile f = file(c.mod("tests") + ".svh");
      RtlTb::agent_tests(f, c);
      put(tb + "/" + c.mod("tests") + ".svh", f); }
    { SvFile f = file(c.mod("tb") + ".sv");
      RtlTb::agent_tb(f, c);
      emit_tb(c.mod("tb") + ".sv", f); }
  } else if(!c.slaves().empty()) {
    for(const NodeCtx::Iface *ip : c.masters()) {
      if(!ip->sig.is_tl()) continue;
      SvFile f = file(c.mod("tb_mem") + ".sv");
      RtlTb::tb_mem(f, c, *ip);
      emit_tb(c.mod("tb_mem") + ".sv", f);
    }
    { SvFile f = file(c.mod("tests") + ".svh");
      RtlTb::unit_tests(f, c);
      if(put(tb + "/" + c.mod("tests") + ".svh", f)) {}
    }
    { SvFile f = file(c.mod("tb") + ".sv");
      RtlTb::unit_tb(f, c);
      emit_tb(c.mod("tb") + ".sv", f); }
  }

  // ------------------------------------------------------------------
  // the shared task set, one copy beside every testbench that
  // includes it, because an include path is a build flag and this
  // keeps the node directory buildable on its own
  // ------------------------------------------------------------------
  {
    SvFile f = file("cgen_tb_tasks.svh");
    RtlTb::tasks(f);
    put(tb + "/cgen_tb_tasks.svh", f);
  }

  // ------------------------------------------------------------------
  // the build
  // ------------------------------------------------------------------
  std::vector<std::string> sh;
  for(const std::string &s : shared_) {
    sh.push_back("../" + sys_ + "/rtl/" + s);
  }
  {
    SvFile f = file(c.name() + ".f", SvFile::Kind::Flist);
    RtlBuild::node_flist(f, c, sh, rtl_files, tb_files);
    put(c.name() + "/" + c.name() + ".f", f);
  }
  {
    SvFile f = file("Makefile", SvFile::Kind::Make);
    RtlBuild::node_make(f, c);
    put(c.name() + "/Makefile", f);
  }

  pfx_snake_.clear();
  pfx_camel_.clear();
}

// --------------------------------------------------------------------
// R-3. Vars.mk at the root of the output tree. Every emitted Makefile
// includes it, so no build resolves a tool from PATH.
//
// This is the ONE emitted file whose contents may vary with the
// command line. R-11 stands for every other one.
// --------------------------------------------------------------------
bool Emitter::emit_vars()
{
  const std::string src = vars_path_.empty()
                          ? ToolVars::default_source()
                          : vars_path_;

  std::string why;
  if(!tools_.load(src, why)) {
    diags_.error(src, "", code::emit_vars,
                 "the emitted build includes " +
                 std::string(ToolVars::file_name()) + " and " + why +
                 ". Give the path with --vars, or set CGEN_ROOT so "
                 "the default " +
                 std::string(ToolVars::relative_source()) +
                 " resolves");
    return false;
  }

  const std::string root = ToolVars::cgen_root();
  for(const std::string &t : tool_args_) {
    std::string w;
    if(tools_.set(t, root, w)) continue;
    diags_.error(src, "", code::emit_vars,
                 "--tool " + msg->tq(t) + " is not usable, " + w);
    return false;
  }

  SvFile f = file(ToolVars::file_name(), SvFile::Kind::Make);
  tools_.emit(f);
  return put(ToolVars::file_name(), f);
}

// --------------------------------------------------------------------
// R-6. The logs, written last so the emission log can name every file
// and the unconsumed report can see every read the emitters made.
// --------------------------------------------------------------------
void Emitter::emit_logs(const Model &m, const Loader &loader)
{
  const std::string dir = GenLog::dir();
  if(!mkpath(dir)) return;

  // R-8. The table is built AFTER emission, so every test that was
  // emitted has already registered the field it exercises.
  static const FieldUse empty;
  feats_.build(m, use_ ? *use_ : empty, loader.files());

  {
    SvFile f = file(GenLog::features_name(), SvFile::Kind::Make);
    GenLog::features(f, feats_);
    put(dir + "/" + GenLog::features_name(), f);
  }

  {
    SvFile f = file(GenLog::geometry_name(), SvFile::Kind::Make);
    GenLog::geometry(f, m);
    put(dir + "/" + GenLog::geometry_name(), f);
  }
  {
    SvFile f = file(GenLog::unconsumed_name(), SvFile::Kind::Make);
    GenLog::unconsumed(f, m, use_ ? *use_ : empty);
    put(dir + "/" + GenLog::unconsumed_name(), f);
  }

  // the emission log names itself, so its own path joins the list
  // before it is written rather than after
  {
    std::vector<std::string> all = written_;
    all.push_back(dir + "/" + GenLog::emission_name());
    std::sort(all.begin(), all.end());

    SvFile f = file(GenLog::emission_name(), SvFile::Kind::Make);
    GenLog::emission(f, m, nodes_, all, skipped_, tools_);
    put(dir + "/" + GenLog::emission_name(), f);
  }
}

// --------------------------------------------------------------------
void Emitter::emit_system(const Model &m)
{
  const std::string dir = sys_ + "/rtl";
  const std::string tbd = sys_ + "/tb";
  if(!mkpath(tbd)) return;

  {
    SvFile f = file(sys_ + "_top.sv");
    RtlSystem::top(f, m, nodes_, sys_);
    put(dir + "/" + sys_ + "_top.sv", f);
  }
  {
    SvFile f = file(sys_ + "_tests.svh");
    RtlTb::sys_tests(f, m, nodes_, sys_);
    put(tbd + "/" + sys_ + "_tests.svh", f);
  }
  {
    SvFile f = file(sys_ + "_tb.sv");
    RtlTb::sys_tb(f, m, nodes_, sys_);
    put(tbd + "/" + sys_ + "_tb.sv", f);
  }
  {
    SvFile f = file("cgen_tb_tasks.svh");
    RtlTb::tasks(f);
    put(tbd + "/cgen_tb_tasks.svh", f);
  }
}

// --------------------------------------------------------------------
void Emitter::emit_build(const Model &m)
{
  std::vector<std::string> files;
  std::vector<std::string> names;

  for(const std::string &s : shared_) files.push_back("rtl/" + s);

  // node order follows the model, which follows the topology, so two
  // runs of one configuration produce the same filelist. R-11.
  for(const Model::Node &n : m.nodes) {
    auto it = nodes_.find(n.name);
    if(it == nodes_.end()) continue;
    names.push_back(n.name);

    const std::string pre = "../" + n.name + "/";
    for(const std::string &w : written_) {
      const std::string want = n.name + "/rtl/";
      if(w.compare(0, want.size(), want) != 0) continue;
      files.push_back(pre + w.substr(n.name.size() + 1));
    }
  }

  files.push_back("rtl/" + sys_ + "_top.sv");
  files.push_back("tb/" + sys_ + "_tb.sv");

  {
    SvFile f = file(sys_ + ".f", SvFile::Kind::Flist);
    RtlBuild::sys_flist(f, sys_, files);
    put(sys_ + "/" + sys_ + ".f", f);
  }
  {
    SvFile f = file("Makefile", SvFile::Kind::Make);
    RtlBuild::sys_make(f, sys_, names);
    put(sys_ + "/Makefile", f);
  }
}

// --------------------------------------------------------------------
bool Emitter::run(const Model &m, const Loader &loader)
{
  // ------------------------------------------------------------------
  // R-3. An error anywhere means nothing is written. A warning does
  // not stop emission.
  // ------------------------------------------------------------------
  const std::string root = loader.files().empty()
                           ? std::string("<no configuration>")
                           : loader.files().front().disp;

  if(diags_.has_error()) {
    diags_.error(root, "", code::emit_refused,
                 "the configuration produced " +
                 std::to_string(diags_.error_count()) +
                 " error diagnostic" +
                 (diags_.error_count() == 1 ? "" : "s") +
                 ", nothing was emitted");
    return false;
  }

  sys_ = m.system_name.empty() ? std::string("system") : m.system_name;
  src_ = base_of(root);

  if(!build_nodes(m, loader)) return false;
  if(!mkpath("")) return false;

  // R-3. Vars.mk first: every Makefile below includes it, and a tree
  // whose Makefiles include a file that was never written is worse
  // than a tree that was never written.
  if(!emit_vars()) return false;

  // R-8. Installed for the whole of emission, so every test emitter
  // below can register the configuration field its check exercises.
  Features::Scope cov(feats_);

  emit_shared(m);
  for(const Model::Node &n : m.nodes) {
    auto it = nodes_.find(n.name);
    if(it != nodes_.end()) emit_node(it->second);
  }
  emit_system(m);
  emit_build(m);

  std::sort(written_.begin(), written_.end());

  // R-6. Last, so the unconsumed report sees every read the emitters
  // made and the emission log can name every file.
  emit_logs(m, loader);

  std::sort(written_.begin(), written_.end());
  return !diags_.has_error();
}

} // namespace cgen
