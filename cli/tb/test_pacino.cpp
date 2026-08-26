// --------------------------------------------------------------------
// FILE:    test_pacino.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// The positive fixture, R-11. The pacino configuration is expected to
// produce exactly one diagnostic, the VIPT index budget on l1i.
//
// It produces three. The other two are schema violations against
// planning/schema/system.schema.json, which is stale relative to the
// pacino system file and to the other four schemas. Both the schema
// and the testcase are read only under D-4 and D-6, so the defect is
// asserted here rather than repaired.
//
//   PacinoNoOtherDiagnostics is the assertion R-11 asks for and it
//   FAILS. That failure is the finding, not a tool defect.
//
//   PacinoSystemSchemaIsStale pins the two extra diagnostics so that
//   repairing the schema breaks this file loudly and both tests can
//   be settled together.
// --------------------------------------------------------------------
#include "fixture.h"
#include <gtest/gtest.h>

using cgen::Fixture;

// --------------------------------------------------------------------
// The one diagnostic the configuration is expected to produce.
// --------------------------------------------------------------------
TEST(Pacino, ExpectedViptDiagnostic)
{
  auto drv = Fixture::run(Fixture::pacino());
  const cgen::DiagList &d = drv->diags();

  ASSERT_EQ(size_t(1), d.count_code("T-8.vipt_index"))
    << "diagnostics: " << Fixture::codes(d);

  const std::vector<cgen::Diag> v = d.with_code("T-8.vipt_index");
  const cgen::Diag &x = v[0];
  EXPECT_TRUE(x.is_error());
  EXPECT_NE(std::string::npos, x.file().find("pacino_caches.json"));
  EXPECT_EQ("/caches/l1i/geometry", x.path());
  EXPECT_NE(std::string::npos, x.message().find("l1i"));
  EXPECT_NE(std::string::npos, x.message().find("8192"));
  EXPECT_NE(std::string::npos, x.message().find("4096"));
}

// --------------------------------------------------------------------
// R-11 verbatim. Expected to fail, see the file header.
// --------------------------------------------------------------------
TEST(Pacino, NoOtherDiagnostics)
{
  auto drv = Fixture::run(Fixture::pacino());
  const cgen::DiagList &d = drv->diags();

  EXPECT_EQ(size_t(1), d.size())
    << "the pacino configuration is expected to produce exactly one\n"
    << "diagnostic. It produces " << d.size() << ": "
    << Fixture::codes(d) << "\n"
    << "The extra ones come from planning/schema/system.schema.json,\n"
    << "which is stale. See test_pacino.cpp and the CLI-001 results.";
}

// --------------------------------------------------------------------
// Pin the stale schema so that repairing it is noticed here.
// --------------------------------------------------------------------
TEST(Pacino, SystemSchemaIsStale)
{
  auto drv = Fixture::run(Fixture::pacino());
  const cgen::DiagList &d = drv->diags();

  std::vector<cgen::Diag> v = d.with_code("schema.violation");
  ASSERT_EQ(size_t(2), v.size()) << Fixture::codes(d);

  for(const cgen::Diag &x : v) {
    EXPECT_NE(std::string::npos, x.file().find("pacino_system.json"));
  }
  // the system schema include enum has no ports value
  EXPECT_EQ("/include/0/type", v[0].path());
  // the system schema is pinned at 0.9.0, the file is 0.10.0
  EXPECT_EQ("/schema_version", v[1].path());
}

// --------------------------------------------------------------------
// Nothing else in the configuration fails to bind or to check.
// --------------------------------------------------------------------
TEST(Pacino, EverythingElseResolves)
{
  auto drv = Fixture::run(Fixture::pacino());
  const cgen::DiagList &d = drv->diags();

  EXPECT_EQ(size_t(0), d.count_code("T-1.node_cache"));
  EXPECT_EQ(size_t(0), d.count_code("T-1.edge_endpoint"));
  EXPECT_EQ(size_t(0), d.count_code("T-1.edge_link"));
  EXPECT_EQ(size_t(0), d.count_code("T-1.edge_port"));
  EXPECT_EQ(size_t(0), d.count_code("T-1.port_type"));
  EXPECT_EQ(size_t(0), d.count_code("T-1.link_port_type"));
  EXPECT_EQ(size_t(0), d.count_code("T-2.duplicate"));
  EXPECT_EQ(size_t(0), d.count_code("T-3.port_type"));
  EXPECT_EQ(size_t(0), d.count_code("T-4.port_role"));
  EXPECT_EQ(size_t(0), d.count_code("T-5.cycle"));
  EXPECT_EQ(size_t(0), d.count_code("T-6.group"));

  const cgen::Model &m = drv->model();
  EXPECT_EQ(size_t(6), m.nodes.size());
  EXPECT_EQ(size_t(5), m.edges.size());
  EXPECT_EQ(32,   m.pa_bits);
  EXPECT_EQ(39,   m.va_bits);
  EXPECT_EQ(4096, m.page_bytes);

  for(const cgen::Model::Node &n : m.nodes) EXPECT_TRUE(n.resolved);
  for(const cgen::Model::Edge &e : m.edges) {
    EXPECT_TRUE(e.from_ok);
    EXPECT_TRUE(e.to_ok);
    EXPECT_TRUE(e.link_ok);
  }
}
