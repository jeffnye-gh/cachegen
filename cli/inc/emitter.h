// --------------------------------------------------------------------
// FILE:    emitter.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// --cmd=emit. Turns a checked configuration into the output tree.
//
// R-3: the check path runs first and NOTHING is written if any
// diagnostic was an error. A warning does not stop emission.
//
// R-4, the layout:
//   <out>/<node>/rtl/     one module per file
//   <out>/<node>/tb/      the unit testbench and its tests
//   <out>/<system>/       the system top, its testbench, the build
//
// <node> is the topology INSTANCE name, so two instances of one cache
// definition get two directories and cannot collide.
//
// R-11: emission is deterministic. Two runs of one configuration
// produce byte identical files. Nothing derived from the clock, the
// host, the user, an absolute path or a git sha reaches an output
// file.
// --------------------------------------------------------------------
#pragma once
#include "diag_list.h"
#include "feature_table.h"
#include "field_use.h"
#include "gen_log.h"
#include "link_sig.h"
#include "loader.h"
#include "model.h"
#include "node_ctx.h"
#include "sv_file.h"
#include "tool_vars.h"
#include <map>
#include <string>
#include <vector>

namespace cgen
{

class Emitter
{
public:
  Emitter(DiagList &diags, const std::string &out_dir)
    : diags_(diags), out_(out_dir) {}

  // ------------------------------------------------------------------
  // R-3. The master Vars.mk and the tool paths the command line set.
  // An empty path means ToolVars::default_source().
  // ------------------------------------------------------------------
  void set_vars(const std::string &vars_path,
                const std::vector<std::string> &tools) {
    vars_path_ = vars_path;
    tool_args_ = tools;
  }

  const ToolVars &tool_vars() const { return tools_; }

  // R-6. What the run recorded reading, for the unconsumed report.
  void set_field_use(const FieldUse *u) { use_ = u; }

  // R-8. The feature table this run built.
  const Features &features() const { return feats_; }

  // Returns false when nothing was written. R-3 refusal is the
  // ordinary case of that and is reported through emit.refused.
  bool run(const Model &m, const Loader &loader);

  // every path written, relative to the output directory, sorted
  const std::vector<std::string> &written() const { return written_; }

  // what the emitter had to decide because nothing else does. R-9
  // reads this rather than the emitter's source.
  const std::vector<std::string> &notes() const { return notes_; }

private:
  bool build_nodes(const Model &m, const Loader &loader);
  bool emit_vars();                 // R-3, Vars.mk at the output root
  void emit_logs(const Model &m, const Loader &loader);
  void emit_shared(const Model &m);
  void emit_node(const NodeCtx &c);
  void emit_system(const Model &m);
  void emit_build(const Model &m);

  bool put(const std::string &rel, const SvFile &f);
  bool mkpath(const std::string &rel);

  SvFile file(const std::string &name,
              SvFile::Kind k = SvFile::Kind::Sv) const;

  // "l1i" -> "L1i", the camel form of a node name
  static std::string camel(const std::string &n);

  DiagList   &diags_;
  std::string out_;
  std::string sys_;
  std::string src_;          // the BASE name of the config, R-11

  ToolVars                 tools_;
  std::string              vars_path_;
  std::vector<std::string> tool_args_;
  const FieldUse          *use_{nullptr};
  Features                 feats_;

  std::map<std::string, NodeCtx>  nodes_;
  std::map<std::string, LinkRef>  links_;
  std::vector<GenLog::Skipped>    skipped_;
  std::vector<std::string> written_;
  std::vector<std::string> notes_;
  std::vector<std::string> shared_;   // shared package paths

  // the node whose files are being written, empty between nodes.
  // Its package members carry its name, see SvFile::set_prefix.
  std::string pfx_snake_;
  std::string pfx_camel_;
};

} // namespace cgen
