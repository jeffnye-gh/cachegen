// --------------------------------------------------------------------
// FILE:    tool_vars.cpp
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "tool_vars.h"
#include "sv_file.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace cgen
{

// --------------------------------------------------------------------
// R-3 and R-4. Every tool the emitted build invokes, in one place.
//
// MAKE is on the list and is NOT assigned. make defines it itself and
// it carries the make that is actually running, which is a better
// value than any this tool could write. It is listed so that R-4's
// assertion covers the bare name 'make' as well.
// --------------------------------------------------------------------
const std::vector<ToolVars::Tool> &ToolVars::all()
{
  static const std::vector<Tool> t = {
    { "VERILATOR", "verilator",
      "$(CGEN_ROOT)/tools/bin/verilator",
      "lints, compiles and elaborates every emitted file", true },

    { "MAKE", "make",
      "",
      "recursion from the system build into each node directory",
      false },

    { "RM", "rm",
      "rm",
      "removes the object directory and the image directory on clean",
      true },

    { "MKDIR", "mkdir",
      "mkdir",
      "creates the image directory the run writes memory images into",
      true },

    { "ECHO", "echo",
      "echo",
      "the progress line each recursive target prints", true }
  };
  return t;
}

// --------------------------------------------------------------------
const ToolVars::Tool *ToolVars::find_var(const std::string &var)
{
  for(const Tool &t : all()) {
    if(var == t.var) return &t;
  }
  return nullptr;
}

// --------------------------------------------------------------------
std::string ToolVars::cgen_root()
{
  const char *e = std::getenv("CGEN_ROOT");
  if(e == nullptr || *e == 0) return "";

  std::string s = e;
  while(s.size() > 1 && (s.back() == '/' || s.back() == '\\')) {
    s.pop_back();
  }
  return s;
}

// --------------------------------------------------------------------
std::string ToolVars::default_source()
{
  const std::string root = cgen_root();
  if(!root.empty()) return root + "/" + relative_source();

  // CGEN_ROOT is not set. Walk up for the same relative path so the
  // gtest suite, which runs from cli/, still finds the master copy.
  std::error_code ec;
  std::filesystem::path p = std::filesystem::current_path(ec);
  for(int up = 0; up < 8; ++up) {
    const std::filesystem::path c = p / relative_source();
    if(std::filesystem::exists(c, ec)) return c.generic_string();
    if(!p.has_parent_path() || p.parent_path() == p) break;
    p = p.parent_path();
  }
  return relative_source();
}

namespace {

// ------------------------------------------------------------------
// The variable a Make assignment line sets, empty when the line is
// not an assignment. Handles VAR=, VAR =, VAR ?=, VAR := and VAR +=.
// ------------------------------------------------------------------
std::string assigned_var(const std::string &line)
{
  size_t i = 0;
  while(i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
  if(i >= line.size() || line[i] == '#') return "";

  const size_t beg = i;
  while(i < line.size() &&
        ((line[i] >= 'A' && line[i] <= 'Z') ||
         (line[i] >= 'a' && line[i] <= 'z') ||
         (line[i] >= '0' && line[i] <= '9') || line[i] == '_')) {
    ++i;
  }
  if(i == beg) return "";

  const std::string name = line.substr(beg, i - beg);
  while(i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
  if(i >= line.size()) return "";

  if(line[i] == '=')                      return name;
  if(i + 1 < line.size() && line[i + 1] == '=' &&
     (line[i] == '?' || line[i] == ':' || line[i] == '+')) {
    return name;
  }
  return "";
}

std::string base_of(const std::string &p)
{
  const size_t k = p.find_last_of("/\\");
  return k == std::string::npos ? p : p.substr(k + 1);
}

} // namespace

// --------------------------------------------------------------------
bool ToolVars::load(const std::string &path, std::string &why)
{
  std::ifstream is(path, std::ios::binary);
  if(!is) {
    why = "the source Vars.mk cannot be read";
    return false;
  }

  std::ostringstream ss;
  ss << is.rdbuf();
  src_body_ = ss.str();
  src_path_ = path;
  src_base_ = base_of(path);

  std::istringstream ls(src_body_);
  std::string line;
  while(std::getline(ls, line)) {
    const std::string v = assigned_var(line);
    if(!v.empty()) src_assigns_.push_back(v);
  }
  return true;
}

// --------------------------------------------------------------------
bool ToolVars::assigned_in_master(const std::string &var) const
{
  for(const std::string &s : src_assigns_) {
    if(s == var) return true;
  }
  return false;
}

// --------------------------------------------------------------------
// --tool VAR=PATH. CGEN_ROOT is special, see the header.
// --------------------------------------------------------------------
bool ToolVars::set(const std::string &spec, const std::string &root,
                   std::string &why)
{
  const size_t eq = spec.find('=');
  if(eq == std::string::npos || eq == 0) {
    why = "expected VAR=PATH";
    return false;
  }

  const std::string var  = spec.substr(0, eq);
  std::string       path = spec.substr(eq + 1);

  if(find_var(var) == nullptr) {
    why = "'" + var + "' is not a tool variable of the emitted build";
    return false;
  }
  if(path.empty()) {
    why = "the path is empty";
    return false;
  }

  // ------------------------------------------------------------------
  // A path inside the tree is written in the $(CGEN_ROOT)/... form so
  // the emitted tree stays portable. A path outside it is written as
  // given, and the tree is machine specific by the user's choice.
  // ------------------------------------------------------------------
  if(!root.empty() && path.size() > root.size() &&
     path.compare(0, root.size(), root) == 0 &&
     (path[root.size()] == '/' || path[root.size()] == '\\')) {
    path = "$(CGEN_ROOT)" + path.substr(root.size());
  } else if(path.compare(0, 12, "$(CGEN_ROOT)") != 0) {
    machine_specific_ = true;
  }

  for(auto &kv : over_) {
    if(kv.first == var) { kv.second = path; return true; }
  }
  over_.push_back({ var, path });
  return true;
}

// --------------------------------------------------------------------
std::string ToolVars::value_of(const Tool &t) const
{
  for(const auto &kv : over_) {
    if(kv.first == t.var) return kv.second;
  }
  if(assigned_in_master(t.var)) return "set by " + src_base_;
  if(!t.assign)                 return "make's own value";
  return t.deflt;
}

// --------------------------------------------------------------------
// The emitted Vars.mk. The master copy goes in verbatim, then the
// tools it does not name, then the command line, so the later
// assignment is the one that stands.
// --------------------------------------------------------------------
void ToolVars::emit(SvFile &f) const
{
  f.note("The tool variable set of the emitted build, R-3.");
  f.note("");
  f.note("Every recipe in every emitted Makefile invokes a tool");
  f.note("through one of the variables below and never by a bare");
  f.note("name, R-4. A Makefile that says 'verilator' runs whatever");
  f.note("is on PATH, which is not necessarily the one this tree");
  f.note("was built and checked against.");
  f.note("");
  f.note("THIS IS THE ONE EMITTED FILE WHOSE CONTENTS MAY VARY WITH");
  f.note("THE COMMAND LINE. Every other emitted file is byte");
  f.note("identical between two runs of one configuration, R-11.");
  f.bar();
  f.note("The master copy, " + src_base_ + ", verbatim. It is read");
  f.note("and copied by cgen and never written.");
  f.note("begin the master copy");

  // the master, line by line, so trailing whitespace is trimmed the
  // same way every other emitted line is
  size_t at = 0;
  while(at < src_body_.size()) {
    const size_t nl  = src_body_.find('\n', at);
    const size_t end = nl == std::string::npos ? src_body_.size() : nl;
    f.ln(src_body_.substr(at, end - at));
    if(nl == std::string::npos) break;
    at = nl + 1;
  }

  f.note("end the master copy");
  f.bar();
  f.note("Every tool the emitted build uses gets a variable, even");
  f.note("where the value resolves to the system copy. A tool the");
  f.note("master copy already names is not reassigned here.");
  f.ln();

  for(const Tool &t : all()) {
    f.note(std::string(t.var) + ", " + t.what);
    if(!t.assign) {
      f.note("  make defines " + std::string(t.var) + " itself and it "
             "carries the make");
      f.note("  that is running. It is not reassigned.");
      f.ln();
      continue;
    }
    if(assigned_in_master(t.var)) {
      f.note("  assigned by " + src_base_ + " above.");
      f.ln();
      continue;
    }
    f.ln(std::string(t.var) + " = " + t.deflt);
    f.ln();
  }

  if(over_.empty()) return;

  f.bar();
  f.note("Set from the cgen command line. A path inside CGEN_ROOT is");
  f.note("written in the $(CGEN_ROOT)/... form so the tree stays");
  f.note("portable.");
  if(machine_specific_) {
    f.note("");
    f.note("A path below is OUTSIDE CGEN_ROOT and is written as it");
    f.note("was given, so THIS OUTPUT TREE IS MACHINE SPECIFIC. That");
    f.note("is the user's choice and not a defect. Emit again with");
    f.note("no --tool, or with a path inside the tree, to undo it.");
  }
  f.ln();
  for(const auto &kv : over_) {
    f.ln(kv.first + " = " + kv.second);
  }
}

} // namespace cgen
