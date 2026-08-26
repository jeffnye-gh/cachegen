// --------------------------------------------------------------------
// FILE:    loader.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// Reads the system file and follows every include. Include paths are
// resolved relative to the including file, see the assumption on D-10
// recorded in the task file. Detects include cycles and mismatches
// between the type an include entry claims and the file_type the
// included document declares. See R-3.
// --------------------------------------------------------------------
#pragma once
#include "diag_list.h"
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <vector>

namespace cgen
{

class Loader
{
public:
  struct File {
    std::string    key;       // canonical path, identity for cycles
    std::string    disp;      // path as reported in diagnostics
    std::string    claimed;   // type the include entry claimed
    std::string    declared;  // file_type inside the document
    nlohmann::json doc;
  };

  explicit Loader(DiagList &diags) : diags_(diags) {}

  // returns false when the root file could not be read at all
  bool load(const std::string &system_path);

  const std::vector<File> &files() const { return files_; }

  // directory holding the root system file, used to locate schemas
  const std::string &root_dir() const { return root_dir_; }

  static std::string display_path(const std::string &p);

private:
  void load_one(const std::string &path,
                const std::string &claimed,
                const std::string &from_file,
                const std::string &from_ptr,
                std::vector<std::string> &stack);

  void follow_includes(const File &f, std::vector<std::string> &stack);

  DiagList                &diags_;
  std::vector<File>        files_;
  std::set<std::string>    seen_;      // canonical paths already read
  std::string              root_dir_;
};

} // namespace cgen
