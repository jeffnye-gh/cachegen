// --------------------------------------------------------------------
// FILE:    test_negative.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// One negative fixture per diagnostic, R-11. Each is minimal and each
// must trigger exactly one diagnostic and no other. The base
// configuration the fixtures are cut from must trigger none.
// --------------------------------------------------------------------
#include "fixture.h"
#include <gtest/gtest.h>

using cgen::Fixture;

namespace {

// ------------------------------------------------------------------
// Assert that a fixture produced exactly one diagnostic, with the
// expected code, and that the message names the offending object.
// ------------------------------------------------------------------
void expect_one(const std::string &name,
                const std::string &code,
                const std::string &names)
{
  auto drv = Fixture::run_neg(name);
  const cgen::DiagList &d = drv->diags();

  ASSERT_EQ(size_t(1), d.size())
    << name << " produced " << Fixture::codes(d);

  const cgen::Diag &x = d.all()[0];
  EXPECT_EQ(code, x.code())      << name << ": " << x.format();
  EXPECT_TRUE(x.is_error())      << name << ": " << x.format();
  EXPECT_FALSE(x.file().empty()) << name << ": no file on the diagnostic";
  EXPECT_NE(std::string::npos, x.message().find(names))
    << name << ": message does not name " << names << ", "
    << x.format();
}

} // namespace

// --------------------------------------------------------------------
TEST(Base, CleanConfigurationHasNoDiagnostic)
{
  auto drv = Fixture::run(Fixture::fixture_dir() +
                          "/base/base_system.json");
  const cgen::DiagList &d = drv->diags();
  EXPECT_EQ(size_t(0), d.size()) << Fixture::codes(d);
  EXPECT_FALSE(d.has_error());
}

// --------------------------------------------------------------------
// T-1, undefined names
// --------------------------------------------------------------------
TEST(Negative, DanglingEdgeEndpoint)
{
  expect_one("neg_dangling_endpoint", "T-1.edge_endpoint", "nosuch");
}

TEST(Negative, DanglingCacheReference)
{
  expect_one("neg_dangling_cache", "T-1.node_cache", "nosuch");
}

// The link is carried by the interface, so an undefined link is a
// diagnostic against the node definition, not against the edge.
TEST(Negative, DanglingLinkReference)
{
  expect_one("neg_dangling_link", "T-1.iface_link", "nosuch");
}

TEST(Negative, UnknownPortType)
{
  expect_one("neg_unknown_port_type", "T-1.port_type", "nosuch");
}

// --------------------------------------------------------------------
// T-2, duplicates
// --------------------------------------------------------------------
TEST(Negative, DuplicateDefinitionAcrossTwoFiles)
{
  expect_one("neg_duplicate_definition", "T-2.duplicate", "l_mem");

  // both sites have to be named, R-5
  auto drv = Fixture::run_neg("neg_duplicate_definition");
  ASSERT_EQ(size_t(1), drv->diags().size());
  EXPECT_NE(std::string::npos,
            drv->diags().all()[0].message().find("base_links.json"));
}

// --------------------------------------------------------------------
// T-3 and T-4, port typing on an edge
// --------------------------------------------------------------------
TEST(Negative, PortTypeMismatchOnAnEdge)
{
  expect_one("neg_port_type_mismatch", "T-3.port_type", "l_core");
}

TEST(Negative, PortRoleMismatchAgainstEdgeDirection)
{
  expect_one("neg_port_role_mismatch", "T-4.port_role", "master");
}

// --------------------------------------------------------------------
// T-9, the link is on the interface, both ends must name the same one
// --------------------------------------------------------------------
TEST(Negative, EdgeEndsDisagreeOnTheLink)
{
  expect_one("neg_link_disagree", "T-9.link_agree", "l_core");
}

// --------------------------------------------------------------------
// T-5, the graph
// --------------------------------------------------------------------
TEST(Negative, GraphDoesNotTerminate)
{
  expect_one("neg_no_terminate", "T-5.cycle", "l1 -> l1");
}

// --------------------------------------------------------------------
// T-6, field groups
// --------------------------------------------------------------------
TEST(Negative, PartlyPopulatedFieldGroup)
{
  expect_one("neg_partial_group", "T-6.group", "replacement");
}

// --------------------------------------------------------------------
// the loader
// --------------------------------------------------------------------
TEST(Negative, IncludeCycle)
{
  expect_one("neg_include_cycle", "load.include_cycle", "cyc_b.json");
}

TEST(Negative, IncludeTypeMismatch)
{
  expect_one("neg_include_type_mismatch", "load.include_type",
             "mm_ports.json");
}

// --------------------------------------------------------------------
// the schemas
// --------------------------------------------------------------------
TEST(Negative, SchemaViolation)
{
  expect_one("neg_schema_violation", "schema.violation", "bogus");
}

// --------------------------------------------------------------------
// T-8, the arithmetic
// --------------------------------------------------------------------
TEST(Negative, NonPowerOfTwoCapacity)
{
  expect_one("neg_capacity_not_pow2", "T-8.capacity_pow2", "12288");
}

TEST(Negative, BanksDoesNotDivideTheSetCount)
{
  expect_one("neg_banks_not_divide", "T-8.bank_divide", "3 banks");
}

// A VIPT way wider than a page puts an index bit above the page
// offset, so one physical line can land in two sets.
TEST(Negative, ViptWayWiderThanAPage)
{
  expect_one("neg_vipt_alias", "T-8.vipt_index", "8192");
}
