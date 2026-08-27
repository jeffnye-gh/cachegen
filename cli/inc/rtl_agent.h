// --------------------------------------------------------------------
// FILE:    rtl_agent.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// An agent node. NOT SYNTHESIZABLE. An agent has interfaces and
// nothing else, D-15, and exists so that an edge into an L1 has a
// from. What it is, in RTL, is a protocol driver: the testbench hands
// it one request on plain signals and it runs the link.
//
// The command port is the same shape for every agent whatever link
// it drives, so one testbench task set works on all of them.
// --------------------------------------------------------------------
#pragma once
#include "node_ctx.h"
#include "sv_file.h"

namespace cgen
{

class RtlAgent
{
public:
  static void top(SvFile &f, const NodeCtx &c);

  // the command port a testbench drives, one per agent
  static std::vector<std::string> cmd_ports(const NodeCtx &c,
                                            const std::string &pfx,
                                            bool last);
  static std::vector<std::string> cmd_wires(const NodeCtx &c,
                                            const std::string &pfx);
};

} // namespace cgen
