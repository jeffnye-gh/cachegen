// --------------------------------------------------------------------
// FILE:    test_geometry.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// The derivation of R-8, checked against the pacino nodes. Every
// value here is computed by the tool, none is read from the input.
// --------------------------------------------------------------------
#include "fixture.h"
#include "geometry.h"
#include <gtest/gtest.h>

using cgen::Fixture;
using cgen::Geometry;
using cgen::Model;

namespace {

const Model::Node *node_of(const Model &m, const char *n)
{
  return m.node(n);
}

} // namespace

// --------------------------------------------------------------------
TEST(GeometryMath, PowersOfTwo)
{
  EXPECT_TRUE (Geometry::is_pow2(1));
  EXPECT_TRUE (Geometry::is_pow2(4096));
  EXPECT_FALSE(Geometry::is_pow2(0));
  EXPECT_FALSE(Geometry::is_pow2(12288));

  EXPECT_EQ( 0, Geometry::log2_exact(1));
  EXPECT_EQ( 6, Geometry::log2_exact(64));
  EXPECT_EQ(24, Geometry::log2_exact(16777216));
  EXPECT_EQ(-1, Geometry::log2_exact(3));
}

// --------------------------------------------------------------------
TEST(GeometryMath, FieldDecomposition)
{
  Model::Field f = Geometry::make_field(6, 7);
  EXPECT_TRUE(f.valid);
  EXPECT_EQ(6,  f.lsb);
  EXPECT_EQ(12, f.msb);
  EXPECT_EQ(7,  f.bits);
  EXPECT_EQ(6,  f.shift);
  EXPECT_EQ(uint64_t(0x1fc0), f.mask);

  Model::Field z = Geometry::make_field(0, 0);
  EXPECT_FALSE(z.valid);
  EXPECT_EQ(uint64_t(0), z.mask);
}

// --------------------------------------------------------------------
// 32KB, 4 way, 64 byte line, against pa_bits 32 and a 4KB page
// --------------------------------------------------------------------
TEST(GeometryPacino, L1iDerivation)
{
  auto drv = Fixture::run(Fixture::pacino());
  const Model::Node *n = node_of(drv->model(), "l1i");
  ASSERT_NE(nullptr, n);

  const Model::Geom &g = n->geom;
  ASSERT_TRUE(g.valid);

  EXPECT_EQ(uint64_t(128),  g.sets);
  EXPECT_EQ(uint64_t(128),  g.sets_per_bank);
  EXPECT_EQ(uint64_t(8192), g.bytes_per_way);
  EXPECT_EQ(6,  g.offset_bits);
  EXPECT_EQ(7,  g.index_bits);
  EXPECT_EQ(19, g.tag_bits);
  EXPECT_EQ(0,  g.bank_bits);
  EXPECT_EQ(32, g.offset_bits + g.index_bits + g.tag_bits);

  EXPECT_EQ(uint64_t(0x0000003f), g.offset.mask);
  EXPECT_EQ(uint64_t(0x00001fc0), g.index.mask);
  EXPECT_EQ(uint64_t(0xffffe000), g.tag.mask);
  EXPECT_EQ(0,  g.offset.shift);
  EXPECT_EQ(6,  g.index.shift);
  EXPECT_EQ(13, g.tag.shift);
  EXPECT_EQ(31, g.tag.msb);

  // 64 byte line over a 32 byte TileLink bus
  EXPECT_EQ(2, g.refill_beats);
}

// --------------------------------------------------------------------
// 512KB, 8 way, 2 banks, PIPT
// --------------------------------------------------------------------
TEST(GeometryPacino, L2Derivation)
{
  auto drv = Fixture::run(Fixture::pacino());
  const Model::Node *n = node_of(drv->model(), "l2");
  ASSERT_NE(nullptr, n);

  const Model::Geom &g = n->geom;
  ASSERT_TRUE(g.valid);

  EXPECT_EQ(uint64_t(1024),  g.sets);
  EXPECT_EQ(uint64_t(512),   g.sets_per_bank);
  EXPECT_EQ(uint64_t(65536), g.bytes_per_way);
  EXPECT_EQ(6,  g.offset_bits);
  EXPECT_EQ(10, g.index_bits);
  EXPECT_EQ(16, g.tag_bits);
  EXPECT_EQ(1,  g.bank_bits);
  EXPECT_EQ(2,  g.refill_beats);
}

// --------------------------------------------------------------------
// The memory node terminates the graph, so it has no beat count.
// --------------------------------------------------------------------
TEST(GeometryPacino, MemoryIsTerminal)
{
  auto drv = Fixture::run(Fixture::pacino());
  const Model::Node *n = node_of(drv->model(), "mem");
  ASSERT_NE(nullptr, n);

  const Model::Geom &g = n->geom;
  ASSERT_TRUE(g.valid);

  EXPECT_EQ(uint64_t(16777216), g.sets);
  EXPECT_EQ(uint64_t(2097152),  g.sets_per_bank);
  EXPECT_EQ(2,  g.tag_bits);
  EXPECT_EQ(3,  g.bank_bits);
  EXPECT_EQ(-1, g.refill_beats);
  EXPECT_FALSE(g.refill_note.empty());
}

// --------------------------------------------------------------------
// An agent node carries ports only, there is nothing to derive.
// --------------------------------------------------------------------
TEST(GeometryPacino, AgentHasNoGeometry)
{
  auto drv = Fixture::run(Fixture::pacino());
  const Model::Node *n = node_of(drv->model(), "ifu");
  ASSERT_NE(nullptr, n);

  EXPECT_TRUE(n->resolved);
  EXPECT_EQ("agent", n->cache_type);
  EXPECT_FALSE(n->geom.valid);
}

// --------------------------------------------------------------------
// T-7, D-5 allows a target port to host more than one edge. The l2
// up ports are one edge each here, the count is still recorded.
// --------------------------------------------------------------------
TEST(GeometryPacino, PortOccupancyIsRecorded)
{
  auto drv = Fixture::run(Fixture::pacino());
  const Model &m = drv->model();

  ASSERT_EQ(size_t(1), m.occupancy.count("l2.up_i"));
  ASSERT_EQ(size_t(1), m.occupancy.count("l2.up_d"));
  EXPECT_EQ(1, m.occupancy.at("l2.up_i"));
  EXPECT_EQ(1, m.occupancy.at("l2.up_d"));
  EXPECT_EQ(1, m.occupancy.at("mem.up"));
}
