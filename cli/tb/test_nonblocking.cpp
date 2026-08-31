// --------------------------------------------------------------------
// FILE:    test_nonblocking.cpp
// SOURCE:  TOOLS-004
// STATUS:  WORKING
// UPDATED: 2026-08-30
// CONTACT: Jeff Nye
//
// THE NOT BLOCKING CORE PORT, and the three link declarations it
// needed before it could be described at all.
//
// What is asserted here is what the C++ side decides and the emitted
// SystemVerilog cannot check about itself: which modules exist, which
// wires the bundle carries, and that a width the tool derives is not
// the degenerate one Verilator rejects. The BEHAVIOUR of the miss
// handling file is proved by the emitted testbench, in l1i_tests.svh,
// and is not restated here.
//
// Every assertion below is made against the tree the tool wrote in
// this run, never against a transcription of it.
// --------------------------------------------------------------------
#include "fixture.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using cgen::Fixture;

namespace {

// ------------------------------------------------------------------
// One emitted file of the pacino tree. The emission is done once per
// test rather than shared, because a test that depended on another
// test's output would depend on the order they run in.
// ------------------------------------------------------------------
std::string emitted(const std::string &leaf, const std::string &rel)
{
  static int seq = 0;
  const std::string out = Fixture::scratch(leaf + std::to_string(seq++));
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  if(drv->diags().has_error()) return std::string();
  return Fixture::slurp(out + "/" + rel);
}

bool contains(const std::string &body, const std::string &what)
{
  return body.find(what) != std::string::npos;
}

// ------------------------------------------------------------------
// The same, with every run of whitespace squeezed to one space. A
// declaration is padded to a column, and the column is not what any
// of these tests is about.
// ------------------------------------------------------------------
bool declares(const std::string &body, const std::string &what)
{
  std::string flat;
  flat.reserve(body.size());
  bool space = false;
  for(char ch : body) {
    const bool ws = (ch == ' ' || ch == '\t');
    if(ws) { space = true; continue; }
    if(space && !flat.empty()) flat += ' ';
    space = false;
    flat += ch;
  }
  return flat.find(what) != std::string::npos;
}

} // namespace

// --------------------------------------------------------------------
// TD-L1I-7. A READ ONLY LINK. write_width_bits 0 takes the whole
// write channel out of the bundle, so the adapter has no write wires
// to tie off and the node and the link no longer disagree about
// whether one exists.
//
// The check is for the ABSENCE of every write wire. Asserting that
// the adapter compiles would not distinguish a link with no write
// channel from one whose write channel is tied off, which is the
// state this closed.
// --------------------------------------------------------------------
TEST(NonBlocking, AReadOnlyLinkCarriesNoWriteChannel)
{
  const std::string slv = emitted("nb_ro", "l1i/rtl/l1i_core_slv.sv");
  ASSERT_FALSE(slv.empty());

  for(const char *w : { "core_wdata", "core_wstrb", "core_rw" }) {
    EXPECT_FALSE(contains(slv, w))
      << "the core adapter still carries " << w
      << ", so the link is not read only";
  }
  EXPECT_FALSE(contains(slv, "unused_wr"))
    << "a write channel is still being tied off";

  // and the same on the requester's side of the same link
  const std::string agent = emitted("nb_ro_ag", "ifu/rtl/ifu.sv");
  ASSERT_FALSE(agent.empty());
  for(const char *w : { "mem_wdata", "mem_wstrb", "mem_rw" }) {
    EXPECT_FALSE(contains(agent, w))
      << "the agent still drives " << w;
  }
}

// --------------------------------------------------------------------
// INFRA-012 E1, FIRST SITE. At a core port as wide as the line,
// WordsPerLine is 1 and $clog2 of it is 0. The declaration
// [WordIdxBits-1:0] is then [-1:0], which Verilator rejects outright.
//
// The tool knows the count, so the degenerate shape is emitted rather
// than computed in the package and discovered at elaboration. The
// assertion is on the SHAPE and not on the lint result, because a
// lint pass proves the file is accepted and not that the reason it is
// accepted is the intended one.
// --------------------------------------------------------------------
TEST(NonBlocking, AWholeLineCorePortEmitsNoNegativeRange)
{
  const std::string pkg = emitted("nb_wi", "l1i/rtl/l1i_pkg.sv");
  ASSERT_FALSE(pkg.empty());

  EXPECT_TRUE(contains(pkg, "L1iWordIdxBits   = 0"))
    << "the word index is not the degenerate zero width one";
  EXPECT_FALSE(contains(pkg, "[L1iWordIdxBits-1:0]"))
    << "a zero width word index is still used as a range, which is "
       "[-1:0]";
  EXPECT_TRUE(contains(pkg, "function automatic logic l1i_word_of"))
    << "word_of was not emitted in its degenerate form";

  // the node that still has several words a line keeps the computed
  // form, so the branch above is a branch and not a rewrite
  const std::string l1d = emitted("nb_wi_d", "l1d/rtl/l1d_pkg.sv");
  ASSERT_FALSE(l1d.empty());
  EXPECT_TRUE(contains(l1d, "$clog2(L1dWordsPerLine)"))
    << "the multi word node lost its computed word index";
}

// --------------------------------------------------------------------
// INFRA-012 E1, SECOND SITE, found by TOOLS-003. The emitted driver
// task sized its write path from write_width_bits and its data
// argument from the read width, so an ASYMMETRIC link truncated 512
// bits into 32 in the testbench even when the RTL was correct.
//
// With no write channel there is nothing to truncate, and the check
// is that the driver names no write wire at all.
// --------------------------------------------------------------------
TEST(NonBlocking, TheDriverTaskDrivesNoWriteWireOnAReadOnlyLink)
{
  const std::string tb = emitted("nb_drv", "l1i/tb/l1i_tb.sv");
  ASSERT_FALSE(tb.empty());

  EXPECT_FALSE(contains(tb, "core_wdata = "))
    << "the driver still assigns write data on a read only link";
  EXPECT_FALSE(contains(tb, "core_wstrb = "))
    << "the driver still assigns write strobes on a read only link";
  EXPECT_TRUE(contains(tb, "core_id    = "))
    << "the driver does not present an identifier";
}

// --------------------------------------------------------------------
// INFRA-012 E4. THE MISS HANDLING FILE EXISTS, and its two counts
// come from the configuration rather than from the emitter. The
// module is what E4 said no node had.
// --------------------------------------------------------------------
TEST(NonBlocking, TheMissHandlingFileIsEmittedAndSizedFromTheInput)
{
  const std::string mshr = emitted("nb_file", "l1i/rtl/l1i_mshr.sv");
  ASSERT_FALSE(mshr.empty()) << "no miss handling file was emitted";

  // the counts reach the package, and the file is declared from them
  const std::string pkg = emitted("nb_file_p", "l1i/rtl/l1i_pkg.sv");
  ASSERT_FALSE(pkg.empty());
  EXPECT_TRUE(declares(pkg, "L1iMshrs = 16"))
    << "the register count did not reach the package";
  EXPECT_TRUE(declares(pkg, "L1iMshrTargets = 4"))
    << "the target count did not reach the package";
  EXPECT_TRUE(declares(pkg, "L1iMaxOutstanding = 16"))
    << "the link's outstanding count did not reach the package";
  EXPECT_TRUE(declares(pkg, "L1iReqIdBits = 4"))
    << "the link's identifier width did not reach the package";

  EXPECT_TRUE(declares(mshr, "e_val [L1iMshrs]"))
    << "the file is not sized from the register count";
  EXPECT_TRUE(declares(mshr, "t_id [L1iMshrs][L1iMshrTargets]"))
    << "the targets are not sized from the target count";

  // A BLOCKING NODE GETS NONE OF IT. l1d declares one outstanding
  // request on its core link, so nothing about it changed.
  const std::string l1d = emitted("nb_file_b", "l1d/rtl/l1d_mshr.sv");
  EXPECT_TRUE(l1d.empty())
    << "a node whose core link is blocking emitted a miss handling "
       "file anyway";
  const std::string l1d_pkg = emitted("nb_file_bp", "l1d/rtl/l1d_pkg.sv");
  ASSERT_FALSE(l1d_pkg.empty());
  EXPECT_FALSE(contains(l1d_pkg, "L1dMshrs"))
    << "a blocking node's package carries a miss handling count";
}

// --------------------------------------------------------------------
// INFRA-012 E2 and E3. outstanding_requests and the identifiers now
// reach emitted logic, so they leave the unconsumed report. That
// report is the tool's own record of what it read, so asserting
// against it is asserting against the mechanism rather than against
// a second copy of it.
// --------------------------------------------------------------------
TEST(NonBlocking, TheOutstandingCountAndTheIdentifierAreConsumed)
{
  const std::string out = Fixture::scratch("nb_consumed");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error())
    << Fixture::codes(drv->diags());

  const cgen::FieldUse &u = drv->field_use();
  const char *want[] = {
    "/links/pe_port_i/custom/outstanding_requests",
    "/links/pe_port_i/custom/id_width_bits",
    "/links/pe_port_i/custom/error_response",
    "/links/pe_port_i/custom/handshake/response_accept",
    // the qualifier is an array, so its LEAVES are what the report
    // enumerates and what has to be covered
    "/links/pe_port_i/custom/request_qualifiers/0/name",
    "/links/pe_port_i/custom/request_qualifiers/0/policy",
    "/links/pe_port_i/custom/request_qualifiers/0/reserve",
    "/caches/l1i/miss_handling/mshrs",
    "/caches/l1i/miss_handling/mshr_targets"
  };

  for(const char *p : want) {
    bool read = false;
    for(const cgen::FieldUse::Leaf &l : u.leaves()) {
      if(l.ptr != p) continue;
      read = u.was_read(l.file, l.ptr);
      break;
    }
    EXPECT_TRUE(read) << p << " is declared and no stage reads it";
  }
}

// --------------------------------------------------------------------
// TD-L1I-9. THE REQUESTER SUPPLIED QUALIFIER. The bundle carries the
// bit, and the ONE place it is read is the ready path of the miss
// handling file. A second read would be a second rule, and the
// interface says there is exactly one.
// --------------------------------------------------------------------
TEST(NonBlocking, TheQualifierBitIsCarriedAndReadInOnePlace)
{
  const std::string slv = emitted("nb_q", "l1i/rtl/l1i_core_slv.sv");
  const std::string mshr = emitted("nb_q_m", "l1i/rtl/l1i_mshr.sv");
  ASSERT_FALSE(slv.empty());
  ASSERT_FALSE(mshr.empty());

  EXPECT_TRUE(contains(slv, "core_prefetch"))
    << "the qualifier does not reach the adapter";
  EXPECT_TRUE(contains(mshr, "req_prefetch"))
    << "the qualifier does not reach the miss handling file";

  // exactly one read of it, in the ready expression
  size_t reads = 0;
  size_t at = 0;
  while((at = mshr.find("req_prefetch", at)) != std::string::npos) {
    ++reads;
    at += 12;
  }
  // the port declaration, the ready expression, and nothing else
  EXPECT_EQ(size_t(2), reads)
    << "the qualifier is named " << reads
    << " times in the miss handling file, so it is read somewhere "
       "other than ready";
  EXPECT_TRUE(contains(mshr, "wire qual_ok = !req_prefetch ||"))
    << "the qualifier is not read in the ready path";

  // the reserve is the configured one and not a number in the emitter
  const std::string pkg = emitted("nb_q_p", "l1i/rtl/l1i_pkg.sv");
  ASSERT_FALSE(pkg.empty());
  EXPECT_TRUE(declares(pkg, "L1iQualReserve = 2"))
    << "the reserve did not come from the configuration";
}

// --------------------------------------------------------------------
// TD-IF-2. THE ERROR RETURN. The adapter used to carry a comment
// saying the link could not declare one and tie the signal into an
// unused net. It now drives it.
// --------------------------------------------------------------------
TEST(NonBlocking, AnErrorReturnIsDrivenRatherThanDropped)
{
  const std::string slv = emitted("nb_err", "l1i/rtl/l1i_core_slv.sv");
  ASSERT_FALSE(slv.empty());

  EXPECT_TRUE(contains(slv, "assign core_rerr"))
    << "the error return is not driven";
  EXPECT_FALSE(contains(slv, "unused_err"))
    << "the error is still being dropped into an unused net";

  // the link that does NOT declare one still drops it, so the branch
  // above is the declaration doing the work
  const std::string l1d = emitted("nb_err_d", "l1d/rtl/l1d_core_slv.sv");
  ASSERT_FALSE(l1d.empty());
  EXPECT_TRUE(contains(l1d, "unused_err"))
    << "a link with no error return stopped dropping the error";
}

// --------------------------------------------------------------------
// The memory side names the requester. mem_a_source was tied to zero
// on every node, so a response could not be matched to the fill that
// asked for it even in principle.
//
// It is no longer LATCHED from one in flight register, because there
// is no longer one: the miss handling file offers a fill and its own
// index rides channel A with it. The assertion moved with the design
// and did not lapse.
// --------------------------------------------------------------------
TEST(NonBlocking, TheMemorySideSourceComesFromTheInFlightState)
{
  const std::string mst = emitted("nb_src", "l1i/rtl/l1i_mem_mst.sv");
  ASSERT_FALSE(mst.empty());

  EXPECT_FALSE(contains(mst, "mem_a_source  = '0"))
    << "the memory side source is still tied to zero";
  EXPECT_TRUE(contains(mst, "mem_a_source  = 4'(mreq_src)"))
    << "the source is not the register the fill belongs to";

  const std::string top = emitted("nb_src_t", "l1i/rtl/l1i.sv");
  ASSERT_FALSE(top.empty());
  EXPECT_TRUE(contains(top, ".m_req_src   (m_req_src)"))
    << "the file does not drive the source of the master adapter";
  EXPECT_TRUE(contains(top, ".m_rsp_src   (m_rsp_src)"))
    << "the return does not name the register back to the file";
}

// --------------------------------------------------------------------
// The miss handling file REPLACED the slave to bank arbitration. If
// both were emitted, two things would be driving b_req_valid and the
// elaboration would be the only thing to say so.
// --------------------------------------------------------------------
TEST(NonBlocking, TheFileReplacesTheArbitrationRatherThanJoiningIt)
{
  const std::string top = emitted("nb_arb", "l1i/rtl/l1i.sv");
  ASSERT_FALSE(top.empty());

  EXPECT_TRUE(contains(top, "u_mshr ("))
    << "the miss handling file is not instantiated";
  EXPECT_FALSE(contains(top, "b_sel[b]"))
    << "the slave to bank arbitration is still emitted beside it";
  EXPECT_FALSE(contains(top, "b_owner"))
    << "the arbitration's ownership register is still emitted";

  // the blocking node keeps the arbitration it always had
  const std::string l1d = emitted("nb_arb_d", "l1d/rtl/l1d.sv");
  ASSERT_FALSE(l1d.empty());
  EXPECT_TRUE(contains(l1d, "b_owner"))
    << "a blocking node lost its arbitration";
}
