// --------------------------------------------------------------------
// FILE:    rtl_agent.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "rtl_agent.h"
#include "rtl_cache.h"
#include "rtl_pkg.h"

namespace cgen
{

namespace {
std::string i2s(int v) { return std::to_string(v); }
std::string pad(const std::string &s, size_t w)
{
  std::string o = s;
  while(o.size() < w) o += ' ';
  return o;
}
}

// --------------------------------------------------------------------
// The command port. The same six signals whatever link the agent
// drives, so a testbench task written once works on every agent.
// --------------------------------------------------------------------
std::vector<std::string> RtlAgent::cmd_ports(const NodeCtx &c,
                                             const std::string &p,
                                             bool last)
{
  const NodeCtx::Iface &i = c.ifaces()[0];
  const int w = i.sig.data_bits();

  std::vector<std::string> o = {
    "  input  logic        " + p + "_go,",
    "  input  logic        " + p + "_go_write,",
    "  input  logic [" + pad(i2s(i.sig.addr_bits() - 1) + ":0]", 8) +
        " " + p + "_go_addr,",
    "  input  logic [" + pad(i2s(w - 1) + ":0]", 8) + " " + p +
        "_go_wdata,",
    "  input  logic [" + pad(i2s(w / 8 - 1) + ":0]", 8) + " " + p +
        "_go_wstrb,",
    "  output logic        " + p + "_busy,",
    "  output logic        " + p + "_done,",
    "  output logic [" + pad(i2s(w - 1) + ":0]", 8) + " " + p +
        "_rdata"
  };
  if(!last) o.back() += ",";
  return o;
}

// --------------------------------------------------------------------
std::vector<std::string> RtlAgent::cmd_wires(const NodeCtx &c,
                                             const std::string &p)
{
  const NodeCtx::Iface &i = c.ifaces()[0];
  const int w = i.sig.data_bits();
  return {
    "  logic        " + p + "_go;",
    "  logic        " + p + "_go_write;",
    "  logic [" + i2s(i.sig.addr_bits() - 1) + ":0] " + p +
        "_go_addr;",
    "  logic [" + i2s(w - 1) + ":0] " + p + "_go_wdata;",
    "  logic [" + i2s(w / 8 - 1) + ":0] " + p + "_go_wstrb;",
    "  logic        " + p + "_busy;",
    "  logic        " + p + "_done;",
    "  logic [" + i2s(w - 1) + ":0] " + p + "_rdata;"
  };
}

// --------------------------------------------------------------------
void RtlAgent::top(SvFile &f, const NodeCtx &c)
{
  const NodeCtx::Iface &i = c.ifaces()[0];
  const bool tl = i.sig.is_tl();

  f.note("Node '" + c.name() + "', an agent. NOT SYNTHESIZABLE.");
  f.note("");
  f.note("An agent carries interfaces and nothing else, D-15. In RTL");
  f.note("that is a protocol driver: the testbench presents one");
  f.note("request on the command port and this module runs it over");
  f.note("link '" + i.link + "', protocol " + i.sig.protocol() + ".");
  f.note("");
  f.note("The command port is the same shape on every agent, so one");
  f.note("set of testbench tasks drives all of them.");
  f.bar();
  RtlPkg::import_of(f, tl ? std::vector<std::string>{
                              RtlPkg::tl_pkg_name() }
                          : std::vector<std::string>{});
  f.ln("module " + c.mod() + " (");
  f.ln("  input  logic        clk,");
  f.ln("  input  logic        rstn,");
  f.ln();
  f.ln("  // the command port the testbench drives");
  f.lines(cmd_ports(c, "cmd", false));
  f.ln();
  f.ln("  // interface '" + i.name + "', link '" + i.link + "'");
  f.lines(RtlCache::iface_ports(i, true));
  f.ln(");");
  f.ln();

  if(tl) {
    f.ln("  // A TileLink driving agent is not built by this emitter.");
    f.ln("  // Every agent in this configuration drives the custom");
    f.ln("  // processor port, and a TL agent needs a source id pool");
    f.ln("  // that nothing has asked for yet.");
    f.ln("  initial $fatal(1, \"" + c.mod() +
         ": TileLink agent not built\");");
    f.ln();
    f.ln("endmodule");
    return;
  }

  const std::string n  = i.name;
  const bool rdonly    = i.sig.read_only();
  const int  idb       = i.sig.id_bits();

  f.ln("  typedef enum logic [1:0] {");
  f.ln("    A_IDLE, A_REQ, A_WAIT, A_DONE");
  f.ln("  } astate_e;");
  f.ln();
  f.ln("  astate_e astate;");
  f.ln("  logic    read_q;");
  f.ln();
  f.ln("  assign cmd_busy = (astate != A_IDLE);");
  f.ln("  assign cmd_done = (astate == A_DONE);");
  f.ln();
  f.ln("  assign " + pad(LinkSig::wire(n, "valid"), 18) +
       "= (astate == A_REQ);");
  if(!rdonly) {
    f.ln("  assign " + pad(LinkSig::wire(n, "rw"), 18) +
         "= cmd_go_write;");
  }
  f.ln("  assign " + pad(LinkSig::wire(n, "addr"), 18) +
       "= cmd_go_addr;");
  if(!rdonly) {
    f.ln("  assign " + pad(LinkSig::wire(n, "wdata"), 18) +
         "= cmd_go_wdata;");
    f.ln("  assign " + pad(LinkSig::wire(n, "wstrb"), 18) +
         "= cmd_go_wstrb;");
  }
  if(idb > 0) {
    f.ln();
    f.ln("  // ONE REQUEST AT A TIME, so one identifier is enough. An");
    f.ln("  // agent that presented several would need a free list, "
         "and");
    f.ln("  // the command port it is driven from has no room for one.");
    f.ln("  assign " + pad(LinkSig::wire(n, "id"), 18) + "= '0;");
  }
  for(const LinkSig::Qual &q : i.sig.quals()) {
    f.ln("  // this agent presents no " + q.name + " request");
    f.ln("  assign " + pad(LinkSig::wire(n, q.name), 18) + "= 1'b0;");
  }
  f.ln();
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      astate    <= A_IDLE;");
  f.ln("      read_q    <= 1'b0;");
  f.ln("      cmd_rdata <= '0;");
  f.ln("    end else begin");
  f.ln("      case (astate)");
  f.ln("        A_IDLE: begin");
  f.ln("          if(cmd_go) begin");
  f.ln("            read_q <= !cmd_go_write;");
  f.ln("            astate <= A_REQ;");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        A_REQ: begin");
  f.ln("          if(" + LinkSig::wire(n, "ready") + ") begin");
  f.ln("            astate <= A_WAIT;");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        A_WAIT: begin");
  f.ln("          // a write has no response on this link, "
       "write_response");
  f.ln("          // is false, so it completes as soon as it is taken");
  f.ln("          if(!read_q) begin");
  f.ln("            astate <= A_DONE;");
  f.ln("          end else if(" + LinkSig::wire(n, "rvalid") +
       ") begin");
  f.ln("            cmd_rdata <= " + LinkSig::wire(n, "rdata") + ";");
  f.ln("            astate    <= A_DONE;");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        A_DONE: begin");
  f.ln("          if(!cmd_go) astate <= A_IDLE;");
  f.ln("        end");
  f.ln();
  f.ln("        default: astate <= A_IDLE;");
  f.ln("      endcase");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  // ------------------------------------------------------------------
  // Whatever the link returns and this agent does not read. One
  // outstanding request means the returned identifier says nothing
  // the state machine does not already know.
  // ------------------------------------------------------------------
  {
    std::vector<std::string> unread;
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.m_drives) continue;              // the agent drives it
      if(g.local == "rdata" || g.local == "ready" ||
         g.local == "rvalid") continue;     // consumed above
      unread.push_back(LinkSig::wire(n, g.local));
    }
    if(rdonly) {
      unread.push_back("cmd_go_write");
      unread.push_back("cmd_go_wdata");
      unread.push_back("cmd_go_wstrb");
    }
    if(!unread.empty()) {
      f.ln("  // what the link carries and this agent does not read");
      f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
      f.ln("  wire unused_ag = |{");
      std::string acc;
      for(const std::string &s : unread) {
        if(!acc.empty()) acc += ",\n";
        acc += "      " + s;
      }
      f.ln(acc);
      f.ln("  };");
      f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
      f.ln();
    }
  }

  f.ln("endmodule");
}

} // namespace cgen
