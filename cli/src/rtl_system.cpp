// --------------------------------------------------------------------
// FILE:    rtl_system.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "rtl_system.h"
#include "rtl_agent.h"
#include "rtl_cache.h"
#include "rtl_pkg.h"

namespace cgen
{

// --------------------------------------------------------------------
std::string RtlSystem::edge_wire(const Model::Edge &e)
{
  if(!e.name.empty()) return "e_" + e.name;
  return "e_" + e.from + "_" + e.to;
}

// --------------------------------------------------------------------
void RtlSystem::top(SvFile &f, const Model &m,
                    const std::map<std::string, NodeCtx> &nodes,
                    const std::string &sys)
{
  f.note("System '" + sys + "'. Every node in the topology and every");
  f.note("edge between them.");
  f.note("");
  f.note(std::to_string(m.nodes.size()) + " nodes, " +
         std::to_string(m.edges.size()) + " edges.");
  f.note("");
  f.note("The agents are drivers and are not synthesizable, so this");
  f.note("top is not synthesizable either. It is the elaboration and");
  f.note("simulation top. The synthesizable part of the design is the");
  f.note("cache nodes, each of which is its own top.");
  f.bar();

  // ------------------------------------------------------------------
  // No import. Every width in this file is a number the tool derived,
  // so the top needs nothing out of a package and cannot pick up a
  // name from one.
  // ------------------------------------------------------------------
  f.ln("// No package is imported here. Every width below is a number");
  f.ln("// cgen derived, so nothing in this file depends on a package");
  f.ln("// and nothing in it can pick up another node's type.");
  f.ln();

  f.ln("module " + sys + "_top (");
  f.ln("  input  logic        clk,");
  f.ln("  input  logic        rstn");

  // agent command ports come out so a testbench can drive them
  std::vector<const NodeCtx *> ag;
  for(const Model::Node &n : m.nodes) {
    auto it = nodes.find(n.name);
    if(it != nodes.end() && it->second.is_agent()) {
      ag.push_back(&it->second);
    }
  }
  for(const NodeCtx *a : ag) {
    f.ln(",");
    f.ln("  // the command port of agent '" + a->name() + "'");
    // cmd_ports already commas every entry but the last
    f.lines(RtlAgent::cmd_ports(*a, a->name(), true));
  }
  f.ln(");");
  f.ln();

  // ------------------------------------------------------------------
  // one wire bundle per edge
  // ------------------------------------------------------------------
  f.ln("  // -------------------------------------------------------");
  f.ln("  // One wire bundle per edge. The bundle is derived from the");
  f.ln("  // link the two interfaces name, and T-9 has already shown");
  f.ln("  // they name the same link, so both ends fit by "
       "construction.");
  f.ln("  // -------------------------------------------------------");
  for(const Model::Edge &e : m.edges) {
    auto it = nodes.find(e.from);
    if(it == nodes.end()) continue;
    const NodeCtx::Iface *fi = nullptr;
    for(const NodeCtx::Iface &x : it->second.ifaces()) {
      if(x.name == e.from_iface) fi = &x;
    }
    if(fi == nullptr) continue;

    f.ln("  // edge '" + (e.name.empty() ? e.from + "->" + e.to
                                         : e.name) + "', " +
         e.from + "." + e.from_iface + " -> " + e.to + "." +
         e.to_iface + ", link '" + e.link + "'");
    f.lines(RtlCache::iface_wires(*fi, edge_wire(e)));
    f.ln();
  }

  // ------------------------------------------------------------------
  // the nodes
  // ------------------------------------------------------------------
  for(const Model::Node &n : m.nodes) {
    auto it = nodes.find(n.name);
    if(it == nodes.end()) continue;
    const NodeCtx &c = it->second;

    f.ln("  // -----------------------------------------------------");
    f.ln("  // node '" + c.name() + "', " + c.type());
    f.ln("  // -----------------------------------------------------");
    f.ln("  " + c.mod() + " u_" + c.name() + " (");
    f.ln("    .clk  (clk),");
    f.ln("    .rstn (rstn),");

    if(c.is_agent()) {
      const std::string p = c.name();
      f.ln("    .cmd_go       (" + p + "_go),");
      f.ln("    .cmd_go_write (" + p + "_go_write),");
      f.ln("    .cmd_go_addr  (" + p + "_go_addr),");
      f.ln("    .cmd_go_wdata (" + p + "_go_wdata),");
      f.ln("    .cmd_go_wstrb (" + p + "_go_wstrb),");
      f.ln("    .cmd_busy     (" + p + "_busy),");
      f.ln("    .cmd_done     (" + p + "_done),");
      f.ln("    .cmd_rdata    (" + p + "_rdata),");
    }

    // every interface joins the edge that lands on it
    const std::vector<NodeCtx::Iface> &ifs = c.ifaces();
    for(size_t k = 0; k < ifs.size(); ++k) {
      std::string bundle;
      for(const Model::Edge &e : m.edges) {
        if(e.from == c.name() && e.from_iface == ifs[k].name) {
          bundle = edge_wire(e);
        }
        if(e.to == c.name() && e.to_iface == ifs[k].name) {
          bundle = edge_wire(e);
        }
      }
      if(bundle.empty()) {
        f.ln("    // interface '" + ifs[k].name + "' has no edge, its");
        f.ln("    // ports are left open");
        continue;
      }
      f.lines(RtlCache::iface_conn(ifs[k], bundle,
                                   k + 1 == ifs.size()));
    }
    f.ln("  );");
    f.ln();
  }

  f.ln("endmodule");
}

} // namespace cgen
