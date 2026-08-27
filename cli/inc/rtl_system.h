// --------------------------------------------------------------------
// FILE:    rtl_system.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The system top. Instantiates every node in the topology and wires
// every edge, R-5.
//
// One edge becomes one wire bundle. The bundle comes from the link
// the two interfaces name, and T-9 has already established that they
// name the same one, so the two ends are the same list by
// construction and there is nothing here to keep in step.
// --------------------------------------------------------------------
#pragma once
#include "model.h"
#include "node_ctx.h"
#include "sv_file.h"
#include <map>
#include <string>

namespace cgen
{

class RtlSystem
{
public:
  static void top(SvFile &f, const Model &m,
                  const std::map<std::string, NodeCtx> &nodes,
                  const std::string &system_name);

  // the name of the wire bundle an edge becomes
  static std::string edge_wire(const Model::Edge &e);
};

} // namespace cgen
