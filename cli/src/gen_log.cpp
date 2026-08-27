// --------------------------------------------------------------------
// FILE:    gen_log.cpp
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "gen_log.h"
#include <algorithm>
#include <cstdio>

namespace cgen
{

namespace {

std::string i2s(int v)      { return std::to_string(v); }
std::string u64(uint64_t v) { return std::to_string(v); }

// ------------------------------------------------------------------
// The base name of a configuration file. A log names a file this way
// and never by a path: a path is relative to the working directory,
// and two runs of one configuration from two directories would then
// write two different logs. See the header.
// ------------------------------------------------------------------
std::string base_of(const std::string &p)
{
  const size_t k = p.find_last_of("/\\");
  return k == std::string::npos ? p : p.substr(k + 1);
}

std::string pad(const std::string &s, size_t w)
{
  std::string o = s;
  while(o.size() < w) o += ' ';
  return o;
}

std::string hex(uint64_t v, int bits)
{
  int digits = (bits + 3) / 4;
  if(digits < 1)  digits = 1;
  if(digits > 16) digits = 16;
  char buf[32];
  std::snprintf(buf, sizeof(buf), "0x%0*llx", digits,
                (unsigned long long)v);
  return std::string(buf);
}

std::string field(const char *name, const Model::Field &f, int bits)
{
  std::string s = pad(name, 8);
  if(!f.valid) return s + "not applicable";
  return s + "[" + i2s(f.msb) + ":" + i2s(f.lsb) + "]  bits " +
         pad(i2s(f.bits), 3) + " shift " + pad(i2s(f.shift), 3) +
         " mask " + hex(f.mask, bits);
}

// ------------------------------------------------------------------
// The topology instances that use one cache definition. A leaf under
// /caches/<def> belongs to every node that names <def>, which is more
// than one as soon as a definition is instantiated twice.
// ------------------------------------------------------------------
std::string nodes_using(const Model &m, const std::string &cache_def)
{
  std::string s;
  for(const Model::Node &n : m.nodes) {
    if(n.cache != cache_def) continue;
    if(!s.empty()) s += ",";
    s += n.name;
  }
  return s;
}

// ------------------------------------------------------------------
// The node a JSON pointer belongs to, empty when it belongs to no
// node. /caches/l2/inclusion -> the instances of cache 'l2'.
// ------------------------------------------------------------------
std::string node_of(const Model &m, const std::string &ptr)
{
  const std::string want = "/caches/";
  if(ptr.compare(0, want.size(), want) != 0) return "";

  const size_t beg = want.size();
  const size_t end = ptr.find('/', beg);
  const std::string def =
      end == std::string::npos ? ptr.substr(beg)
                               : ptr.substr(beg, end - beg);
  return nodes_using(m, def);
}

} // namespace

// --------------------------------------------------------------------
void GenLog::emission(SvFile &f, const Model &m,
                      const std::map<std::string, NodeCtx> &nodes,
                      const std::vector<std::string> &written,
                      const std::vector<Skipped> &skipped,
                      const ToolVars &tv)
{
  f.note("R-6a. WHAT THIS RUN EMITTED.");
  f.bar();
  f.note("system " + (m.system_name.empty() ? std::string("<unnamed>")
                                            : m.system_name));
  f.note("nodes  " + i2s(int(m.nodes.size())) + " in the topology, " +
         i2s(int(nodes.size())) + " emitted, " +
         i2s(int(skipped.size())) + " skipped");
  f.note("edges  " + i2s(int(m.edges.size())));
  f.note("files  " + i2s(int(written.size())) + ", counting this one");
  f.ln();

  f.bar();
  f.note("NODES EMITTED. The directory is the topology INSTANCE name,");
  f.note("R-4, so two instances of one definition cannot collide.");
  f.bar();
  for(const Model::Node &n : m.nodes) {
    auto it = nodes.find(n.name);
    if(it == nodes.end()) continue;

    int count = 0;
    for(const std::string &w : written) {
      if(w.compare(0, n.name.size() + 1, n.name + "/") == 0) ++count;
    }
    f.note("  " + pad(n.name, 10) + pad(n.node_type, 14) +
           "cache " + pad(n.cache, 10) + i2s(count) + " files");
  }
  f.ln();

  f.bar();
  f.note("NODES SKIPPED, and why.");
  f.bar();
  if(skipped.empty()) {
    f.note("  none. Every node in the topology produced output.");
  } else {
    for(const Skipped &s : skipped) {
      f.note("  " + s.node);
      f.note("    " + s.why);
    }
  }
  f.ln();

  f.bar();
  f.note("EVERY FILE WRITTEN, relative to the output directory.");
  f.note("The path is relative because an absolute one would differ");
  f.note("between two runs given two spellings of --output, R-11.");
  f.bar();
  for(const std::string &w : written) f.note("  " + w);
  f.ln();

  f.bar();
  f.note("THE TOOL VARIABLE SET, R-3. THIS BLOCK VARIES WITH THE");
  f.note("COMMAND LINE and is the only part of any log that does.");
  f.note("It records what the emitted build was told to run, which");
  f.note("is command line input, exactly as Vars.mk itself is.");
  f.bar();
  f.note("  master copy   " + tv.source_base());
  f.note("  written to    " + std::string(ToolVars::file_name()) +
         " at the output root");
  f.note("  included by   every emitted Makefile, as " +
         std::string(ToolVars::include_line()));
  f.ln();
  for(const ToolVars::Tool &t : ToolVars::all()) {
    f.note("  " + pad(t.var, 12) + tv.value_of(t));
    f.note("               " + std::string(t.what));
  }
  f.ln();
  if(tv.machine_specific()) {
    f.note("  A tool path outside CGEN_ROOT was given on the command");
    f.note("  line and is written verbatim. THIS OUTPUT TREE IS");
    f.note("  MACHINE SPECIFIC, by the user's choice.");
  } else {
    f.note("  Every tool path is inside CGEN_ROOT and is written in");
    f.note("  the $(CGEN_ROOT)/... form, so the tree is portable.");
  }
}

// --------------------------------------------------------------------
// R-6b. THE UNCONSUMED FIELD REPORT.
// --------------------------------------------------------------------
void GenLog::unconsumed(SvFile &f, const Model &m, const FieldUse &u)
{
  const std::vector<FieldUse::Leaf> gone = u.unread();

  f.note("R-6b. THE UNCONSUMED FIELD REPORT.");
  f.bar();
  f.note("Every field this configuration carries that NO STAGE READ.");
  f.note("");
  f.note("THIS IS NOT AN ERROR. A field on this list is accepted,");
  f.note("carried, and reaches nothing: no derived value, no");
  f.note("diagnostic, and no emitted text. Edit it, emit again, and");
  f.note("the output is byte for byte what it was. Without this list");
  f.note("a field that does nothing and a field that works are");
  f.note("indistinguishable from outside the tool.");
  f.note("");
  f.note("THE LIST IS DERIVED, NOT MAINTAINED. Every leaf of every");
  f.note("loaded document is enumerated once, and every stage records");
  f.note("the field at the point it takes the value. What is left is");
  f.note("this. A stage that starts consuming a field drops it from");
  f.note("the list in the same change, and nothing has to be");
  f.note("remembered.");
  f.note("");
  f.note("PRESENCE IS NOT A READ. The T-6 group completeness check");
  f.note("asks whether a field is there and never looks at its value,");
  f.note("so it does not count. A group whose members are all inert");
  f.note("is exactly what this list is for.");
  f.bar();
  f.note(i2s(int(gone.size())) + " unconsumed of " +
         i2s(int(u.leaves().size())) + " fields");
  f.ln();

  if(gone.empty()) {
    f.note("Every field the configuration carries is read by some");
    f.note("stage.");
    return;
  }

  // grouped by the file and the node the field belongs to, so the
  // JSON pointer of every entry fits on its own line at 80 columns
  std::string cur;
  for(const FieldUse::Leaf &l : gone) {
    std::string node = node_of(m, l.ptr);
    const std::string head =
        base_of(l.file) +
        (node.empty() ? std::string(", no node")
                      : ", node " + node);
    if(head != cur) {
      if(!cur.empty()) f.ln();
      f.note(head);
      cur = head;
    }
    f.note("  " + l.ptr);
  }
}

// --------------------------------------------------------------------
// R-8. THE FEATURE TABLE.
// --------------------------------------------------------------------
void GenLog::features(SvFile &f, const Features &ft)
{
  f.note("R-8. THE FEATURE TABLE.");
  f.bar();
  f.note("Every feature the configuration declares, node by node and");
  f.note("field by field, with the test that covers it or the reason");
  f.note("none can.");
  f.note("");
  f.note("ALL THREE COLUMNS ARE DERIVED. The features are the leaves");
  f.note("of the configuration. The tests are registered by the");
  f.note("emitter at the point it emits each check, so a test that");
  f.note("stops being emitted stops claiming coverage. The reasons");
  f.note("come from R-6b: A FEATURE NO STAGE CONSUMES CANNOT BE");
  f.note("TESTED, because nothing in the emitted design moves when");
  f.note("it changes.");
  f.note("");
  f.note("A test marked top is in the top level testbench, where");
  f.note("behaviour visible only when nodes interact lives. One");
  f.note("marked unit is in that node's own testbench.");
  f.bar();
  f.note(i2s(ft.tested()) + " covered, " + i2s(ft.untested()) +
         " not, of " + i2s(int(ft.all().size())) + " features");
  f.ln();

  std::string cur;
  for(const Features::Feature &x : ft.all()) {
    if(x.node != cur) {
      if(!cur.empty()) f.ln();
      f.bar();
      f.note(x.node);
      f.bar();
      cur = x.node;
    }

    f.note("  " + x.ptr);
    f.note("    declared  " + (x.value.empty() ? std::string("-")
                                               : x.value));
    if(x.covers.empty()) {
      f.note("    NO TEST   " +
             std::string(x.consumed ? "consumed" : "inert"));
      std::string rest = x.why;
      while(!rest.empty()) {
        size_t cut = rest.size() <= 60 ? rest.size() : 60;
        if(cut < rest.size()) {
          const size_t sp = rest.rfind(' ', cut);
          if(sp != std::string::npos && sp > 0) cut = sp;
        }
        f.note("              " + rest.substr(0, cut));
        rest = cut >= rest.size() ? "" : rest.substr(cut + 1);
      }
    } else {
      for(const Features::Cover &c : x.covers) {
        f.note("    " +
               pad(c.level == Features::Level::Top ? "top" : "unit", 6) +
               pad(c.bench, 12) + c.test);
      }
    }
  }
}

// --------------------------------------------------------------------
// R-6c. The derived geometry per node.
// --------------------------------------------------------------------
void GenLog::geometry(SvFile &f, const Model &m)
{
  f.note("R-6c. THE DERIVED GEOMETRY, node by node.");
  f.bar();
  f.note("Every value here is COMPUTED from the configuration and");
  f.note("none of it is read from it, D-37. It is the same content");
  f.note("the console report carries, written where a later stage or");
  f.note("a reader can diff it.");
  f.bar();
  f.note("system     " + (m.system_name.empty()
                          ? std::string("<unnamed>") : m.system_name));
  f.note("pa_bits    " + (m.has_addressing ? i2s(m.pa_bits)
                                           : std::string("n/a")));
  f.note("va_bits    " + (m.has_va_bits ? i2s(m.va_bits)
                                        : std::string("n/a")));
  f.note("page_bytes " + (m.has_page_bytes ? i2s(m.page_bytes)
                                           : std::string("n/a")));
  f.ln();

  for(const Model::Node &n : m.nodes) {
    f.bar();
    f.note("node " + n.name + ", cache " + n.cache + ", type " +
           n.node_type +
           (n.indexing.empty() ? "" : ", indexing " + n.indexing));
    f.bar();

    if(!n.resolved) {
      f.note("  unresolved, no geometry derived");
      f.ln();
      continue;
    }

    const Model::Geom &g = n.geom;
    if(!g.valid && g.capacity_bytes == 0) {
      f.note("  no geometry block, nothing to derive");
      f.ln();
      continue;
    }

    f.note("  capacity      " + u64(g.capacity_bytes) + " bytes");
    f.note("  line          " + u64(g.line_bytes) + " bytes");
    f.note("  ways          " + i2s(g.associativity));
    f.note("  banks         " + i2s(g.banks));

    if(!g.valid) {
      f.note("  derivation incomplete, see the diagnostics");
      f.ln();
      continue;
    }

    f.note("  sets          " + u64(g.sets));
    f.note("  sets per bank " + u64(g.sets_per_bank));
    f.note("  bytes per way " + u64(g.bytes_per_way));
    f.note("  refill beats  " +
           (g.refill_beats >= 0 ? i2s(g.refill_beats)
                                : std::string("n/a")) +
           (g.refill_note.empty() ? "" : "  (" + g.refill_note + ")"));
    f.ln();
    f.note("  bits offset " + i2s(g.offset_bits) + "  index " +
           i2s(g.index_bits) + "  tag " + i2s(g.tag_bits) +
           "  bank " + i2s(g.bank_bits));
    f.note("  " + field("offset", g.offset, m.pa_bits));
    f.note("  " + field("index",  g.index,  m.pa_bits));
    f.note("  " + field("tag",    g.tag,    m.pa_bits));

    if(g.banks > 1) {
      if(g.bank_resolved) {
        f.note("  " + field("bank",   g.bank,      m.pa_bits));
        f.note("  " + field("setidx", g.set_index, m.pa_bits));
      } else {
        f.note("  bank    UNRESOLVED, no bounds are emitted");
      }
      if(!g.bank_note.empty()) {
        // wrapped so the note cannot push the log over 80 columns
        std::string rest = g.bank_note;
        while(!rest.empty()) {
          size_t cut = rest.size() <= 66 ? rest.size() : 66;
          if(cut < rest.size()) {
            const size_t sp = rest.rfind(' ', cut);
            if(sp != std::string::npos && sp > 0) cut = sp;
          }
          f.note("    " + rest.substr(0, cut));
          rest = cut >= rest.size() ? "" : rest.substr(cut + 1);
        }
      }
    }
    f.ln();
  }
}

} // namespace cgen
