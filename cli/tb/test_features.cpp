// --------------------------------------------------------------------
// FILE:    test_features.cpp
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
//
// R-8 and R-9. The feature table and the memory images.
//
// What is asserted about the table is its INVARIANTS, never its
// contents. A test that listed which features pacino covers would be
// a second copy of the table, and the two would agree until the first
// time one of them was edited.
//
// The invariant that matters: A FEATURE NO STAGE CONSUMES HAS NO
// TEST. A test claiming to cover an inert field would be claiming to
// observe something nothing in the emitted design can do.
// --------------------------------------------------------------------
#include "feature_table.h"
#include "fixture.h"
#include "gen_log.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <set>
#include <string>

using cgen::Features;
using cgen::Fixture;
using cgen::GenLog;

namespace {

std::string features_log(const std::string &out)
{
  return out + "/" + GenLog::dir() + "/" + GenLog::features_name();
}

} // namespace

// --------------------------------------------------------------------
// R-8. Every feature the configuration declares gets a test OR a
// statement of why it cannot have one. Never neither, and never both.
// --------------------------------------------------------------------
TEST(Featnames, EveryFeatureHasATestOrAReason)
{
  const std::string out = Fixture::scratch("feat_all");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error())
    << Fixture::codes(drv->diags());

  const Features &ft = drv->features();
  ASSERT_FALSE(ft.all().empty()) << "the feature table is empty";

  for(const Features::Feature &f : ft.all()) {
    if(f.covers.empty()) {
      EXPECT_FALSE(f.why.empty())
        << f.node << " " << f.ptr
        << " has no test and no reason for having none";
    } else {
      EXPECT_TRUE(f.why.empty())
        << f.node << " " << f.ptr
        << " carries both a test and a reason for having none";
    }
    EXPECT_FALSE(f.node.empty()) << f.ptr << " belongs to no node";
  }
}

// --------------------------------------------------------------------
// R-8, the invariant that matters. A feature no stage consumes cannot
// be tested, because nothing in the emitted design moves when it
// changes. A test claiming one would be claiming the impossible.
// --------------------------------------------------------------------
TEST(Featnames, NoTestClaimsAnUnconsumedFeature)
{
  const std::string out = Fixture::scratch("feat_inert");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  int inert = 0;
  for(const Features::Feature &f : drv->features().all()) {
    if(f.consumed) continue;
    ++inert;
    EXPECT_TRUE(f.covers.empty())
      << f.node << " " << f.ptr
      << " is read by no stage and a test claims to cover it";
  }

  EXPECT_LT(0, inert)
    << "no inert feature was found, so the invariant was not "
    << "exercised at all";
}

// --------------------------------------------------------------------
// R-8. "Seven checks is not coverage." The top level testbench has to
// reach features the unit testbenches do not, or it is not earning
// its place. Asserted as the property rather than as a count.
// --------------------------------------------------------------------
TEST(Featnames, TheTopLevelReachesFeaturesNoUnitTestDoes)
{
  const std::string out = Fixture::scratch("feat_top");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  int only_top = 0;
  int top_covs = 0;

  for(const Features::Feature &f : drv->features().all()) {
    bool top = false;
    bool unit = false;
    for(const Features::Cover &c : f.covers) {
      if(c.level == Features::Level::Top)  top  = true;
      if(c.level == Features::Level::Unit) unit = true;
    }
    if(top)          ++top_covs;
    if(top && !unit) ++only_top;
  }

  EXPECT_LT(0, top_covs) << "the top level covers no feature at all";
  EXPECT_LT(0, only_top)
    << "every feature the top level covers is already covered by a "
    << "unit test, so the top level testbench is proving the nodes "
    << "connect and nothing else";
}

// --------------------------------------------------------------------
// R-8. The table reaches the log, so what is reported is what the
// tool built rather than a transcription of it.
// --------------------------------------------------------------------
TEST(Featnames, TheTableReachesTheLog)
{
  const std::string out = Fixture::scratch("feat_log");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::string body = Fixture::slurp(features_log(out));
  ASSERT_FALSE(body.empty()) << "the feature log is empty";

  for(const Features::Feature &f : drv->features().all()) {
    EXPECT_NE(std::string::npos, body.find("  " + f.ptr))
      << f.ptr << " is in the table and not in the log";
  }

  EXPECT_NE(std::string::npos,
            body.find(std::to_string(drv->features().tested()) +
                      " covered"))
    << "the log's count does not match the table";
}

// --------------------------------------------------------------------
// R-9. The memory model carries the image dump and the two accessors
// a test needs, and the format header says what a reader has to know
// before it can parse a line.
// --------------------------------------------------------------------
TEST(Featnames, TheMemoryModelCarriesTheImageDump)
{
  const std::string out = Fixture::scratch("image_rtl");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  int mems = 0;
  for(const cgen::Model::Node &n : drv->model().nodes) {
    if(n.node_type != "memory") continue;
    ++mems;

    bool found = false;
    for(const std::string &f : Fixture::tree(out)) {
      if(f.compare(0, n.name.size() + 5, n.name + "/rtl/") != 0) {
        continue;
      }
      const std::string body = Fixture::slurp(out + "/" + f);
      if(body.find("task automatic cg_dump_image") ==
         std::string::npos) {
        continue;
      }
      found = true;

      // the accessors a testbench asks the store its questions with
      EXPECT_NE(std::string::npos, body.find("cg_has(input"))
        << f << " dumps an image and has no cg_has";
      // the return type carries the node prefix, so the name is
      // what is looked for and not the whole declaration
      EXPECT_NE(std::string::npos, body.find("cg_peek(input"))
        << f << " dumps an image and has no cg_peek";

      // the header a reader needs. Nothing that varies between runs.
      for(const char *k : { "cgen-memimage 1", "addr_bits", "data_bits",
                            "beat_bytes", "line_bytes", "entries" }) {
        EXPECT_NE(std::string::npos, body.find(k))
          << f << " image header carries no " << k;
      }
      EXPECT_EQ(std::string::npos, body.find("$time"))
        << f << " puts a timestamp in the memory image";
    }
    EXPECT_TRUE(found) << n.name << " emits no memory image dump";
  }
  EXPECT_LT(0, mems) << "the configuration has no memory node";
}

// --------------------------------------------------------------------
// R-9. The top level testbench asks for an image at named points, and
// an intermediate one and a final one are both among them.
// --------------------------------------------------------------------
TEST(Featnames, TheTopLevelTestbenchWritesImagesAtNamedPoints)
{
  const std::string out = Fixture::scratch("image_tb");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::string sys = drv->model().system_name;
  const std::string tests =
      Fixture::slurp(out + "/" + sys + "/tb/" + sys + "_tests.svh");
  ASSERT_FALSE(tests.empty());

  // counted rather than named, so a point added later is still
  // covered by the "more than one" half of this
  size_t at = 0;
  int    points = 0;
  while((at = tests.find("mem_image(\"", at)) != std::string::npos) {
    ++points;
    at += 11;
  }
  EXPECT_LE(2, points)
    << "R-9 asks for intermediate AND final images and the top level "
    << "testbench writes " << points;

  EXPECT_NE(std::string::npos, tests.find("mem_image(\"final\")"))
    << "no final image";

  // and the top level Makefile makes the directory they go in
  const std::string mk = Fixture::slurp(out + "/" + sys + "/Makefile");
  EXPECT_NE(std::string::npos, mk.find("$(IMAGES)"))
    << "the top level Makefile has no image directory";
}
