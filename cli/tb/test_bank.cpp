// --------------------------------------------------------------------
// FILE:    test_bank.cpp
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
//
// R-7. THE BANK FIELD, RULED. bank_interleave_granularity determines
// the bank position on its own and bank_select_position is deleted.
//
// line means consecutive lines alternate banks, so the select is the
// bits IMMEDIATELY ABOVE THE OFFSET. CLI-004 read the same
// configuration as above_index and put the select at the TOP of the
// index. Both derivations pass the sets_per_bank corroboration,
// because taking N bits from either end of the index leaves the same
// number of bits. The corroboration was never going to separate them
// and the second field is what made them separable at all, which is
// why it had to go rather than be chosen between.
//
// The independent check CLI-004 built stays and is asserted here:
// the set index the field arithmetic leaves and the sets-per-bank the
// division gives are two routes to one number, and geometry refuses
// to resolve the field if they disagree.
// --------------------------------------------------------------------
#include "fixture.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

using cgen::Fixture;
using nlohmann::json;

namespace {

const cgen::Model::Node *node_of(const cgen::Model &m, const char *n)
{
  return m.node(n);
}

// ------------------------------------------------------------------
// The value a package localparam is declared with. The member name
// carries the node prefix and the columns are padded, so the name is
// found and the integer after the next '=' is read, rather than the
// whole line being matched.
// ------------------------------------------------------------------
int localparam_value(const std::string &body, const std::string &name)
{
  size_t at = 0;
  while((at = body.find(name, at)) != std::string::npos) {
    const size_t nl = body.find('\n', at);
    const size_t eq = body.find('=', at);
    if(eq == std::string::npos || (nl != std::string::npos && eq > nl)) {
      at += name.size();
      continue;
    }
    size_t i = eq + 1;
    while(i < body.size() && body[i] == ' ') ++i;
    if(i >= body.size() || body[i] < '0' || body[i] > '9') {
      at += name.size();
      continue;
    }
    int v = 0;
    while(i < body.size() && body[i] >= '0' && body[i] <= '9') {
      v = v * 10 + (body[i] - '0');
      ++i;
    }
    return v;
  }
  return -1;
}

} // namespace

// --------------------------------------------------------------------
// R-7. line granularity puts the select immediately above the offset.
// The bounds are derived rather than written out here: the assertion
// is the RULE, applied to whatever geometry the node declares.
// --------------------------------------------------------------------
TEST(Bank, LineGranularityPutsTheSelectImmediatelyAboveTheOffset)
{
  auto drv = Fixture::run(Fixture::pacino());
  ASSERT_FALSE(drv->diags().has_error())
    << Fixture::codes(drv->diags());

  int banked = 0;
  for(const cgen::Model::Node &n : drv->model().nodes) {
    const cgen::Model::Geom &g = n.geom;
    if(!g.valid || g.banks <= 1)              continue;
    if(g.bank_granularity != "line")          continue;
    ++banked;

    ASSERT_TRUE(g.bank_resolved)
      << n.name << " declares line granularity and the field did not "
      << "resolve: " << g.bank_note;

    // the select sits at the bottom of the index, which is the bit
    // immediately above the offset
    EXPECT_EQ(g.offset_bits, g.bank.lsb)
      << n.name << " bank select does not start at the offset";
    EXPECT_EQ(g.index.lsb, g.bank.lsb)
      << n.name << " bank select is not taken from the index";
    EXPECT_EQ(g.bank_bits, g.bank.bits);

    // and the set index is what is left, above it
    EXPECT_EQ(g.bank.msb + 1, g.set_index.lsb)
      << n.name << " set index does not sit above the bank select";
    EXPECT_EQ(g.index.msb, g.set_index.msb)
      << n.name << " set index does not end where the index does";
    EXPECT_EQ(g.index_bits - g.bank_bits, g.set_index.bits);
  }

  EXPECT_LE(2, banked) << "only " << banked
                       << " banked nodes were derived";
}

// --------------------------------------------------------------------
// R-7. The corroboration CLI-004 built. Two independent routes to the
// same number, and they agree.
// --------------------------------------------------------------------
TEST(Bank, TheSetIndexWidthAgreesWithSetsPerBank)
{
  auto drv = Fixture::run(Fixture::pacino());
  ASSERT_FALSE(drv->diags().has_error());

  for(const cgen::Model::Node &n : drv->model().nodes) {
    const cgen::Model::Geom &g = n.geom;
    if(!g.valid || !g.bank_resolved || g.banks <= 1) continue;

    // sets_per_bank came from sets / banks, the set index width came
    // from the field arithmetic
    EXPECT_EQ(uint64_t(1) << g.set_index.bits, g.sets_per_bank)
      << n.name << " leaves a " << g.set_index.bits
      << " bit set index against " << g.sets_per_bank
      << " sets per bank";

    // and the three fields still account for every address bit
    EXPECT_EQ(drv->model().pa_bits,
              g.offset_bits + g.index_bits + g.tag_bits)
      << n.name << " address fields do not sum to pa_bits";

    // the select is inside the index and nowhere else
    EXPECT_LE(g.index.lsb, g.bank.lsb);
    EXPECT_LE(g.bank.msb, g.index.msb);
  }
}

// --------------------------------------------------------------------
// R-7. l2 explicitly, because it is the node the ruling moved and the
// task file asks for its decomposition in full. This is the one place
// a literal is right: it records what the ruling produces, so a later
// change to the derivation cannot pass silently.
// --------------------------------------------------------------------
TEST(Bank, L2DecomposesAsTheRulingSays)
{
  auto drv = Fixture::run(Fixture::pacino());
  ASSERT_FALSE(drv->diags().has_error());

  const cgen::Model::Node *n = node_of(drv->model(), "l2");
  ASSERT_NE(nullptr, n);
  const cgen::Model::Geom &g = n->geom;
  ASSERT_TRUE(g.valid);
  ASSERT_TRUE(g.bank_resolved) << g.bank_note;

  // 512 KB, 8 ways, 64 B lines, 2 banks, line granularity, 36 bit PA
  EXPECT_EQ(6,  g.offset.bits);  EXPECT_EQ(0,  g.offset.lsb);
  EXPECT_EQ(5,  g.offset.msb);

  EXPECT_EQ(10, g.index.bits);   EXPECT_EQ(6,  g.index.lsb);
  EXPECT_EQ(15, g.index.msb);

  // pa_bits moves the tag and nothing else. The bank select and the
  // set index sit inside the index, which the geometry alone fixes.
  EXPECT_EQ(20, g.tag.bits);     EXPECT_EQ(16, g.tag.lsb);
  EXPECT_EQ(35, g.tag.msb);

  // the ruling. CLI-004 emitted bank [15:15] and setidx [14:6].
  EXPECT_EQ(1, g.bank.bits);     EXPECT_EQ(6,  g.bank.lsb);
  EXPECT_EQ(6,  g.bank.msb);

  EXPECT_EQ(9, g.set_index.bits); EXPECT_EQ(7,  g.set_index.lsb);
  EXPECT_EQ(15, g.set_index.msb);

  EXPECT_EQ(uint64_t(1024), g.sets);
  EXPECT_EQ(uint64_t(512),  g.sets_per_bank);
}

// --------------------------------------------------------------------
// R-7. word granularity remains unresolved and reports why, exactly
// as CLI-004 left it. The path is reached by editing a copy of the
// configuration, so the live derivation is exercised rather than the
// note being taken on trust.
// --------------------------------------------------------------------
TEST(Bank, WordGranularityIsUnresolvedAndSaysWhy)
{
  namespace fs = std::filesystem;

  const fs::path src = fs::path(Fixture::pacino()).parent_path();
  const std::string work = Fixture::scratch("bank_word");

  std::error_code ec;
  fs::copy(src, fs::path(work), fs::copy_options::recursive, ec);
  ASSERT_FALSE(ec) << "could not copy the configuration";

  const std::string caches = work + "/pacino_caches.json";
  json doc;
  { std::ifstream in(caches); ASSERT_TRUE(in.is_open()); in >> doc; }

  const json::json_pointer p(
      "/caches/l2/geometry/bank_interleave_granularity");
  ASSERT_TRUE(doc.contains(p)) << "l2 declares no bank granularity";
  doc[p] = "word";
  { std::ofstream os(caches); os << doc.dump(2) << "\n"; }

  auto drv = Fixture::run(work + "/pacino_system.json");
  ASSERT_FALSE(drv->diags().has_error())
    << "word granularity is not an error, it is unresolved: "
    << Fixture::codes(drv->diags());

  const cgen::Model::Node *n = node_of(drv->model(), "l2");
  ASSERT_NE(nullptr, n);

  EXPECT_FALSE(n->geom.bank_resolved)
    << "word granularity resolved a bank field it cannot determine";
  EXPECT_NE(std::string::npos, n->geom.bank_note.find("word"))
    << "the note does not say why: " << n->geom.bank_note;
  EXPECT_NE(std::string::npos, n->geom.bank_note.find("offset"))
    << "the note does not say where the select goes: "
    << n->geom.bank_note;

  // and the index is left whole, since nothing was taken out of it
  EXPECT_EQ(n->geom.index.bits, n->geom.set_index.bits);
}

// --------------------------------------------------------------------
// R-7. bank_select_position is DELETED. A configuration that still
// carries it is now a schema violation, which is what makes the
// deletion real rather than a field the tool has stopped reading.
// --------------------------------------------------------------------
TEST(Bank, BankSelectPositionIsRejectedBySchema)
{
  namespace fs = std::filesystem;

  const fs::path src = fs::path(Fixture::pacino()).parent_path();
  const std::string work = Fixture::scratch("bank_stale");

  std::error_code ec;
  fs::copy(src, fs::path(work), fs::copy_options::recursive, ec);
  ASSERT_FALSE(ec);

  const std::string caches = work + "/pacino_caches.json";
  json doc;
  { std::ifstream in(caches); ASSERT_TRUE(in.is_open()); in >> doc; }

  doc[json::json_pointer("/caches/l2/geometry/bank_select_position")] =
      "above_index";
  { std::ofstream os(caches); os << doc.dump(2) << "\n"; }

  auto drv = Fixture::run(work + "/pacino_system.json");
  EXPECT_TRUE(drv->diags().has_error())
    << "a deleted field was accepted: "
    << Fixture::codes(drv->diags());
}

// --------------------------------------------------------------------
// R-7. The emitted package carries the derived bounds, so the RTL and
// the derivation cannot drift apart. D-39.
// --------------------------------------------------------------------
TEST(Bank, TheEmittedPackageCarriesTheDerivedBounds)
{
  const std::string out = Fixture::scratch("bank_emit");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  for(const cgen::Model::Node &n : drv->model().nodes) {
    const cgen::Model::Geom &g = n.geom;
    if(!g.valid || g.banks <= 1 || !g.bank_resolved) continue;

    const std::string body =
        Fixture::slurp(out + "/" + n.name + "/rtl/" + n.name +
                       "_pkg.sv");
    ASSERT_FALSE(body.empty()) << n.name << " has no package";

    EXPECT_EQ(g.bank.lsb, localparam_value(body, "BankLsb"))
      << n.name << " package BankLsb is not the derived value";
    EXPECT_EQ(g.bank.msb, localparam_value(body, "BankMsb"))
      << n.name << " package BankMsb is not the derived value";
    EXPECT_EQ(g.set_index.lsb, localparam_value(body, "SetIdxLsb"))
      << n.name << " package SetIdxLsb is not the derived value";
    EXPECT_EQ(g.set_index.msb, localparam_value(body, "SetIdxMsb"))
      << n.name << " package SetIdxMsb is not the derived value";
  }
}
