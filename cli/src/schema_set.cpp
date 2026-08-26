// --------------------------------------------------------------------
// FILE:    schema_set.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "schema_set.h"
#include "msg.h"
#include <nlohmann/json-schema.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using nlohmann::json;

namespace cgen
{

namespace {

// ------------------------------------------------------------------
// Collects every validator complaint as a diagnostic instead of
// throwing on the first one. The JSON pointer the validator hands
// back is the path into the document, which is what R-10 wants.
// ------------------------------------------------------------------
class Collector : public nlohmann::json_schema::error_handler
{
public:
  Collector(DiagList &d, const std::string &f) : diags_(d), file_(f) {}

  void error(const json::json_pointer &ptr,
             const json &,
             const std::string &message) override
  {
    std::string p = ptr.to_string();
    if(p.empty()) p = "/";
    diags_.error(file_, p, "schema.violation", message);
  }

private:
  DiagList   &diags_;
  std::string file_;
};

} // namespace

// --------------------------------------------------------------------
const std::vector<std::string> &SchemaSet::types()
{
  static const std::vector<std::string> t = {
    "system", "ports", "caches", "links", "topology"
  };
  return t;
}

// --------------------------------------------------------------------
SchemaSet::SchemaSet(DiagList &diags) : diags_(diags) {}
SchemaSet::~SchemaSet() = default;

// --------------------------------------------------------------------
bool SchemaSet::holds_all(const std::string &d) const
{
  if(d.empty()) return false;
  for(const std::string &t : types()) {
    if(!fs::exists(fs::path(d) / (t + ".schema.json"))) return false;
  }
  return true;
}

// --------------------------------------------------------------------
bool SchemaSet::locate(const std::string &config_dir)
{
  const char *env = std::getenv("CGEN_SCHEMA_DIR");
  if(env && holds_all(env)) {
    dir_ = env;
    return read_schemas();
  }
  if(env) {
    diags_.warn("", "", "schema.dir",
                "CGEN_SCHEMA_DIR " + msg->tq(env) +
                " does not hold the five schemas, ignoring it");
  }

  std::error_code ec;
  std::vector<fs::path> starts;
  if(!config_dir.empty()) {
    starts.push_back(fs::weakly_canonical(fs::path(config_dir), ec));
  }
  starts.push_back(fs::current_path(ec));

  for(const fs::path &s : starts) {
    fs::path p = s;
    for(int up = 0; up < 16; ++up) {
      fs::path c = p / "planning" / "schema";
      if(holds_all(c.generic_string())) {
        dir_ = c.generic_string();
        return read_schemas();
      }
      if(!p.has_parent_path() || p.parent_path() == p) break;
      p = p.parent_path();
    }
  }

  diags_.error("", "", "schema.dir",
               "cannot locate the schema directory, set CGEN_SCHEMA_DIR "
               "or run from a tree holding planning/schema");
  return false;
}

// --------------------------------------------------------------------
bool SchemaSet::read_schemas()
{
  bool ok = true;

  for(const std::string &t : types()) {
    fs::path p = fs::path(dir_) / (t + ".schema.json");
    std::ifstream in(p);
    if(!in.is_open()) {
      diags_.error(Loader::display_path(p.generic_string()), "",
                   "schema.open", "cannot open schema");
      ok = false;
      continue;
    }

    json s;
    try {
      in >> s;
    } catch(const json::parse_error &e) {
      diags_.error(Loader::display_path(p.generic_string()), "",
                   "schema.parse",
                   "schema parse failed: " + std::string(e.what()));
      ok = false;
      continue;
    }

    auto v = std::make_unique<nlohmann::json_schema::json_validator>();
    try {
      v->set_root_schema(s);
    } catch(const std::exception &e) {
      diags_.error(Loader::display_path(p.generic_string()), "",
                   "schema.build",
                   "schema is not usable: " + std::string(e.what()));
      ok = false;
      continue;
    }

    val_[t] = std::move(v);
  }

  return ok;
}

// --------------------------------------------------------------------
void SchemaSet::validate(const Loader::File &f)
{
  if(f.declared.empty()) return;   // already reported by the loader

  auto it = val_.find(f.declared);
  if(it == val_.end()) {
    diags_.error(f.disp, "/file_type", "schema.unknown_type",
                 "no schema for file_type " + msg->tq(f.declared));
    return;
  }

  Collector c(diags_, f.disp);
  it->second->validate(f.doc, c);
}

} // namespace cgen
