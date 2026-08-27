// --------------------------------------------------------------------
// FILE:    fixture.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "fixture.h"
#include "tool_vars.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace cgen
{

namespace {

const char *SCHEMA_DIR_ENV = "CGEN_SCHEMA_DIR";

// ------------------------------------------------------------------
// True when name ends with tail.
// ------------------------------------------------------------------
bool ends_with(const std::string &name, const std::string &tail)
{
  if(name.size() < tail.size()) return false;
  return name.compare(name.size() - tail.size(), tail.size(), tail) == 0;
}


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
// One system file per fixture directory that carries one. base/ names
// its file base_system.json and the neg_* directories name theirs
// system.json, so the tail is what is matched, not the whole name.
// --------------------------------------------------------------------
std::vector<std::string> Fixture::configs()
{
  std::vector<std::string> out;
  std::error_code ec;

  for(const fs::directory_entry &d :
      fs::directory_iterator(fixture_dir(), ec)) {
    if(!d.is_directory()) continue;

    for(const fs::directory_entry &f :
        fs::directory_iterator(d.path(), ec)) {
      if(!f.is_regular_file()) continue;
      if(!ends_with(f.path().filename().generic_string(), "system.json")) {
        continue;
      }
      out.push_back(f.path().generic_string());
    }
  }

  std::sort(out.begin(), out.end());

  std::string p = pacino();
  if(!p.empty()) out.push_back(p);

  return out;
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
std::unique_ptr<Driver> Fixture::run_with_schema_dir(
    const std::string &system_file,
    const std::string &schema_dir)
{
  const char *prev  = std::getenv(SCHEMA_DIR_ENV);
  const bool  had   = prev != nullptr;
  const std::string saved = had ? prev : "";

  setenv(SCHEMA_DIR_ENV, schema_dir.c_str(), 1);
  std::unique_ptr<Driver> d = run(system_file);

  if(had) setenv(SCHEMA_DIR_ENV, saved.c_str(), 1);
  else    unsetenv(SCHEMA_DIR_ENV);

  return d;
}

// --------------------------------------------------------------------
std::unique_ptr<Driver> Fixture::run_emit(const std::string &system_file,
                                          const std::string &out_dir)
{
  Driver::Args a;
  a.cmd    = "emit";
  a.config = system_file;
  a.output = out_dir;
  a.quiet  = true;

  auto d = std::make_unique<Driver>(a);
  d->run();
  return d;
}

// --------------------------------------------------------------------
std::unique_ptr<Driver> Fixture::run_emit_vars(
    const std::string &system_file,
    const std::string &out_dir,
    const std::string &vars,
    const std::vector<std::string> &tools)
{
  Driver::Args a;
  a.cmd    = "emit";
  a.config = system_file;
  a.output = out_dir;
  a.vars   = vars;
  a.tool   = tools;
  a.quiet  = true;

  auto d = std::make_unique<Driver>(a);
  d->run();
  return d;
}

// --------------------------------------------------------------------
std::string Fixture::vars_master()
{
  return ToolVars::default_source();
}

// --------------------------------------------------------------------
std::string Fixture::scratch(const std::string &leaf)
{
  std::error_code ec;
  const fs::path p = fs::temp_directory_path(ec) / ("cgen_tb_" + leaf);
  fs::remove_all(p, ec);
  fs::create_directories(p, ec);
  return p.generic_string();
}

// --------------------------------------------------------------------
std::vector<std::string> Fixture::tree(const std::string &dir)
{
  std::vector<std::string> out;
  std::error_code ec;

  const fs::path root(dir);
  for(fs::recursive_directory_iterator it(root, ec), end;
      it != end; it.increment(ec)) {
    if(ec) break;
    if(!it->is_regular_file()) continue;
    out.push_back(fs::relative(it->path(), root, ec).generic_string());
  }

  std::sort(out.begin(), out.end());
  return out;
}

// --------------------------------------------------------------------
std::string Fixture::slurp(const std::string &path)
{
  std::ifstream is(path, std::ios::binary);
  if(!is) return "";
  return std::string(std::istreambuf_iterator<char>(is),
                     std::istreambuf_iterator<char>());
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
