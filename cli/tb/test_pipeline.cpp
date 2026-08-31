// --------------------------------------------------------------------
// FILE:    test_pipeline.cpp
// SOURCE:  TOOLS-005
// STATUS:  WORKING
// UPDATED: 2026-08-30
// CONTACT: Jeff Nye
//
// THE PIPELINED CONTROL, THE TIMING FIELDS THAT SHAPE IT, AND THE
// FILLS IN FLIGHT BEHIND IT.
//
// What is asserted here is what the C++ side decides and the emitted
// SystemVerilog cannot check about itself: which control was chosen,
// which numbers came out of the configuration, and which
// declarations the emitter refuses. The BEHAVIOUR of the pipeline is
// proved by the emitted testbench, in l1i_tests.svh T11 through T16,
// and is not restated here.
//
// Every assertion is made against the tree the tool wrote in this
// run, never against a transcription of it. Every one of them also
// checks that the BLOCKING node did not get the same treatment, so
// the branch is proved to be a branch.
// --------------------------------------------------------------------
#include "diag_codes.h"
#include "fixture.h"
#include <gtest/gtest.h>
#include <string>

using cgen::DiagCodes;
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

size_t occurrences(const std::string &body, const std::string &what)
{
  size_t n = 0;
  size_t at = body.find(what);
  while(at != std::string::npos) {
    ++n;
    at = body.find(what, at + what.size());
  }
  return n;
}

// ------------------------------------------------------------------
// Emit one negative fixture and assert it produced exactly one
// diagnostic, with the expected code, naming the expected field.
//
// The check runs in the EMIT path, because the pipeline it is about
// is decided there: a node's control is chosen when its context is
// built and the check path never builds one. run_neg would report
// nothing and say so.
// ------------------------------------------------------------------
void expect_one_emit(const std::string &name,
                     const std::string &code,
                     const std::string &names)
{
  const std::string out = Fixture::scratch("neg_" + name);
  auto drv = Fixture::run_emit(
      Fixture::fixture_dir() + "/" + name + "/system.json", out);
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
// L1I-5, hit throughput. The control of a pipelined node is a
// pipeline and not the C_IDLE through C_DONE state machine, and the
// blocking node still has the state machine.
//
// The check is for the ABSENCE of the states. A pipeline emitted
// beside a state machine that still owns req_ready would take one
// access at a time however many stages stood behind it.
// --------------------------------------------------------------------
TEST(Pipeline, ThePipelinedControlIsNotAStateMachine)
{
  const std::string ctl = emitted("pl_ctl", "l1i/rtl/l1i_ctrl.sv");
  ASSERT_FALSE(ctl.empty());

  EXPECT_FALSE(contains(ctl, "C_IDLE"))
    << "the blocking control's states are still emitted";
  EXPECT_FALSE(contains(ctl, "cstate"))
    << "the blocking control's state register is still emitted";
  EXPECT_FALSE(contains(ctl, "req_ready = (cstate"))
    << "ready still comes from a state";

  // the blocking node keeps the control it always had
  const std::string l1d = emitted("pl_ctl_d", "l1d/rtl/l1d_ctrl.sv");
  ASSERT_FALSE(l1d.empty());
  EXPECT_TRUE(contains(l1d, "C_IDLE"))
    << "a blocking node lost its control";
  EXPECT_TRUE(contains(l1d, "assign req_ready = (cstate == C_IDLE);"))
    << "a blocking node's ready stopped coming from its state";
}

// --------------------------------------------------------------------
// E5. read_latency_cycles and tag_compare_stage reached no emitted
// logic. They now shape the pipeline, and the package carries the
// numbers they produced so the emitted testbench can measure against
// the configuration rather than against a literal of its own.
//
// pacino declares read_latency_cycles 2 and tag_compare_stage
// next_cycle against a registered tag read, which is a compare in
// stage 1 and no pad stage.
// --------------------------------------------------------------------
TEST(Pipeline, TheTimingFieldsShapeTheEmittedPipeline)
{
  const std::string pkg = emitted("pl_pkg", "l1i/rtl/l1i_pkg.sv");
  ASSERT_FALSE(pkg.empty());

  EXPECT_TRUE(contains(pkg, "L1iReadLatency"))
    << "the declared latency did not reach the package";
  EXPECT_TRUE(contains(pkg, "L1iCmpStage"))
    << "the compare stage did not reach the package";
  EXPECT_TRUE(contains(pkg, "L1iPipePad"))
    << "the pad depth did not reach the package";

  // the numbers, derived from the two fields and the array read port
  EXPECT_TRUE(contains(pkg, "L1iReadLatency   = 2;"))
    << "the latency is not the one the configuration declares";
  EXPECT_TRUE(contains(pkg, "L1iCmpStage      = 1;"))
    << "tag_compare_stage next_cycle did not put the compare in "
       "stage 1";
  EXPECT_TRUE(contains(pkg, "L1iPipePad       = 0;"))
    << "a latency equal to the minimum asked for a pad stage";

  // a BLOCKING node's package carries none of them, because its
  // control reads neither field
  const std::string l1d = emitted("pl_pkg_d", "l1d/rtl/l1d_pkg.sv");
  ASSERT_FALSE(l1d.empty());
  EXPECT_FALSE(contains(l1d, "ReadLatency"))
    << "a blocking node's package claims a pipeline depth";
}

// --------------------------------------------------------------------
// The pipeline is what the package says it is. A design that carried
// the numbers and ignored them would pass the test above.
// --------------------------------------------------------------------
TEST(Pipeline, TheControlIsBuiltFromThoseNumbersAndNotFromLiterals)
{
  const std::string ctl = emitted("pl_dep", "l1i/rtl/l1i_ctrl.sv");
  ASSERT_FALSE(ctl.empty());

  // the compare stage is a register, because CmpStage is 1
  EXPECT_TRUE(contains(ctl, "c_val  <= accept;"))
    << "the access is not carried to the compare stage";
  EXPECT_TRUE(contains(ctl, "tag_of(c_addr)"))
    << "the compare does not read the stage's own address";

  // and the answer leaves that stage, because PipePad is 0
  EXPECT_TRUE(contains(ctl, "assign rsp_valid = c_hit;"))
    << "the answer is not taken out of the compare stage";
  EXPECT_FALSE(contains(ctl, "a_val [PipePad-1]"))
    << "a pad stage was emitted where the configuration asked for "
       "none";

  // a miss leaves in the same cycle rather than holding the pipeline
  EXPECT_TRUE(contains(ctl, "assign mis_valid = c_miss;"))
    << "a miss does not leave the pipeline in the compare stage";
  EXPECT_TRUE(contains(ctl, "assign req_ready    = 1'b1;"))
    << "something other than the post reset walk takes ready down";
}

// --------------------------------------------------------------------
// L1I-23, sixteen fills in flight. The master adapter is straight
// through on channel A and reassembles channel D per source, so
// nothing in it serialises the fills the file offers.
// --------------------------------------------------------------------
TEST(Pipeline, TheMasterHoldsOneFillPerRegister)
{
  const std::string mst = emitted("pl_mst", "l1i/rtl/l1i_mem_mst.sv");
  ASSERT_FALSE(mst.empty());

  EXPECT_FALSE(contains(mst, "M_IDLE"))
    << "the one fill at a time state machine is still emitted";
  EXPECT_TRUE(contains(mst, "assign mreq_ready = mem_a_ready;"))
    << "channel A is not straight through";
  EXPECT_TRUE(contains(mst, "line_t                  f_buf  [L1iMshrs];"))
    << "there is not one reassembly buffer per register";
  EXPECT_TRUE(contains(mst, "wire l1i_mshr_t d_src  = "
                            "l1i_mshr_t'(mem_d_source);"))
    << "a D beat is not placed by the source it carries";

  // the blocking node's master still holds exactly one
  const std::string l1d = emitted("pl_mst_d", "l1d/rtl/l1d_mem_mst.sv");
  ASSERT_FALSE(l1d.empty());
  EXPECT_TRUE(contains(l1d, "M_IDLE"))
    << "a blocking node's master lost its state machine";
}

// --------------------------------------------------------------------
// A fill is issued by the file, one per cycle, and an issued fill is
// not reissued. Without e_snt the same register would be offered
// every cycle until it came back.
// --------------------------------------------------------------------
TEST(Pipeline, TheFileIssuesOneFillPerCycleAndDoesNotReissue)
{
  const std::string mh = emitted("pl_mshr", "l1i/rtl/l1i_mshr.sv");
  ASSERT_FALSE(mh.empty());

  EXPECT_TRUE(contains(mh, "if(e_val[m-1] && e_mis[m-1] && "
                           "!e_snt[m-1]) begin"))
    << "the fill choice does not skip the fills already sent";
  EXPECT_TRUE(contains(mh, "assign m_req_valid = fis_any;"))
    << "the file does not offer a fill of its own";
  EXPECT_TRUE(contains(mh, "e_snt[fis_sel] <= 1'b1;"))
    << "an issued fill is not marked, so it would be reissued";

  // and a return is placed by the register it names, not by order
  EXPECT_TRUE(contains(mh, "if(m_rsp_valid) begin"))
    << "the file does not take a return";
  EXPECT_TRUE(contains(mh, "e_got [m_rsp_src] <= 1'b1;"))
    << "a return is not placed by the register it names";
}

// --------------------------------------------------------------------
// The bank answers a hit combinationally and the file registers it,
// so the last stage of the hit path is the file's. That is what
// makes a hit land exactly ReadLatency cycles after the acceptance
// AND leaves one response scan for hits and fills alike, which is
// IF-13.
// --------------------------------------------------------------------
TEST(Pipeline, OneScanAnswersHitsAndFillsAlike)
{
  const std::string mh = emitted("pl_one", "l1i/rtl/l1i_mshr.sv");
  ASSERT_FALSE(mh.empty());

  EXPECT_EQ(size_t(1), occurrences(mh, "assign rsp_valid"))
    << "the response is driven from more than one place";
  EXPECT_TRUE(contains(mh, "assign rsp_valid = ret_any;"))
    << "the response does not come from the retirement scan";

  // both producers write the same register field
  EXPECT_TRUE(contains(mh, "e_got [b_rsp_src[b]] <= 1'b1;"))
    << "a hit does not land in the register that asked for it";
  EXPECT_TRUE(contains(mh, "e_got [m_rsp_src] <= 1'b1;"))
    << "a fill does not land in the register that asked for it";
}

// --------------------------------------------------------------------
// The lookup leaves in the SAME cycle the request is accepted. A
// register between the port and the arrays would add a cycle the
// declared latency does not have.
// --------------------------------------------------------------------
TEST(Pipeline, TheLookupLeavesInTheCycleTheRequestIsAccepted)
{
  const std::string mh = emitted("pl_iss", "l1i/rtl/l1i_mshr.sv");
  ASSERT_FALSE(mh.empty());

  EXPECT_TRUE(contains(mh, "b_req_valid[b] = accept && !mrg_hit &&"))
    << "the lookup is not driven from the acceptance";
  EXPECT_TRUE(contains(mh, "b_req_addr[b]  = req_addr;"))
    << "the lookup does not carry the address off the port";
  EXPECT_FALSE(contains(mh, "e_iss"))
    << "the lookup still waits on a register that says it went out";
}

// --------------------------------------------------------------------
// T-11. A declared timing the emitter cannot build is REFUSED rather
// than emitted as something that does not match the number.
//
// The two fixtures are the two ends of the supported range: a
// compare asked for in the cycle the array read was issued against a
// registered read port, and a latency shorter than the array read
// plus the compare.
// --------------------------------------------------------------------
TEST(Pipeline, ACompareBeforeItsArrayOutputIsRefused)
{
  expect_one_emit("neg_tag_stage", "T-11.tag_stage",
                  "tag_compare_stage");
}

TEST(Pipeline, ALatencyShorterThanTheLookupIsRefused)
{
  expect_one_emit("neg_read_latency", "T-11.read_latency",
                  "read_latency_cycles");
}

// --------------------------------------------------------------------
// The range the emitter supports is stated in the refusal, not left
// to the reader. A diagnostic that says only that the value is wrong
// does not say which values are right.
// --------------------------------------------------------------------
TEST(Pipeline, TheRefusalStatesTheShortestLatencyThatCanBeBuilt)
{
  const std::string out = Fixture::scratch("neg_range");
  auto drv = Fixture::run_emit(
      Fixture::fixture_dir() + "/neg_read_latency/system.json", out);
  const cgen::DiagList &d = drv->diags();

  ASSERT_EQ(size_t(1), d.size()) << Fixture::codes(d);
  const std::string m = d.all()[0].message();

  EXPECT_NE(std::string::npos, m.find("shortest hit"))
    << "the refusal does not say what the shortest hit is, " << m;
  EXPECT_NE(std::string::npos, m.find("2 cycles"))
    << "the refusal does not name the number, " << m;
}

// --------------------------------------------------------------------
// Both codes are on the one list every emission site draws from, so
// neither reached a diagnostic without being declared first.
// --------------------------------------------------------------------
TEST(Pipeline, BothTimingCodesAreOnTheDiagnosticList)
{
  EXPECT_NE(nullptr, DiagCodes::find("T-11.read_latency"));
  EXPECT_NE(nullptr, DiagCodes::find("T-11.tag_stage"));
}
