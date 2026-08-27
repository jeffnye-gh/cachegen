// --------------------------------------------------------------------
// FILE:    rtl_mem.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "rtl_mem.h"
#include "rtl_cache.h"
#include "rtl_pkg.h"

namespace cgen
{

namespace {
std::string i2s(int v) { return std::to_string(v); }
}

// --------------------------------------------------------------------
// The memory has one interface and no internal fabric, so the slave
// adapter IS the memory. It is emitted as its own module anyway, so
// that the file set of a memory node has the same shape as the file
// set of a cache node.
// --------------------------------------------------------------------
void RtlMem::slave(SvFile &f, const NodeCtx &c,
                   const NodeCtx::Iface &i)
{
  const int  dbytes = i.sig.data_bytes();
  const int  shift  = Replacement::log2i(dbytes);
  const bool rc     = c.range_check();

  f.note("The store behind node '" + c.name() + "'.");
  f.note("");
  f.note("NOT SYNTHESIZABLE. The capacity in the configuration is " +
         std::to_string(c.geom().capacity_bytes));
  f.note("bytes, which is not a declarable array. The store is an");
  f.note("associative array keyed by BEAT ADDRESS, so only the beats");
  f.note("a test touches ever exist. R-5 asks for a model sufficient");
  f.note("to elaborate and respond and this is that model.");
  f.note("");
  f.note("Read latency is " + std::to_string(c.read_latency()) +
         " cycles, which the configuration declares.");
  f.note("It is a property of the memory, not simulation control, so");
  f.note("it belongs here and not on a command line. D-41.");
  if(rc) {
    f.note("");
    f.note("range_check is on: an address at or above the capacity is");
    f.note("answered with denied rather than quietly wrapped.");
  }
  f.bar();
  RtlPkg::import_of(f, { c.pkg(), RtlPkg::tl_pkg_name() });
  f.ln("module " + c.mod((i.name + "_slv").c_str()) + " (");
  f.ln("  input  logic  clk,");
  f.ln("  input  logic  rstn,");
  f.ln();
  f.lines(RtlCache::iface_ports(i, true));
  f.ln(");");
  f.ln();

  const std::string a = i.name + "_a";
  const std::string d = i.name + "_d";

  f.ln("  localparam int unsigned BeatShift = " + i2s(shift) + ";");
  f.ln("  localparam int unsigned Latency   = " +
       i2s(c.read_latency()) + ";");
  f.ln("  localparam int unsigned LatBits   =");
  f.ln("      (Latency > 1) ? $clog2(Latency+1) : 1;");
  f.ln();
  f.ln("  // the sparse store, one entry per beat actually touched");
  f.ln("  beat_t store [longint unsigned];");
  f.ln();
  f.ln("  typedef enum logic [2:0] {");
  f.ln("    S_IDLE, S_WDATA, S_WAIT, S_READ, S_ACK");
  f.ln("  } sstate_e;");
  f.ln();
  f.ln("  sstate_e sstate;");
  f.ln("  addr_t   addr_q;");
  f.ln("  logic [2:0] size_q;");
  f.ln("  " + std::string("logic [") +
       i2s(int(i.sig.sigs().size()) > 0 ? 0 : 0) + ":0] unused_pad;");
  f.ln("  logic    write_q;");
  f.ln("  logic    denied_q;");
  f.ln("  logic [15:0] beats_q;");
  f.ln("  logic [15:0] beat_i;");
  f.ln("  logic [LatBits-1:0] lat_q;");
  for(const LinkSig::Sig &g : i.sig.sigs()) {
    if(g.local == "a_source") {
      f.ln("  logic [" + i2s(g.bits - 1) + ":0] source_q;");
    }
  }
  f.ln();
  f.ln("  // how many beats one request of this size is. Derived from");
  f.ln("  // the size field and the link width, never configured.");
  f.ln("  function automatic logic [15:0] beats_of(input logic [2:0] "
       "s);");
  f.ln("    beats_of = (int'(s) > int'(BeatShift))");
  f.ln("             ? 16'(1 << (int'(s) - int'(BeatShift)))");
  f.ln("             : 16'd1;");
  f.ln("  endfunction");
  f.ln();
  f.ln("  function automatic longint unsigned key_of");
  f.ln("      (input addr_t base, input logic [15:0] n);");
  f.ln("    key_of = longint'({32'd0, base}) >> BeatShift;");
  f.ln("    key_of = key_of + longint'({48'd0, n});");
  f.ln("  endfunction");
  f.ln();

  f.ln("  wire a_fire = " + a + "_valid && " + a + "_ready;");
  f.ln("  wire d_fire = " + d + "_valid && " + d + "_ready;");
  f.ln();
  f.ln("  assign " + a + "_ready = (sstate == S_IDLE) ||");
  f.ln("                          (sstate == S_WDATA);");
  f.ln();
  f.ln("  assign " + d + "_valid   = (sstate == S_READ) ||");
  f.ln("                            (sstate == S_ACK);");
  f.ln("  assign " + d + "_opcode  = (sstate == S_READ)");
  f.ln("                            ? TlDAccessAckData : TlDAccessAck;");
  f.ln("  assign " + d + "_param   = TlParamZero;");
  f.ln("  assign " + d + "_size    = size_q;");
  f.ln("  assign " + d + "_source  = source_q;");
  f.ln("  assign " + d + "_sink    = '0;");
  f.ln("  assign " + d + "_denied  = denied_q;");
  f.ln("  assign " + d + "_corrupt = 1'b0;");
  f.ln("  assign " + d + "_data    =");
  f.ln("      store.exists(key_of(addr_q, beat_i))");
  f.ln("      ? store[key_of(addr_q, beat_i)] : '0;");
  f.ln();

  f.ln("  // The store is an associative array, and IEEE 1800-2023");
  f.ln("  // 6.21 forbids a nonblocking assignment to a dynamically");
  f.ln("  // sized variable. Its writes are therefore blocking and");
  f.ln("  // BLKSEQ is turned off across this process alone.");
  f.ln("  /* verilator lint_off BLKSEQ */");
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      sstate   <= S_IDLE;");
  f.ln("      addr_q   <= '0;");
  f.ln("      size_q   <= 3'd0;");
  f.ln("      write_q  <= 1'b0;");
  f.ln("      denied_q <= 1'b0;");
  f.ln("      beats_q  <= 16'd1;");
  f.ln("      beat_i   <= 16'd0;");
  f.ln("      lat_q    <= '0;");
  f.ln("      source_q <= '0;");
  f.ln("      store.delete();");
  f.ln("    end else begin");
  f.ln("      case (sstate)");
  f.ln("        S_IDLE: begin");
  f.ln("          if(a_fire) begin");
  f.ln("            addr_q   <= " + a + "_address;");
  f.ln("            size_q   <= " + a + "_size;");
  f.ln("            source_q <= " + a + "_source;");
  f.ln("            beats_q  <= beats_of(" + a + "_size);");
  f.ln("            beat_i   <= 16'd0;");
  f.ln("            lat_q    <= LatBits'(Latency);");
  if(rc) {
    f.ln("            denied_q <=");
    f.ln("                (" + a + "_address >= "
         "addr_t'(CapacityBytes));");
  } else {
    f.ln("            denied_q <= 1'b0;");
  }
  f.ln("            if((" + a + "_opcode == TlAPutFullData) ||");
  f.ln("               (" + a + "_opcode == TlAPutPartialData)) begin");
  f.ln("              write_q <= 1'b1;");
  f.ln("              store[key_of(" + a + "_address, 16'd0)] =");
  f.ln("                  " + a + "_data;");
  f.ln("              sstate  <= (beats_of(" + a + "_size) > 16'd1)");
  f.ln("                         ? S_WDATA : S_WAIT;");
  f.ln("              beat_i  <= 16'd1;");
  f.ln("            end else begin");
  f.ln("              write_q <= 1'b0;");
  f.ln("              sstate  <= S_WAIT;");
  f.ln("            end");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        S_WDATA: begin");
  f.ln("          if(a_fire) begin");
  f.ln("            store[key_of(addr_q, beat_i)] = " + a + "_data;");
  f.ln("            if(beat_i == beats_q - 16'd1) begin");
  f.ln("              sstate <= S_WAIT;");
  f.ln("              beat_i <= 16'd0;");
  f.ln("            end else begin");
  f.ln("              beat_i <= beat_i + 16'd1;");
  f.ln("            end");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        S_WAIT: begin");
  f.ln("          if(lat_q == '0) begin");
  f.ln("            beat_i <= 16'd0;");
  f.ln("            sstate <= write_q ? S_ACK : S_READ;");
  f.ln("          end else begin");
  f.ln("            lat_q <= LatBits'(lat_q - 1);");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        S_READ: begin");
  f.ln("          if(d_fire) begin");
  f.ln("            if(beat_i == beats_q - 16'd1) begin");
  f.ln("              sstate <= S_IDLE;");
  f.ln("              beat_i <= 16'd0;");
  f.ln("            end else begin");
  f.ln("              beat_i <= beat_i + 16'd1;");
  f.ln("            end");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        S_ACK: begin");
  f.ln("          if(d_fire) sstate <= S_IDLE;");
  f.ln("        end");
  f.ln();
  f.ln("        default: sstate <= S_IDLE;");
  f.ln("      endcase");
  f.ln("    end");
  f.ln("  end");
  f.ln("  /* verilator lint_on BLKSEQ */");
  f.ln();

  if(i.sig.has_bce()) {
    f.ln("  // TL-C channels this memory does not run, tied off, R-5");
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.local.size() < 2 || g.local[1] != '_') continue;
      const char ch = g.local[0];
      if(ch != 'b' && ch != 'c' && ch != 'e') continue;
      if(g.m_drives == i.master) continue;
      f.ln("  assign " + LinkSig::wire(i.name, g.local) + " = " +
           (g.bits == 1 ? "1'b0" : "'0") + ";");
    }
    f.ln();
  }

  f.ln("  // request fields a flat memory does not consume");
  f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
  f.ln("  wire unused_a = |{");
  f.ln("      " + a + "_param,");
  f.ln("      " + a + "_mask,");
  f.ln("      " + a + "_corrupt,");
  f.ln("      unused_pad");
  f.ln("  };");
  f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
  f.ln();
  f.ln("  assign unused_pad = 1'b0;");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
void RtlMem::top(SvFile &f, const NodeCtx &c)
{
  f.note("Node '" + c.name() + "', a memory. NOT SYNTHESIZABLE.");
  f.note("");
  f.note("Capacity " + std::to_string(c.geom().capacity_bytes) +
         " bytes, line " + std::to_string(c.geom().line_bytes) +
         " bytes.");
  f.note("");
  f.note("The configuration gives this node " +
         std::to_string(c.geom().banks) + " banks and a bank select");
  if(c.geom().bank_resolved && c.geom().banks > 1) {
    f.note("at bits [" + std::to_string(c.geom().bank.msb) + ":" +
           std::to_string(c.geom().bank.lsb) + "]. A sparse "
           "behavioural store has no");
    f.note("bank structure to apply it to, so the field is derived "
           "and");
    f.note("carried in the package and this model ignores it. A "
           "banked");
    f.note("memory controller is not what R-5 asks of this node.");
  }
  f.bar();
  RtlPkg::import_of(f, { c.pkg() });
  f.ln("module " + c.mod() + " (");
  f.ln("  input  logic                 clk,");
  f.ln("  input  logic                 rstn,");
  for(size_t k = 0; k < c.ifaces().size(); ++k) {
    f.ln();
    f.ln("  // interface '" + c.ifaces()[k].name + "', link '" +
         c.ifaces()[k].link + "'");
    f.lines(RtlCache::iface_ports(c.ifaces()[k],
                                  k + 1 == c.ifaces().size()));
  }
  f.ln(");");
  f.ln();
  for(const NodeCtx::Iface &i : c.ifaces()) {
    f.ln("  " + c.mod((i.name + "_slv").c_str()) + " u_" + i.name +
         "_slv (");
    f.ln("    .clk  (clk),");
    f.ln("    .rstn (rstn),");
    f.lines(RtlCache::iface_conn(i, i.name, true));
    f.ln("  );");
    f.ln();
  }
  f.ln("endmodule");
}

} // namespace cgen
