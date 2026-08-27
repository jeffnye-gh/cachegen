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

  // the codes of every diagnostic, in order, joined by a comma
  static std::string codes(const DiagList &d);
};

} // namespace cgen
