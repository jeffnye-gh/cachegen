// --------------------------------------------------------------------
// FILE:    loader.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "loader.h"
#include "msg.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using nlohmann::json;

namespace cgen
{

// --------------------------------------------------------------------
// Shorten a path against the working directory when that is shorter.
// Purely cosmetic, diagnostics stay unambiguous either way.
// --------------------------------------------------------------------
std::string Loader::display_path(const std::string &p)
{
  std::error_code ec;
  fs::path rel = fs::relative(p, fs::current_path(), ec);
  if(ec || rel.empty()) return p;
  std::string r = rel.generic_string();
  return r.size() < p.size() ? r : p;
}

// --------------------------------------------------------------------
bool Loader::load(const std::string &system_path)
{
  std::error_code ec;
  fs::path root = fs::weakly_canonical(fs::path(system_path), ec);
  if(ec) root = fs::path(system_path);

  root_dir_ = root.parent_path().generic_string();
  if(root_dir_.empty()) root_dir_ = ".";

  size_t before = files_.size();
  std::vector<std::string> stack;
  load_one(root.generic_string(), "system", "", "", stack);

  return files_.size() > before;
}

// --------------------------------------------------------------------
// Read one file, record it, then follow its own include list.
// --------------------------------------------------------------------
void Loader::load_one(const std::string &path,
                      const std::string &claimed,
                      const std::string &from_file,
                      const std::string &from_ptr,
                      std::vector<std::string> &stack)
{
  std::error_code ec;
  fs::path cp = fs::weakly_canonical(fs::path(path), ec);
  std::string key = ec ? path : cp.generic_string();
  std::string disp = display_path(key);

  // ------------------------------------------------------------------
  // cycle: the file is already open further up this include chain
  // ------------------------------------------------------------------
  auto hit = std::find(stack.begin(), stack.end(), key);
  if(hit != stack.end()) {
    std::string chain;
    for(auto it = hit; it != stack.end(); ++it) {
      chain += display_path(*it) + " -> ";
    }
    chain += disp;
    diags_.error(from_file, from_ptr, "load.include_cycle",
                 "include cycle: " + chain);
    return;
  }

  // a file reached twice by different paths is read once, not an error
  if(seen_.count(key)) return;
  seen_.insert(key);

  std::ifstream in(key);
  if(!in.is_open()) {
    diags_.error(from_file.empty() ? disp : from_file, from_ptr,
                 "load.open", "cannot open " + msg->tq(disp));
    return;
  }

  File f;
  f.key     = key;
  f.disp    = disp;
  f.claimed = claimed;

  try {
    in >> f.doc;
  } catch(const json::parse_error &e) {
    diags_.error(disp, "", "load.parse",
                 "JSON parse failed: " + std::string(e.what()));
    return;
  }

  if(!f.doc.is_object() || !f.doc.contains("file_type") ||
     !f.doc["file_type"].is_string()) {
    diags_.error(disp, "/file_type", "load.type",
                 "file_type is missing or is not a string");
    files_.push_back(f);
    return;
  }

  f.declared = f.doc["file_type"].get<std::string>();

  if(f.declared != claimed) {
    diags_.error(from_file.empty() ? disp : from_file,
                 from_ptr.empty() ? "/file_type" : from_ptr,
                 "load.include_type",
                 "include of " + msg->tq(disp) + " claims type " +
                 msg->tq(claimed) + " but the file declares file_type " +
                 msg->tq(f.declared));
  }

  files_.push_back(f);
  follow_includes(files_.back(), stack);
}

// --------------------------------------------------------------------
void Loader::follow_includes(const File &f, std::vector<std::string> &stack)
{
  if(!f.doc.contains("include") || !f.doc["include"].is_array()) return;

  // copy, files_ can reallocate while the recursion runs
  const json inc = f.doc["include"];
  const std::string parent_key  = f.key;
  const std::string parent_disp = f.disp;
  fs::path base = fs::path(parent_key).parent_path();

  stack.push_back(parent_key);

  for(size_t i = 0; i < inc.size(); ++i) {
    const json &e = inc[i];
    std::string ptr = "/include/" + std::to_string(i);

    if(!e.is_object() || !e.contains("file") || !e["file"].is_string() ||
       !e.contains("type") || !e["type"].is_string()) {
      diags_.error(parent_disp, ptr, "load.include_shape",
                   "include entry needs a string file and a string type");
      continue;
    }

    std::string rel  = e["file"].get<std::string>();
    std::string type = e["type"].get<std::string>();
    fs::path    tgt  = fs::path(rel).is_absolute() ? fs::path(rel)
                                                   : base / rel;

    load_one(tgt.generic_string(), type, parent_disp, ptr, stack);
  }

  stack.pop_back();
}

} // namespace cgen
