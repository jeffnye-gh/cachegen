// --------------------------------------------------------------------
// FILE:    rtl_tb.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The testbenches, R-5. A unit testbench for every node, the system
// testbench, and the SV task set both of them include.
//
// The task set is protocol agnostic: counting, timing, reporting and
// checking. It is included INSIDE the module so its tasks see clk
// and rstn by name and nothing has to be passed through a port.
//
// Everything protocol specific is generated per node from the same
// link bundle the RTL was generated from, so a testbench cannot be
// driving a different port list from the one the design has.
// --------------------------------------------------------------------
#pragma once
#include "model.h"
#include "node_ctx.h"
#include "sv_file.h"
#include <map>

namespace cgen
{

class RtlTb
{
public:
  // the shared task set, included by every testbench
  static void tasks(SvFile &f);

  // a TileLink responder for a unit testbench to sit downstream of
  static void tb_mem(SvFile &f, const NodeCtx &c,
                     const NodeCtx::Iface &i);

  // the same thing for a pipelined node, which may have one fill in
  // flight per miss handling register. It holds one per source and
  // lets a test choose the order the beats come back in
  static void tb_mem_pipe(SvFile &f, const NodeCtx &c,
                          const NodeCtx::Iface &i);

  // the same thing for the ad hoc processor port, so an agent has
  // something to talk to in its own unit testbench
  static void tb_slv(SvFile &f, const NodeCtx &c,
                     const NodeCtx::Iface &i);

  // an agent drives rather than answers, so its unit testbench is a
  // different shape from a cache's
  static void agent_tb(SvFile &f, const NodeCtx &c);
  static void agent_tests(SvFile &f, const NodeCtx &c);

  // one node's unit testbench and its tests
  static void unit_tb(SvFile &f, const NodeCtx &c);
  static void unit_tests(SvFile &f, const NodeCtx &c);

  // the system testbench and its tests
  static void sys_tb(SvFile &f, const Model &m,
                     const std::map<std::string, NodeCtx> &nodes,
                     const std::string &sys);
  static void sys_tests(SvFile &f, const Model &m,
                        const std::map<std::string, NodeCtx> &nodes,
                        const std::string &sys);
};

} // namespace cgen
