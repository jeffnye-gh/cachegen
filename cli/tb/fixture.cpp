// --------------------------------------------------------------------
// FILE:    fixture.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "fixture.h"
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace cgen
{

namespace {

// ------------------------------------------------------------------
// Walk up from the working directory looking for a known path, so
// the suite runs from cli/ or from the repository root.
// ------------------------------------------------------------------
std::string find_up(const char *want)
{
  std::error_code ec;
  fs::path p = fs::current_path(ec);
  for(int up = 0; up < 8; ++up) {
    fs::path c = p / want;
    if(fs::exists(c)) return c.generic_string();
    if(!p.has_parent_path() || p.parent_path() == p) break;
    p = p.parent_path();
  }
  return "";
}

} // namespace

// --------------------------------------------------------------------
std::string Fixture::fixture_dir()
{
  const char *env = std::getenv("CGEN_FIXTURE_DIR");
  if(env && *env) return env;

  std::string d = find_up("cli/tb/fixtures");
  if(!d.empty()) return d;
  return "tb/fixtures";
}

// --------------------------------------------------------------------
std::string Fixture::pacino()
{
  const char *env = std::getenv("CGEN_PACINO");
  if(env && *env) return env;

  std::string d = find_up("testcases/pacino/pacino_system.json");
  if(!d.empty()) return d;
  return "../testcases/pacino/pacino_system.json";
}

// --------------------------------------------------------------------
std::unique_ptr<Driver> Fixture::run(const std::string &system_file)
{
  Driver::Args a;
  a.cmd    = "check";
  a.config = system_file;
  a.quiet  = true;

  auto d = std::make_unique<Driver>(a);
  d->run();
  return d;
}

// --------------------------------------------------------------------
std::unique_ptr<Driver> Fixture::run_neg(const std::string &name)
{
  return run(fixture_dir() + "/" + name + "/system.json");
}

// --------------------------------------------------------------------
std::string Fixture::codes(const DiagList &d)
{
  std::string s;
  for(size_t i = 0; i < d.all().size(); ++i) {
    if(i) s += ",";
    s += d.all()[i].code();
  }
  return s;
}

} // namespace cgen
