// --------------------------------------------------------------------
// FILE:    test_options.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The loader and the diagnostic object, the parts of R-3 and R-10 the
// negative fixtures do not reach directly.
// --------------------------------------------------------------------
#include "diag_list.h"
#include "fixture.h"
#include <gtest/gtest.h>

using cgen::Diag;
using cgen::DiagList;
using cgen::Fixture;

// --------------------------------------------------------------------
TEST(Diagnostic, CarriesFilePathSeverityAndMessage)
{
  Diag d(Diag::Sev::Error, "a.json", "/caches/l1", "T-8.tag_bits",
         "tag is too narrow");

  EXPECT_TRUE(d.is_error());
  EXPECT_EQ("a.json",       d.file());
  EXPECT_EQ("/caches/l1",   d.path());
  EXPECT_EQ("T-8.tag_bits", d.code());
  EXPECT_EQ("a.json:/caches/l1: [T-8.tag_bits] tag is too narrow",
            d.format());
  EXPECT_EQ("error", Diag::sev_text(Diag::Sev::Error));
}

// --------------------------------------------------------------------
TEST(Diagnostic, ListAccumulatesUntilExitOnError)
{
  DiagList d;
  d.error("a.json", "/x", "c1", "one");
  d.warn ("a.json", "/y", "c2", "two");
  EXPECT_EQ(size_t(2), d.size());
  EXPECT_EQ(size_t(1), d.error_count());
  EXPECT_TRUE(d.has_error());
  EXPECT_EQ(size_t(1), d.count_code("c1"));

  DiagList e;
  e.set_eoe(true);
  e.warn("a.json", "/y", "c2", "a warning does not stop the run");
  EXPECT_NO_THROW(e.warn("a.json", "/y", "c2", "nor does a second"));
  EXPECT_THROW(e.error("a.json", "/x", "c1", "this one does"),
               DiagList::Halt);
}

// --------------------------------------------------------------------
// A missing config file is reported, not crashed on.
// --------------------------------------------------------------------
TEST(Loader, MissingConfigIsReported)
{
  auto drv = Fixture::run(Fixture::fixture_dir() + "/no_such_file.json");
  EXPECT_TRUE(drv->diags().has_error());
  EXPECT_EQ(size_t(1), drv->diags().count_code("load.open"));
}

// --------------------------------------------------------------------
// Includes resolve relative to the including file, the assumption
// recorded against D-10. base_ports.json is reached only through
// base_caches.json, which sits beside it.
// --------------------------------------------------------------------
TEST(Loader, NestedIncludeResolvesAgainstTheIncludingFile)
{
  auto drv = Fixture::run(Fixture::fixture_dir() +
                          "/base/base_system.json");
  EXPECT_EQ(size_t(0), drv->diags().size());

  // p_init and p_targ came from the nested ports file
  const cgen::Model &m = drv->model();
  const cgen::Model::Edge &e0 = m.edges[0];
  EXPECT_EQ("p_init", e0.from_port_type);
  EXPECT_EQ("p_targ", e0.to_port_type);
}
