// --------------------------------------------------------------------
// FILE:    fixture.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// Locates the test configurations and runs one front half pass over
// a chosen configuration. The Driver owns everything the Model points
// into, so it is handed back by pointer and kept alive by the caller.
// --------------------------------------------------------------------
#pragma once
#include "driver.h"
#include <memory>
#include <string>
#include <vector>

namespace cgen
{

class Fixture
{
public:
  // directory holding base/ and the neg_* configurations
  static std::string fixture_dir();

  // the pacino system file
  static std::string pacino();

  // Every configuration the suite can run, sorted: the system file
  // of each fixture directory that has one, plus pacino. Enumerated
  // rather than listed, so a new fixture directory is picked up
  // without an edit here.
  static std::vector<std::string> configs();

  // run a negative fixture, tb/fixtures/<name>/system.json
  static std::unique_ptr<Driver> run_neg(const std::string &name);

  // run any system file
  static std::unique_ptr<Driver> run(const std::string &system_file);

  // Run with CGEN_SCHEMA_DIR pointed somewhere else, then put the
  // variable back. This is the only way to reach the diagnostics
  // that are about the schema directory rather than the input.
  static std::unique_ptr<Driver> run_with_schema_dir(
      const std::string &system_file,
      const std::string &schema_dir);

  // ------------------------------------------------------------------
  // CLI-004. The emit path.
  // ------------------------------------------------------------------

  // run one configuration with --cmd=emit into out_dir
  static std::unique_ptr<Driver> run_emit(const std::string &system_file,
                                          const std::string &out_dir);

  // ------------------------------------------------------------------
  // CLI-005, R-3. The same with a chosen master Vars.mk and a chosen
  // set of --tool overrides. An empty vars path takes the default.
  // ------------------------------------------------------------------
  static std::unique_ptr<Driver> run_emit_vars(
      const std::string &system_file,
      const std::string &out_dir,
      const std::string &vars,
      const std::vector<std::string> &tools = {});

  // the master Vars.mk the tool would take by default
  static std::string vars_master();

  // a scratch directory under the system temp area, emptied first so
  // one run cannot see what an earlier one left
  static std::string scratch(const std::string &leaf);

  // every regular file under dir, relative to it, sorted
  static std::vector<std::string> tree(const std::string &dir);

  // the whole of one file, empty when it cannot be read
  static std::string slurp(const std::string &path);

  // the codes of every diagnostic, in order, joined by a comma
  static std::string codes(const DiagList &d);
};

} // namespace cgen
