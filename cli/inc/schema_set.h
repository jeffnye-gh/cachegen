// --------------------------------------------------------------------
// FILE:    schema_set.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// Loads the five schemas from disk, never compiled in, and validates
// each loaded document against the schema for its declared file_type.
// See R-4. The search order used to find the directory is documented
// on locate().
// --------------------------------------------------------------------
#pragma once
#include "diag_list.h"
#include "loader.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nlohmann { namespace json_schema { class json_validator; } }

namespace cgen
{

class SchemaSet
{
public:
  explicit SchemaSet(DiagList &diags);
  ~SchemaSet();

  // Search order, first directory holding all five schemas wins:
  //   1. $CGEN_SCHEMA_DIR
  //   2. planning/schema, walking up from the system file directory
  //   3. planning/schema, walking up from the working directory
  bool locate(const std::string &config_dir);

  const std::string &dir() const { return dir_; }

  // validate one loaded file against the schema for its file_type
  void validate(const Loader::File &f);

  static const std::vector<std::string> &types();

private:
  bool holds_all(const std::string &d) const;
  bool read_schemas();

  DiagList   &diags_;
  std::string dir_;

  std::map<std::string,
           std::unique_ptr<nlohmann::json_schema::json_validator>> val_;
};

} // namespace cgen
