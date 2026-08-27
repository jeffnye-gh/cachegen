// --------------------------------------------------------------------
// FILE:    tool_vars.h
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
//
// R-3 and R-4. The tool variable set of the emitted build.
//
// The table in all() is the ONE source. The emitted Vars.mk is
// generated from it and the R-4 test enumerates the same table, so a
// tool added to the table is covered by the emitted build and by the
// assertion at once, with no second edit anywhere.
//
// R-4: no emitted Makefile carries a bare tool name. Every invocation
// goes through the variable. A generated Makefile that says
// 'verilator' runs whatever happens to be on PATH, which is how a
// 5.020 four years older than the one in this tree came to report ten
// warnings as errors.
//
// planning/tools/Vars.mk is the MASTER copy. It is read and copied and
// never written. Everything this class adds is appended below the
// copy, so the master's own assignments win unless the command line
// overrides them explicitly.
// --------------------------------------------------------------------
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace cgen
{

class SvFile;

class ToolVars
{
public:
  // ------------------------------------------------------------------
  // One tool the emitted build invokes.
  //
  //   var     the Make variable, VERILATOR
  //   name    the bare command, verilator. No recipe in any emitted
  //           Makefile may carry this as a command word, R-4
  //   deflt   what the generated block assigns when neither the master
  //           copy nor the command line names a path
  //   what    one line, what the emitted build uses it for
  //   assign  false when make already defines the variable and
  //           reassigning it would take a working value away
  // ------------------------------------------------------------------
  struct Tool {
    const char *var;
    const char *name;
    const char *deflt;
    const char *what;
    bool        assign;
  };

  static const std::vector<Tool> &all();
  static const Tool *find_var(const std::string &var);

  // ------------------------------------------------------------------
  // The current expansion of CGEN_ROOT, empty when it is not set, and
  // the master copy path that follows from it.
  //
  // R-3 names the default as planning/tools/Vars.mk relative to
  // CGEN_ROOT. With CGEN_ROOT unset there is no such path, so the
  // working directory is walked upward for the same relative path
  // instead. That fallback is an assumption and is reported.
  // ------------------------------------------------------------------
  static std::string cgen_root();
  static std::string default_source();
  static const char *relative_source() {
    return "planning/tools/Vars.mk";
  }

  // the path an output directory reaches the master copy by, from a
  // node directory and from the system directory alike
  static const char *include_line() { return "include ../Vars.mk"; }
  static const char *file_name()    { return "Vars.mk"; }

  // ------------------------------------------------------------------
  // Read the master copy. Returns false with why set when the file
  // cannot be read; the caller turns that into emit.vars.
  // ------------------------------------------------------------------
  bool load(const std::string &path, std::string &why);

  // ------------------------------------------------------------------
  // One --tool VAR=PATH from the command line.
  //
  // CGEN_ROOT IS SPECIAL. A path that begins with the current
  // expansion of CGEN_ROOT is written back as $(CGEN_ROOT)/... so the
  // emitted tree stays portable. A path outside the tree is written
  // verbatim and the tree is machine specific by the user's choice.
  // ------------------------------------------------------------------
  bool set(const std::string &spec, const std::string &cgen_root,
           std::string &why);

  // true when a path outside CGEN_ROOT was written verbatim
  bool machine_specific() const { return machine_specific_; }

  const std::string &source_path() const { return src_path_; }
  const std::string &source_base() const { return src_base_; }

  const std::vector<std::pair<std::string, std::string>> &
  overrides() const { return over_; }

  // true when the master copy assigns this variable itself
  bool assigned_in_master(const std::string &var) const;

  // the whole emitted Vars.mk below the generated header
  void emit(SvFile &f) const;

  // what the variable resolves to in the emitted file, for the log
  std::string value_of(const Tool &t) const;

private:
  std::string src_path_;
  std::string src_base_;
  std::string src_body_;
  std::vector<std::string> src_assigns_;
  std::vector<std::pair<std::string, std::string>> over_;
  bool machine_specific_{false};
};

} // namespace cgen
