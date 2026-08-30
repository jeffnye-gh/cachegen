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

// The edge names an interface the node definition does not carry.
TEST(Negative, DanglingEdgeInterface)
{
  expect_one("neg_edge_interface", "T-1.edge_interface", "nosuch");
}

// The interface resolves, the port inside it does not.
TEST(Negative, DanglingEdgePort)
{
  expect_one("neg_edge_port", "T-1.edge_port", "nosuch");
}

// A link end names a port type nothing defines. The link is attached
// to no interface, so this is the resolver reaching a definition that
// no edge would have taken it to.
TEST(Negative, DanglingLinkPortType)
{
  expect_one("neg_link_port_type", "T-1.link_port_type", "nosuch");
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
// the addressing block, taken from the first topology file seen
// --------------------------------------------------------------------
TEST(Negative, TwoTopologyFilesDisagreeOnAddressing)
{
  expect_one("neg_addressing_disagree", "topology.addressing",
             "addressing disagrees");
}

// --------------------------------------------------------------------
// T-10, a link address width against the system pa_bits. The fixture
// widens l_core to 40 and leaves the addressing block at 32, so the
// edge that carries l_core into the icache is the one that reports.
// l_mem is untouched and stays silent, which is what makes the count
// exactly one.
// --------------------------------------------------------------------
TEST(Negative, LinkAddressWidthDisagreesWithPaBits)
{
  expect_one("neg_link_addr_width", "T-10.addr_width", "address width");
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

TEST(Negative, IncludeNamesAFileThatIsNotThere)
{
  expect_one("neg_include_missing", "load.open", "missing_ports.json");
}

// The file is named by the diagnostic rather than by the message, so
// the message is matched on what the parser said.
TEST(Negative, IncludedFileIsNotJson)
{
  expect_one("neg_include_parse", "load.parse", "parse error");
}

TEST(Negative, IncludedFileDeclaresNoFileType)
{
  expect_one("neg_include_no_type", "load.type", "file_type");
}

// --------------------------------------------------------------------
// the schemas
// --------------------------------------------------------------------
TEST(Negative, SchemaViolation)
{
  expect_one("neg_schema_violation", "schema.violation", "bogus");
}

// --------------------------------------------------------------------
// A LINK RETURNING valid_with_id AND NAMING NO IDENTIFIER. The two
// fields exist independently and either one alone is meaningless: an
// identifier nothing returns cannot correlate anything, and a
// response keyed by a zero width identifier keys on nothing. The
// schema ties them, so the pair is refused before any stage reads
// either of them.
// --------------------------------------------------------------------
TEST(Negative, ValidWithIdNeedsANonZeroIdentifierWidth)
{
  expect_one("neg_id_width_zero", "schema.violation", "l_core");
}

// --------------------------------------------------------------------
// the schema directory, which is the tool's environment and not the
// configuration. CGEN_SCHEMA_DIR is moved for the run and put back.
// --------------------------------------------------------------------
namespace {

void expect_one_with_schema_dir(const std::string &schema_dir,
                                const std::string &code,
                                const std::string &names)
{
  const std::string base = Fixture::fixture_dir() +
                           "/base/base_system.json";
  auto drv = Fixture::run_with_schema_dir(base, schema_dir);
  const cgen::DiagList &d = drv->diags();

  ASSERT_EQ(size_t(1), d.size()) << schema_dir << " produced "
                                 << Fixture::codes(d);
  EXPECT_EQ(code, d.all()[0].code()) << d.all()[0].format();
  EXPECT_NE(std::string::npos, d.all()[0].message().find(names))
    << d.all()[0].format();
}

} // namespace

// A directory holding no schemas is ignored with a warning, and the
// search falls back to walking up for planning/schema.
TEST(Negative, SchemaDirWithoutTheFiveSchemasIsIgnored)
{
  expect_one_with_schema_dir(Fixture::fixture_dir(), "schema.dir",
                             "does not hold the five schemas");
}

TEST(Negative, SchemaFileIsNotJson)
{
  expect_one_with_schema_dir(Fixture::fixture_dir() +
                             "/schemas_bad_parse",
                             "schema.parse", "parse error");
}

TEST(Negative, SchemaFileIsNotUsable)
{
  expect_one_with_schema_dir(Fixture::fixture_dir() +
                             "/schemas_bad_build",
                             "schema.build", "undefined references");
}

// --------------------------------------------------------------------
// T-8, the arithmetic
// --------------------------------------------------------------------
TEST(Negative, NonPowerOfTwoCapacity)
{
  expect_one("neg_capacity_not_pow2", "T-8.capacity_pow2", "12288");
}

// capacity / (line * ways) leaves a remainder, so there is no set
// count to derive at all.
TEST(Negative, SetCountIsNotAnInteger)
{
  expect_one("neg_sets_not_integer", "T-8.sets_integer", "ways 3");
}

// An integer set count that no index field can address.
TEST(Negative, SetCountIsNotAPowerOfTwo)
{
  expect_one("neg_sets_not_pow2", "T-8.sets_pow2", "192");
}

// offset plus index consumes the whole physical address, so there is
// nothing left to compare a tag against.
TEST(Negative, TagFieldIsEmpty)
{
  expect_one("neg_tag_bits", "T-8.tag_bits", "tag_bits 0");
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
