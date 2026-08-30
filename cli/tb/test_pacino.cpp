// --------------------------------------------------------------------
// FILE:    test_pacino.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The positive fixture, R-11. The pacino configuration is expected to
// produce no diagnostics at all.
//
// It used to produce one, the VIPT index budget on l1i at 32KB over
// four ways. A way must not exceed a page, so at a 4096 byte page a
// way caps at 4096 bytes and 32KB needs eight of them. l1i is eight
// way and the budget is now met exactly.
//
// The negative side of that check moved to the neg_vipt_alias
// fixture, which is where it belongs.
// --------------------------------------------------------------------
#include "fixture.h"
#include <gtest/gtest.h>

using cgen::Fixture;

// --------------------------------------------------------------------
// The VIPT index budget is met, and met exactly. Every index bit of
// l1i and of l1d sits below the page offset, so no virtual address
// bit that reaches the index is translated and the caches carry no
// synonyms.
// --------------------------------------------------------------------
TEST(Pacino, ViptIndexBudgetIsMet)
{
  auto drv = Fixture::run(Fixture::pacino());
  const cgen::DiagList &d = drv->diags();
  const cgen::Model    &m = drv->model();

  EXPECT_EQ(size_t(0), d.count_code("T-8.vipt_index"))
    << "diagnostics: " << Fixture::codes(d);

  const char *vipt[2] = { "l1i", "l1d" };
  for(const char *name : vipt) {
    const cgen::Model::Node *n = m.node(name);
    ASSERT_NE(nullptr, n) << name;
    EXPECT_EQ("VIPT", n->indexing) << name;
    EXPECT_EQ(uint64_t(m.page_bytes), n->geom.bytes_per_way) << name;
    EXPECT_EQ(12, n->geom.offset_bits + n->geom.index_bits) << name;
  }
}

// --------------------------------------------------------------------
// R-11 verbatim.
// --------------------------------------------------------------------
TEST(Pacino, NoDiagnostics)
{
  auto drv = Fixture::run(Fixture::pacino());
  const cgen::DiagList &d = drv->diags();

  EXPECT_EQ(size_t(0), d.size())
    << "the pacino configuration is expected to be clean. It produces "
    << d.size() << ": " << Fixture::codes(d);
  EXPECT_FALSE(d.has_error());
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
  EXPECT_EQ(size_t(0), d.count_code("T-1.iface_link"));
  EXPECT_EQ(size_t(0), d.count_code("T-1.edge_interface"));
  EXPECT_EQ(size_t(0), d.count_code("T-1.edge_port"));
  EXPECT_EQ(size_t(0), d.count_code("T-1.port_type"));
  EXPECT_EQ(size_t(0), d.count_code("T-1.link_port_type"));
  EXPECT_EQ(size_t(0), d.count_code("T-2.duplicate"));
  EXPECT_EQ(size_t(0), d.count_code("T-3.port_type"));
  EXPECT_EQ(size_t(0), d.count_code("T-4.port_role"));
  EXPECT_EQ(size_t(0), d.count_code("T-5.cycle"));
  EXPECT_EQ(size_t(0), d.count_code("T-6.group"));
  EXPECT_EQ(size_t(0), d.count_code("T-9.link_agree"));
  EXPECT_EQ(size_t(0), d.count_code("T-10.addr_width"));
  EXPECT_EQ(size_t(0), d.count_code("T-8.vipt_index"));

  const cgen::Model &m = drv->model();
  EXPECT_EQ(size_t(6), m.nodes.size());
  EXPECT_EQ(size_t(5), m.edges.size());
  EXPECT_EQ(36,   m.pa_bits);
  EXPECT_EQ(39,   m.va_bits);
  EXPECT_EQ(4096, m.page_bytes);

  for(const cgen::Model::Node &n : m.nodes) EXPECT_TRUE(n.resolved);
  for(const cgen::Model::Edge &e : m.edges) {
    EXPECT_TRUE(e.from_ok);
    EXPECT_TRUE(e.to_ok);
    EXPECT_TRUE(e.link_ok);
  }
}
