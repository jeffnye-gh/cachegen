// --------------------------------------------------------------------
// FILE:    test_schema_files.cpp
// SOURCE:  CLI-003
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The schema files on disk have to declare the draft the tool's own
// validator implements. pboettch json-schema-validator implements
// draft 7 and discards $schema rather than erroring on it, so a file
// declaring a draft the validator does not implement is accepted and
// validated as draft 7 anyway. Nothing in the tool reports that, so
// the assertion has to be made here.
//
// $schema carries the JSON Schema draft. It is not the project's
// schema version, which lives in $id and in schema_version, D-47.
//
// The directory is enumerated rather than listed, so a sixth schema
// file cannot arrive without this assertion applying to it.
// --------------------------------------------------------------------
#include "fixture.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using cgen::Fixture;
using nlohmann::json;

namespace {

const char *DRAFT_07 = "http://json-schema.org/draft-07/schema#";

// ------------------------------------------------------------------
// The schema directory the tool actually used, so the test cannot
// drift onto a different copy from the one that was validated
// against.
// ------------------------------------------------------------------
std::string schema_dir_in_use()
{
  auto drv = Fixture::run(Fixture::fixture_dir() +
                          "/base/base_system.json");
  return drv->schemas().dir();
}

} // namespace

// --------------------------------------------------------------------
TEST(SchemaFiles, DeclareDraft07)
{
  const std::string dir = schema_dir_in_use();
  ASSERT_FALSE(dir.empty()) << "the schema directory was not located";

  std::error_code ec;
  std::vector<std::string> files;

  for(const fs::directory_entry &e : fs::directory_iterator(dir, ec)) {
    if(!e.is_regular_file()) continue;
    const std::string name = e.path().filename().generic_string();
    if(name.size() < 12) continue;
    if(name.compare(name.size() - 12, 12, ".schema.json") != 0) continue;
    files.push_back(e.path().generic_string());
  }

  ASSERT_FALSE(ec) << "cannot read " << dir;
  EXPECT_LE(size_t(5), files.size())
    << "expected at least the five schemas in " << dir;

  for(const std::string &p : files) {
    std::ifstream in(p);
    ASSERT_TRUE(in.is_open()) << p;

    json s;
    ASSERT_NO_THROW(in >> s) << p;

    ASSERT_TRUE(s.is_object())        << p << " is not an object";
    ASSERT_TRUE(s.contains("$schema")) << p << " declares no $schema";
    ASSERT_TRUE(s["$schema"].is_string())
      << p << " $schema is not a string";

    EXPECT_EQ(std::string(DRAFT_07), s["$schema"].get<std::string>())
      << p << " declares a draft the tool's validator does not "
      << "implement. The validator is draft 7 and discards $schema, "
      << "so the mismatch is silent at run time.";
  }
}
