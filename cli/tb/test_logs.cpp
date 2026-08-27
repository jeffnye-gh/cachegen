// --------------------------------------------------------------------
// FILE:    test_logs.cpp
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
//
// R-6. The generation logs, and the one thing about them that is easy
// to get wrong: the unconsumed field report has to be DERIVED, not
// maintained.
//
// A test that listed the seven fields pacino leaves unread would be
// the very defect R-6b removes. What is asserted instead is the
// PROPERTY: a field on the list is one no stage read, and a field off
// the list is one some stage did. Both halves come from the recorder,
// so a stage that starts consuming a field moves it without an edit
// here.
//
// The consequence of being on the list is asserted directly and it is
// the one a user cares about: EDIT AN UNCONSUMED FIELD, EMIT AGAIN,
// AND THE OUTPUT IS BYTE FOR BYTE WHAT IT WAS.
// --------------------------------------------------------------------
#include "fixture.h"
#include "field_use.h"
#include "gen_log.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <string>

namespace fs = std::filesystem;

using cgen::FieldUse;
using cgen::Fixture;
using cgen::GenLog;
using nlohmann::json;

namespace {

std::string log_path(const char *name)
{
  return std::string(GenLog::dir()) + "/" + name;
}

bool has(const std::vector<std::string> &v, const std::string &s)
{
  return std::find(v.begin(), v.end(), s) != v.end();
}

} // namespace

// --------------------------------------------------------------------
// R-6 and R-8. Every log is written, under <output>/logs.
// --------------------------------------------------------------------
TEST(Logs, EveryLogIsWritten)
{
  const std::string out = Fixture::scratch("logs");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error())
    << Fixture::codes(drv->diags());

  const std::vector<std::string> files = Fixture::tree(out);
  EXPECT_TRUE(has(files, log_path(GenLog::emission_name())));
  EXPECT_TRUE(has(files, log_path(GenLog::unconsumed_name())));
  EXPECT_TRUE(has(files, log_path(GenLog::geometry_name())));
  EXPECT_TRUE(has(files, log_path(GenLog::features_name())));

  for(const std::string &f : files) {
    if(f.compare(0, 5, "logs/") != 0) continue;
    EXPECT_FALSE(Fixture::slurp(out + "/" + f).empty())
      << f << " is empty";
  }
}

// --------------------------------------------------------------------
// R-6a. The emission log names every file the run wrote, itself
// included. A log that names most of them is worse than none: it
// looks complete.
// --------------------------------------------------------------------
TEST(Logs, TheEmissionLogNamesEveryFileTheRunWrote)
{
  const std::string out = Fixture::scratch("logs_emission");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::string body =
      Fixture::slurp(out + "/" + log_path(GenLog::emission_name()));
  ASSERT_FALSE(body.empty());

  for(const std::string &f : Fixture::tree(out)) {
    EXPECT_NE(std::string::npos, body.find("  " + f))
      << f << " was written and the emission log does not name it";
  }

  // and every node the topology carries
  for(const cgen::Model::Node &n : drv->model().nodes) {
    EXPECT_NE(std::string::npos, body.find("  " + n.name + " "))
      << n.name << " is not in the emission log's node list";
  }
}

// --------------------------------------------------------------------
// R-6b. The property, both directions. Every field the report names
// is one the recorder saw no read of, and every field it does not
// name is one the recorder did.
//
// This is what makes the report derived rather than maintained. It
// says nothing about WHICH fields, on purpose.
// --------------------------------------------------------------------
TEST(Logs, TheUnconsumedReportAgreesWithWhatTheToolRead)
{
  const std::string out = Fixture::scratch("logs_unconsumed");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const FieldUse &u = drv->field_use();
  const std::vector<FieldUse::Leaf> gone = u.unread();

  EXPECT_FALSE(u.leaves().empty())
    << "no configuration field was enumerated at all";
  EXPECT_FALSE(gone.empty())
    << "pacino carries fields no stage reads and the report is empty";
  EXPECT_LT(gone.size(), u.leaves().size())
    << "every field is unread, the recorder is not being called";

  const std::string body =
      Fixture::slurp(out + "/" + log_path(GenLog::unconsumed_name()));
  ASSERT_FALSE(body.empty());

  // every unread field is named
  for(const FieldUse::Leaf &l : gone) {
    EXPECT_NE(std::string::npos, body.find("  " + l.ptr))
      << l.ptr << " was read by nothing and the report omits it";
  }

  // and no field the tool did read is named
  for(const FieldUse::Leaf &l : u.leaves()) {
    if(!u.was_read(l.file, l.ptr)) continue;
    EXPECT_EQ(std::string::npos, body.find("\n#   " + l.ptr + "\n"))
      << l.ptr << " was read and the report names it anyway";
  }
}

// --------------------------------------------------------------------
// R-6b, the consequence. THIS IS THE ASSERTION THAT MATTERS.
//
// A field on the report is one a user can edit, re-emit, and see no
// difference from. That is the whole reason the report exists, and it
// is checked by doing it: one unconsumed integer field is changed in
// a copy of pacino and the two emitted trees are compared byte for
// byte.
//
// The field is CHOSEN FROM THE REPORT at run time, not written here.
// --------------------------------------------------------------------
TEST(Logs, EditingAnUnconsumedFieldChangesNothingEmitted)
{
  const std::string base = Fixture::scratch("inert_base");
  auto d0 = Fixture::run_emit(Fixture::pacino(), base);
  ASSERT_FALSE(d0->diags().has_error());

  // ------------------------------------------------------------------
  // the first unconsumed field in the caches file that carries an
  // integer, so the edit stays inside the schema's enum-free ranges
  // ------------------------------------------------------------------
  const fs::path cfg_dir = fs::path(Fixture::pacino()).parent_path();
  const std::string caches =
      (cfg_dir / "pacino_caches.json").generic_string();

  json doc;
  { std::ifstream in(caches); ASSERT_TRUE(in.is_open()); in >> doc; }

  std::string chosen;
  for(const FieldUse::Leaf &l : d0->field_use().unread()) {
    if(l.file.find("caches") == std::string::npos) continue;
    const json::json_pointer p(l.ptr);
    if(!doc.contains(p))              continue;
    if(!doc[p].is_number_integer())   continue;
    chosen = l.ptr;
    break;
  }
  ASSERT_FALSE(chosen.empty())
    << "no unconsumed integer field to try the property on";

  // ------------------------------------------------------------------
  // a copy of the whole configuration with that one field changed
  // ------------------------------------------------------------------
  const std::string work = Fixture::scratch("inert_cfg");
  std::error_code ec;
  fs::copy(cfg_dir, fs::path(work), fs::copy_options::recursive, ec);
  ASSERT_FALSE(ec) << "could not copy the configuration";

  const json::json_pointer p(chosen);
  doc[p] = doc[p].get<int>() + 1;
  {
    std::ofstream os(work + "/pacino_caches.json");
    ASSERT_TRUE(os.is_open());
    os << doc.dump(2) << "\n";
  }

  const std::string edited = Fixture::scratch("inert_edited");
  auto d1 = Fixture::run_emit(work + "/pacino_system.json", edited);
  ASSERT_FALSE(d1->diags().has_error())
    << "the edited configuration no longer checks: "
    << Fixture::codes(d1->diags());

  const std::vector<std::string> fa = Fixture::tree(base);
  ASSERT_EQ(fa, Fixture::tree(edited))
    << "changing " << chosen << " changed the file set";

  // ------------------------------------------------------------------
  // features.log is the one exemption and it is by design. The R-8
  // table prints the DECLARED VALUE of every feature, so it moves
  // when any field moves, consumed or not. That is what makes it a
  // readable record of the configuration rather than a list of
  // pointers. Everything else, the RTL, the testbenches, the build,
  // and the other three logs, has to be identical.
  // ------------------------------------------------------------------
  const std::string table = std::string(cgen::GenLog::dir()) + "/" +
                            cgen::GenLog::features_name();

  int compared = 0;
  for(const std::string &f : fa) {
    if(f == table) continue;
    ++compared;
    EXPECT_EQ(Fixture::slurp(base + "/" + f),
              Fixture::slurp(edited + "/" + f))
      << f << " changed when " << chosen
      << " was edited, so the field is NOT unconsumed and the report "
      << "is wrong";
  }
  EXPECT_LT(50, compared) << "only " << compared << " files compared";
}

// --------------------------------------------------------------------
// R-6c. The geometry log carries what the console report carries, so
// every node with a geometry block is in it with its field bounds.
// --------------------------------------------------------------------
TEST(Logs, TheGeometryLogCarriesEveryDerivedNode)
{
  const std::string out = Fixture::scratch("logs_geom");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::string body =
      Fixture::slurp(out + "/" + log_path(GenLog::geometry_name()));
  ASSERT_FALSE(body.empty());

  int derived = 0;
  for(const cgen::Model::Node &n : drv->model().nodes) {
    EXPECT_NE(std::string::npos, body.find("node " + n.name + ","))
      << n.name << " is not in the geometry log";
    if(!n.geom.valid) continue;
    ++derived;

    EXPECT_NE(std::string::npos,
              body.find("sets          " +
                        std::to_string(n.geom.sets)))
      << n.name << " has no set count in the geometry log";
  }
  EXPECT_LE(4, derived) << "only " << derived << " nodes derived";
}

// --------------------------------------------------------------------
// R-6 and R-11. Two runs of one configuration with one command line
// write byte identical logs. The only block that is allowed to vary
// is the tool variable set, and that varies with the command line
// rather than with the run, which Vars.ToolPathsReach... asserts.
// --------------------------------------------------------------------
TEST(Logs, TwoRunsWriteIdenticalLogs)
{
  const std::string a = Fixture::scratch("logs_det_a");
  const std::string b = Fixture::scratch("logs_det_b");

  auto d1 = Fixture::run_emit(Fixture::pacino(), a);
  auto d2 = Fixture::run_emit(
      fs::absolute(Fixture::pacino()).generic_string(), b);
  ASSERT_FALSE(d1->diags().has_error());
  ASSERT_FALSE(d2->diags().has_error());

  int checked = 0;
  for(const std::string &f : Fixture::tree(a)) {
    if(f.compare(0, 5, "logs/") != 0) continue;
    ++checked;
    EXPECT_EQ(Fixture::slurp(a + "/" + f), Fixture::slurp(b + "/" + f))
      << f << " differs between two runs of one configuration";
  }
  // the log set, from GenLog rather than a count, so a fifth log is
  // covered by this assertion the day it is added
  const std::vector<std::string> want = {
    GenLog::emission_name(), GenLog::unconsumed_name(),
    GenLog::geometry_name(), GenLog::features_name()
  };
  EXPECT_EQ(int(want.size()), checked)
    << "expected " << want.size() << " logs, found " << checked;
}

// --------------------------------------------------------------------
// R-6 and R-11. A log names a configuration file by its BASE NAME.
// A path is relative to the working directory, so a path in a log
// would make two runs of one configuration from two directories
// disagree.
// --------------------------------------------------------------------
TEST(Logs, NoLogCarriesAPath)
{
  const std::string out = Fixture::scratch("logs_path");
  auto drv = Fixture::run_emit(
      fs::absolute(Fixture::pacino()).generic_string(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::string cfg_dir =
      fs::absolute(Fixture::pacino()).parent_path().generic_string();

  for(const std::string &f : Fixture::tree(out)) {
    if(f.compare(0, 5, "logs/") != 0) continue;
    const std::string body = Fixture::slurp(out + "/" + f);

    EXPECT_EQ(std::string::npos, body.find(cfg_dir))
      << f << " carries the configuration's directory";
    EXPECT_EQ(std::string::npos, body.find(out))
      << f << " carries the output directory";
  }
}
