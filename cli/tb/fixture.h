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

namespace cgen
{

class Fixture
{
public:
  // directory holding base/ and the neg_* configurations
  static std::string fixture_dir();

  // the pacino system file
  static std::string pacino();

  // run a negative fixture, tb/fixtures/<name>/system.json
  static std::unique_ptr<Driver> run_neg(const std::string &name);

  // run any system file
  static std::unique_ptr<Driver> run(const std::string &system_file);

  // the codes of every diagnostic, in order, joined by a comma
  static std::string codes(const DiagList &d);
};

} // namespace cgen
