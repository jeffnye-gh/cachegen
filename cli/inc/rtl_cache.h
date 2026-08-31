// --------------------------------------------------------------------
// FILE:    rtl_cache.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The synthesizable RTL of one cache node, one module per file.
//
//   <n>_<iface>_slv   a slave link to the internal request bundle
//   <n>_<iface>_mst   the internal line request to a master link
//   <n>_meta_array    tag, valid, dirty and replacement state
//   <n>_data_array    line storage
//   <n>_ctrl          the control of one bank
//   <n>_bank          one bank, the arrays and the control
//   <n>_mshr          the miss handling file, on a node whose core
//                     link is not blocking. It sits between the core
//                     adapter and the banks and owns everything the
//                     blocking control had no place for: what is in
//                     flight, which requester each answer belongs to,
//                     and which requests share one line
//   <n>               the node, the adapters, the banks and the
//                     arbitration between them
//
// The internal request bundle is the same shape for every node, so
// the control does not know which protocol the request arrived on.
// Every protocol difference lives in an adapter.
// --------------------------------------------------------------------
#pragma once
#include "node_ctx.h"
#include "sv_file.h"

namespace cgen
{

class RtlCache
{
public:
  static void slave(SvFile &f, const NodeCtx &c,
                    const NodeCtx::Iface &i);
  static void master(SvFile &f, const NodeCtx &c,
                     const NodeCtx::Iface &i);

  // the master adapter of a pipelined node. It holds one partial
  // line per miss handling register, so many fills may be in flight
  // and their beats may arrive interleaved and out of order.
  static void master_pipe(SvFile &f, const NodeCtx &c,
                          const NodeCtx::Iface &i);
  static void meta_array(SvFile &f, const NodeCtx &c);
  static void data_array(SvFile &f, const NodeCtx &c);
  static void ctrl(SvFile &f, const NodeCtx &c);

  // the pipelined form of the control, on a node the timing
  // configuration and the core link together ask for one. It takes
  // an access every cycle and answers a hit ReadLatency cycles
  // later; the miss handling file owns everything a miss needs.
  static void ctrl_pipe(SvFile &f, const NodeCtx &c);

  static void bank(SvFile &f, const NodeCtx &c);
  static void mshr(SvFile &f, const NodeCtx &c);
  static void top(SvFile &f, const NodeCtx &c);

  // the port list of one interface, from this node's point of view.
  // last is true on the final entry so the comma is right.
  static std::vector<std::string> iface_ports(const NodeCtx::Iface &i,
                                              bool last);

  // wire declarations for one interface bundle, used by the system
  // top and by every testbench
  static std::vector<std::string> iface_wires(const NodeCtx::Iface &i,
                                              const std::string &pfx);

  // the port connections of one interface, .name(pfx_name)
  static std::vector<std::string> iface_conn(const NodeCtx::Iface &i,
                                             const std::string &pfx,
                                             bool last);

  // how many bits it takes to name one of n things, never zero
  static int idx_bits(int n);
};

} // namespace cgen
