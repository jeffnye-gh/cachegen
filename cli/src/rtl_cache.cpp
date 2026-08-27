// --------------------------------------------------------------------
// FILE:    rtl_cache.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "rtl_cache.h"
#include "rtl_pkg.h"

namespace cgen
{

namespace {

std::string i2s(int v) { return std::to_string(v); }

// pad to a column so a port or a connection list lines up
std::string pad(const std::string &s, size_t w)
{
  std::string o = s;
  while(o.size() < w) o += ' ';
  return o;
}

// a port line: direction, type, name
// A port line: direction, type, name, and a comment only when it
// fits. 80 columns is the project width and a decorative comment
// does not get to break it.
std::string port(const char *dir, const std::string &type,
                 const std::string &name, bool last,
                 const std::string &comment = "")
{
  std::string s = "  " + pad(dir, 7) + pad(type, 22) + name;
  if(!last) s += ",";
  if(comment.empty()) return s;

  const std::string with = pad(s, 52) + "// " + comment;
  return with.size() <= 80 ? with : s;
}

std::string typ(int bits)
{
  if(bits <= 1) return "logic";
  return "logic [" + i2s(bits - 1) + ":0]";
}

} // namespace

// --------------------------------------------------------------------
int RtlCache::idx_bits(int n)
{
  int b = 0;
  while((1 << b) < n) ++b;
  return b < 1 ? 1 : b;
}

// --------------------------------------------------------------------
// One interface's ports, from THIS node's point of view. A master end
// drives what the bundle says the master drives; a slave end drives
// the other half. The bundle is the single source, so the two ends
// cannot disagree.
// --------------------------------------------------------------------
std::vector<std::string> RtlCache::iface_ports(const NodeCtx::Iface &i,
                                               bool last)
{
  std::vector<std::string> o;
  const std::vector<LinkSig::Sig> &s = i.sig.sigs();

  for(size_t k = 0; k < s.size(); ++k) {
    const bool drive = (s[k].m_drives == i.master);
    const bool fin   = last && (k + 1 == s.size());
    o.push_back(port(drive ? "output" : "input",
                     typ(s[k].bits),
                     LinkSig::wire(i.name, s[k].local),
                     fin, s[k].comment));
  }
  return o;
}

// --------------------------------------------------------------------
std::vector<std::string> RtlCache::iface_wires(const NodeCtx::Iface &i,
                                               const std::string &pfx)
{
  std::vector<std::string> o;
  for(const LinkSig::Sig &g : i.sig.sigs()) {
    o.push_back("  " + pad(typ(g.bits), 22) +
                LinkSig::wire(pfx, g.local) + ";");
  }
  return o;
}

// --------------------------------------------------------------------
std::vector<std::string> RtlCache::iface_conn(const NodeCtx::Iface &i,
                                              const std::string &pfx,
                                              bool last)
{
  std::vector<std::string> o;
  const std::vector<LinkSig::Sig> &s = i.sig.sigs();
  for(size_t k = 0; k < s.size(); ++k) {
    std::string l = "    ." +
                    pad(LinkSig::wire(i.name, s[k].local), 20) +
                    "(" + LinkSig::wire(pfx, s[k].local) + ")";
    if(!(last && k + 1 == s.size())) l += ",";
    o.push_back(l);
  }
  return o;
}

// --------------------------------------------------------------------
// A slave adapter. One outstanding request, which is what the links
// in this configuration declare and what the control accepts, so
// nothing here needs a queue.
//
// A TileLink request can be WIDER THAN ONE INTERNAL WORD. An L1
// asking its L2 for a line asks for the whole line in one A beat and
// expects the line back as several D beats. The control behind this
// adapter answers one word at a time, so the adapter walks the beats:
// one internal request per beat, one D beat per answer. Getting that
// wrong is a deadlock, not a wrong answer, because the two ends
// disagree about how many beats are owed.
// --------------------------------------------------------------------
void RtlCache::slave(SvFile &f, const NodeCtx &c,
                     const NodeCtx::Iface &i)
{
  const std::string mod = c.mod((i.name + "_slv").c_str());
  const bool tl   = i.sig.is_tl();
  const int  dbit = c.core_data_bits();

  f.note("Interface '" + i.name + "' of node '" + c.name() +
         "', the slave end of");
  f.note("link '" + i.link + "', protocol " + i.sig.protocol() +
         (tl ? " " + i.sig.conformance() : "") + ".");
  f.note("");
  f.note("Turns a request on the link into internal requests and the");
  f.note("internal answers back into a link response. The control");
  f.note("behind it never sees which protocol this was.");
  if(tl) {
    f.note("");
    f.note("A request may be wider than one internal word, so this");
    f.note("adapter walks the beats: one internal request and one D");
    f.note("beat per beat of the request. The beat count comes from");
    f.note("the size field and the link width and is never");
    f.note("configured.");
  }
  f.bar();
  RtlPkg::import_of(f, tl ? std::vector<std::string>{
                              c.pkg(), RtlPkg::tl_pkg_name() }
                          : std::vector<std::string>{ c.pkg() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic                 clk,");
  f.ln("  input  logic                 rstn,");
  f.ln();
  f.ln("  // the link");
  f.lines(iface_ports(i, false));
  f.ln();
  f.ln("  // the internal request bundle");
  f.ln("  output logic                 req_valid,");
  f.ln("  input  logic                 req_ready,");
  f.ln("  output addr_t                req_addr,");
  if(c.has_writes()) {
    f.ln("  output logic                 req_write,");
    f.ln("  output logic [" + pad(i2s(dbit - 1) + ":0]", 14) +
         "req_wdata,");
    f.ln("  output logic [" + pad(i2s(dbit / 8 - 1) + ":0]", 14) +
         "req_wstrb,");
  }
  f.ln();
  f.ln("  // the internal response");
  f.ln("  input  logic                 rsp_valid,");
  f.ln("  input  logic [" + pad(i2s(dbit - 1) + ":0]", 14) +
       "rsp_data,");
  f.ln("  input  logic                 rsp_err");
  f.ln(");");
  f.ln();

  // ------------------------------------------------------------------
  // the ad hoc processor port: one word, one answer, no beats
  // ------------------------------------------------------------------
  if(!tl) {
    const std::string n = i.name;
    f.ln("  // one outstanding request, so the state is a single flag");
    f.ln("  logic busy;");
    f.ln("  logic pend_read;");
    f.ln();
    f.ln("  wire accept = " + LinkSig::wire(n, "valid") + " && " +
         LinkSig::wire(n, "ready") + ";");
    f.ln();
    f.ln("  assign " + pad(LinkSig::wire(n, "ready"), 22) +
         "= req_ready && !busy;");
    f.ln("  assign " + pad("req_valid", 22) + "= " +
         LinkSig::wire(n, "valid") + " && !busy;");
    f.ln("  assign " + pad("req_addr", 22) + "= addr_t'(" +
         LinkSig::wire(n, "addr") + ");");
    if(c.has_writes()) {
      f.ln("  assign " + pad("req_write", 22) + "= " +
           LinkSig::wire(n, "rw") + ";");
      f.ln("  assign " + pad("req_wdata", 22) + "= " +
           LinkSig::wire(n, "wdata") + ";");
      f.ln("  assign " + pad("req_wstrb", 22) + "= " +
           LinkSig::wire(n, "wstrb") + ";");
    }
    f.ln();
    f.ln("  always_ff @(posedge clk or negedge rstn) begin");
    f.ln("    if(!rstn) begin");
    f.ln("      busy      <= 1'b0;");
    f.ln("      pend_read <= 1'b0;");
    f.ln("    end else begin");
    f.ln("      if(accept) begin");
    f.ln("        busy      <= 1'b1;");
    if(c.has_writes()) {
      f.ln("        pend_read <= !" + LinkSig::wire(n, "rw") + ";");
    } else {
      f.ln("        pend_read <= 1'b1;");
    }
    f.ln("      end else if(rsp_valid) begin");
    f.ln("        busy      <= 1'b0;");
    f.ln("        pend_read <= 1'b0;");
    f.ln("      end");
    f.ln("    end");
    f.ln("  end");
    f.ln();
    f.ln("  assign " + pad(LinkSig::wire(n, "rvalid"), 22) +
         "= rsp_valid && pend_read;");
    f.ln("  assign " + pad(LinkSig::wire(n, "rdata"), 22) +
         "= rsp_data;");
    f.ln();
    f.ln("  // the link declares no error return, so a slave error is");
    f.ln("  // dropped here. It reaches a testbench through the "
         "node's");
    f.ln("  // own checking, not through this link.");
    f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
    f.ln("  wire unused_err = |{rsp_err};");
    f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
    if(!c.has_writes()) {
      f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
      f.ln("  wire unused_wr = |{" +
           LinkSig::wire(n, "rw") + ", " +
           LinkSig::wire(n, "wdata") + ", " +
           LinkSig::wire(n, "wstrb") + "};");
      f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
    }
    f.ln();
    f.ln("endmodule");
    return;
  }

  // ------------------------------------------------------------------
  // TileLink, walking the beats
  // ------------------------------------------------------------------
  const std::string n = i.name;
  const std::string a = n + "_a";
  const std::string d = n + "_d";
  const int shift = Replacement::log2i(dbit / 8);

  int src_bits = 0;
  int siz_bits = 3;
  for(const LinkSig::Sig &g : i.sig.sigs()) {
    if(g.local == "a_source") src_bits = g.bits;
    if(g.local == "a_size")   siz_bits = g.bits;
  }

  f.ln("  localparam int unsigned WordShift = " + i2s(shift) +
       ";  // log2 of one internal word in bytes");
  f.ln();
  f.ln("  typedef enum logic [1:0] {");
  f.ln("    S_IDLE, S_REQ, S_RSP, S_WDATA");
  f.ln("  } sstate_e;");
  f.ln();
  f.ln("  sstate_e sstate;");
  f.ln("  addr_t   base_q;");
  f.ln("  logic [" + i2s(siz_bits - 1) + ":0] size_q;");
  if(src_bits > 0) {
    f.ln("  logic [" + i2s(src_bits - 1) + ":0] source_q;");
  }
  f.ln("  logic    write_q;");
  f.ln("  logic    acquire_q;");
  f.ln("  logic [15:0] beats_q;");
  f.ln("  logic [15:0] beat_i;");
  f.ln("  logic [" + i2s(dbit - 1) + ":0] wdata_q;");
  f.ln("  logic [" + i2s(dbit / 8 - 1) + ":0] wstrb_q;");
  f.ln("  logic    rsp_held;");
  f.ln("  logic [" + i2s(dbit - 1) + ":0] rsp_data_q;");
  f.ln("  logic    rsp_err_q;");
  f.ln();
  f.ln("  // how many internal words one request of this size is.");
  f.ln("  // Derived from the size field and the link width, D-37.");
  f.ln("  function automatic logic [15:0] beats_of");
  f.ln("      (input logic [" + i2s(siz_bits - 1) + ":0] s);");
  f.ln("    beats_of = (int'(s) > int'(WordShift))");
  f.ln("             ? 16'(1 << (int'(s) - int'(WordShift)))");
  f.ln("             : 16'd1;");
  f.ln("  endfunction");
  f.ln();
  f.ln("  wire a_fire  = " + a + "_valid && " + a + "_ready;");
  f.ln("  wire d_fire  = " + d + "_valid && " + d + "_ready;");
  f.ln("  wire last_bt = (beat_i == beats_q - 16'd1);");
  f.ln("  wire is_wr   = (" + a + "_opcode == TlAPutFullData) ||");
  f.ln("                 (" + a + "_opcode == TlAPutPartialData);");
  f.ln();
  f.ln("  assign " + a + "_ready = (sstate == S_IDLE) ||");
  f.ln("                          (sstate == S_WDATA);");
  f.ln();
  f.ln("  // one internal request per beat, at that beat's address");
  f.ln("  assign req_valid = (sstate == S_REQ);");
  f.ln("  assign req_addr  = base_q +");
  f.ln("      addr_t'({16'd0, beat_i} << WordShift);");
  if(c.has_writes()) {
    f.ln("  assign req_write = write_q;");
    f.ln("  assign req_wdata = wdata_q;");
    f.ln("  assign req_wstrb = wstrb_q;");
  }
  f.ln();
  f.ln("  // A read answers every beat. A write answers once, on the");
  f.ln("  // last one, because TileLink acknowledges a put with a");
  f.ln("  // single AccessAck however many beats it carried.");
  f.ln("  assign " + d + "_valid = rsp_held &&");
  f.ln("                          (!write_q || last_bt);");
  f.ln();
  f.ln("  always_comb begin");
  f.ln("    " + d + "_opcode = TlDAccessAck;");
  f.ln("    if(acquire_q)     " + d + "_opcode = TlDGrantData;");
  f.ln("    else if(!write_q) " + d + "_opcode = TlDAccessAckData;");
  f.ln("  end");
  f.ln();
  f.ln("  assign " + d + "_param   = acquire_q ? TlCapToT");
  f.ln("                                       : TlParamZero;");
  f.ln("  assign " + d + "_size    = size_q;");
  if(src_bits > 0) {
    f.ln("  assign " + d + "_source  = source_q;");
  }
  f.ln("  assign " + d + "_sink    = '0;");
  f.ln("  assign " + d + "_denied  = rsp_err_q;");
  f.ln("  assign " + d + "_data    = rsp_data_q;");
  f.ln("  assign " + d + "_corrupt = 1'b0;");
  f.ln();
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      sstate     <= S_IDLE;");
  f.ln("      base_q     <= '0;");
  f.ln("      size_q     <= '0;");
  if(src_bits > 0) f.ln("      source_q   <= '0;");
  f.ln("      write_q    <= 1'b0;");
  f.ln("      acquire_q  <= 1'b0;");
  f.ln("      beats_q    <= 16'd1;");
  f.ln("      beat_i     <= 16'd0;");
  f.ln("      wdata_q    <= '0;");
  f.ln("      wstrb_q    <= '0;");
  f.ln("      rsp_held   <= 1'b0;");
  f.ln("      rsp_data_q <= '0;");
  f.ln("      rsp_err_q  <= 1'b0;");
  f.ln("    end else begin");
  f.ln("      case (sstate)");
  f.ln("        S_IDLE: begin");
  f.ln("          if(a_fire) begin");
  f.ln("            base_q    <= " + a + "_address;");
  f.ln("            size_q    <= " + a + "_size;");
  if(src_bits > 0) {
    f.ln("            source_q  <= " + a + "_source;");
  }
  f.ln("            beats_q   <= beats_of(" + a + "_size);");
  f.ln("            beat_i    <= 16'd0;");
  f.ln("            write_q   <= is_wr;");
  f.ln("            acquire_q <=");
  f.ln("                (" + a + "_opcode == TlAAcquireBlock) ||");
  f.ln("                (" + a + "_opcode == TlAAcquirePerm);");
  f.ln("            wdata_q   <= " + a + "_data;");
  f.ln("            wstrb_q   <= " + a + "_mask;");
  f.ln("            rsp_held  <= 1'b0;");
  f.ln("            sstate    <= S_REQ;");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        S_REQ: begin");
  f.ln("          if(req_ready) sstate <= S_RSP;");
  f.ln("        end");
  f.ln();
  f.ln("        S_RSP: begin");
  f.ln("          if(rsp_valid) begin");
  f.ln("            rsp_held   <= 1'b1;");
  f.ln("            rsp_data_q <= rsp_data;");
  f.ln("            rsp_err_q  <= rsp_err;");
  f.ln("          end");
  f.ln();
  f.ln("          // a write beat that is not the last one takes no");
  f.ln("          // D beat, it just goes back for more data");
  f.ln("          if(rsp_held && write_q && !last_bt) begin");
  f.ln("            rsp_held <= 1'b0;");
  f.ln("            beat_i   <= beat_i + 16'd1;");
  f.ln("            sstate   <= S_WDATA;");
  f.ln("          end else if(d_fire) begin");
  f.ln("            rsp_held <= 1'b0;");
  f.ln("            if(last_bt) begin");
  f.ln("              beat_i <= 16'd0;");
  f.ln("              sstate <= S_IDLE;");
  f.ln("            end else begin");
  f.ln("              beat_i <= beat_i + 16'd1;");
  f.ln("              sstate <= S_REQ;");
  f.ln("            end");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        S_WDATA: begin");
  f.ln("          if(a_fire) begin");
  f.ln("            wdata_q <= " + a + "_data;");
  f.ln("            wstrb_q <= " + a + "_mask;");
  f.ln("            sstate  <= S_REQ;");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        default: sstate <= S_IDLE;");
  f.ln("      endcase");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  if(i.sig.has_bce()) {
    f.ln("  // -------------------------------------------------------");
    f.ln("  // TL-C carries B, C and E and this design exercises none");
    f.ln("  // of them: there is no probe path and no release path in");
    f.ln("  // the configuration. R-5 asks for the signalling anyway,");
    f.ln("  // so the channels are emitted and tied off rather than");
    f.ln("  // left out.");
    f.ln("  // -------------------------------------------------------");
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.local.size() < 2 || g.local[1] != '_') continue;
      const char ch = g.local[0];
      if(ch != 'b' && ch != 'c' && ch != 'e') continue;
      if(g.m_drives != i.master) continue;
      f.ln("  assign " + pad(LinkSig::wire(n, g.local), 22) +
           "= " + (g.bits == 1 ? "1'b0" : "'0") + ";");
    }
    f.ln();
    f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
    f.ln("  wire unused_bce = |{");
    std::string acc;
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.local.size() < 2 || g.local[1] != '_') continue;
      const char ch = g.local[0];
      if(ch != 'b' && ch != 'c' && ch != 'e') continue;
      if(g.m_drives == i.master) continue;
      if(!acc.empty()) acc += ",\n";
      acc += "      " + LinkSig::wire(n, g.local);
    }
    if(acc.empty()) acc = "      1'b0";
    f.ln(acc);
    f.ln("  };");
    f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
    f.ln();
  }

  f.ln("  // request fields this adapter does not consume");
  f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
  f.ln("  wire unused_a = |{");
  f.ln("      " + a + "_param,");
  f.ln("      " + a + "_corrupt");
  if(!c.has_writes()) {
    f.ln("      , " + a + "_data");
    f.ln("      , " + a + "_mask");
  }
  f.ln("  };");
  f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
// A master adapter. Turns one whole line request into the beats the
// link carries, and the beats back into one whole line.
//
// Beat count is derived, not configured: the line size divided by the
// link's data width, R-5 and D-37.
// --------------------------------------------------------------------
void RtlCache::master(SvFile &f, const NodeCtx &c,
                      const NodeCtx::Iface &i)
{
  const std::string mod = c.mod((i.name + "_mst").c_str());
  const bool tl   = i.sig.is_tl();
  const bool wr   = c.has_dirty() || c.write_hit() == "write_through" ||
                    c.write_miss() == "no_allocate";
  const int  size = Replacement::log2i(int(c.geom().line_bytes));

  f.note("Interface '" + i.name + "' of node '" + c.name() +
         "', the master end of");
  f.note("link '" + i.link + "', protocol " + i.sig.protocol() +
         (tl ? " " + i.sig.conformance() : "") + ".");
  f.note("");
  f.note("One line is " + std::to_string(c.geom().line_bytes) +
         " bytes and the link carries " +
         std::to_string(i.sig.data_bytes()));
  f.note("bytes a beat, so a line is " +
         std::to_string(c.refill_beats()) +
         " beats. That number is derived");
  f.note("from the two, never read from the configuration, D-37.");
  f.bar();
  RtlPkg::import_of(f, tl ? std::vector<std::string>{
                              c.pkg(), RtlPkg::tl_pkg_name() }
                          : std::vector<std::string>{ c.pkg() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic                 clk,");
  f.ln("  input  logic                 rstn,");
  f.ln();
  f.ln("  // the internal line request");
  f.ln("  input  logic                 mreq_valid,");
  f.ln("  output logic                 mreq_ready,");
  f.ln("  input  addr_t                mreq_addr,");
  if(wr) {
    f.ln("  input  logic                 mreq_write,");
    f.ln("  input  line_t                mreq_wdata,");
  }
  f.ln();
  f.ln("  // the internal line response");
  f.ln("  output logic                 mrsp_valid,");
  f.ln("  output line_t                mrsp_data,");
  f.ln("  output logic                 mrsp_err,");
  f.ln();
  f.ln("  // the link");
  f.lines(iface_ports(i, true));
  f.ln(");");
  f.ln();

  f.ln("  typedef enum logic [1:0] {");
  f.ln("    M_IDLE, M_REQ, M_RSP, M_DONE");
  f.ln("  } mstate_e;");
  f.ln();
  f.ln("  mstate_e mstate;");
  if(wr) f.ln("  logic [BeatIdxBits-1:0] abeat;");
  f.ln("  logic [BeatIdxBits-1:0] dbeat;");
  f.ln("  addr_t addr_q;");
  f.ln("  line_t fill_q;");
  f.ln("  logic  err_q;");
  if(wr) {
    f.ln("  logic  write_q;");
    f.ln("  line_t wdata_q;");
  }
  f.ln();
  f.ln("  localparam logic [2:0] LineSize = 3'd" +
       std::to_string(size) + ";  // log2 of " +
       std::to_string(c.geom().line_bytes) + " bytes");
  f.ln();

  const std::string a = i.name + "_a";
  const std::string d = i.name + "_d";

  f.ln("  wire a_fire = " + a + "_valid && " + a + "_ready;");
  f.ln("  wire d_fire = " + d + "_valid && " + d + "_ready;");
  if(wr) {
    f.ln("  wire last_a = (abeat == BeatIdxBits'(Beats-1));");
  }
  f.ln("  wire last_d = (dbeat == BeatIdxBits'(Beats-1));");
  f.ln();

  f.ln("  assign mreq_ready = (mstate == M_IDLE);");
  f.ln("  assign " + d + "_ready  = (mstate == M_RSP);");
  f.ln();

  f.ln("  // ---------------------------------------------------------");
  f.ln("  // Channel A. A read is one Get for the whole line, a write");
  f.ln("  // is one PutFullData beat per beat of the line.");
  f.ln("  // ---------------------------------------------------------");
  f.ln("  always_comb begin");
  f.ln("    " + a + "_valid   = (mstate == M_REQ);");
  f.ln("    " + a + "_opcode  = TlAGet;");
  f.ln("    " + a + "_param   = TlParamZero;");
  f.ln("    " + a + "_size    = LineSize;");
  f.ln("    " + a + "_source  = '0;");
  f.ln("    " + a + "_address = addr_q;");
  f.ln("    " + a + "_mask    = '0;");
  f.ln("    " + a + "_data    = '0;");
  f.ln("    " + a + "_corrupt = 1'b0;");
  if(wr) {
    f.ln("    if(write_q) begin");
    f.ln("      " + a + "_opcode = TlAPutFullData;");
    f.ln("      " + a + "_mask   = '1;");
    f.ln("      " + a + "_data   =");
    f.ln("          beat_t'(wdata_q >> (int'(abeat) * BeatBits));");
    f.ln("    end");
  }
  f.ln("  end");
  f.ln();

  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      mstate     <= M_IDLE;");
  if(wr) f.ln("      abeat      <= '0;");
  f.ln("      dbeat      <= '0;");
  f.ln("      addr_q     <= '0;");
  f.ln("      fill_q     <= '0;");
  f.ln("      err_q      <= 1'b0;");
  f.ln("      mrsp_valid <= 1'b0;");
  f.ln("      mrsp_data  <= '0;");
  f.ln("      mrsp_err   <= 1'b0;");
  if(wr) {
    f.ln("      write_q    <= 1'b0;");
    f.ln("      wdata_q    <= '0;");
  }
  f.ln("    end else begin");
  f.ln("      mrsp_valid <= 1'b0;");
  f.ln();
  f.ln("      case (mstate)");
  f.ln("        M_IDLE: begin");
  f.ln("          if(mreq_valid) begin");
  f.ln("            addr_q  <= line_base(mreq_addr);");
  if(wr) f.ln("            abeat   <= '0;");
  f.ln("            dbeat   <= '0;");
  f.ln("            err_q   <= 1'b0;");
  if(wr) {
    f.ln("            write_q <= mreq_write;");
    f.ln("            wdata_q <= mreq_wdata;");
  }
  f.ln("            mstate  <= M_REQ;");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        M_REQ: begin");
  f.ln("          if(a_fire) begin");
  if(wr) {
    f.ln("            if(!write_q || last_a) begin");
    f.ln("              mstate <= M_RSP;");
    f.ln("            end else begin");
    f.ln("              abeat  <= abeat + BeatIdxBits'(1);");
    f.ln("            end");
  } else {
    f.ln("            mstate <= M_RSP;");
  }
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        M_RSP: begin");
  f.ln("          if(d_fire) begin");
  f.ln("            if(" + d + "_denied || " + d + "_corrupt) begin");
  f.ln("              err_q <= 1'b1;");
  f.ln("            end");
  if(wr) {
    f.ln("            if(write_q) begin");
    f.ln("              mstate <= M_DONE;");
    f.ln("            end else begin");
    f.ln("              fill_q[int'(dbeat)*BeatBits +: BeatBits] <=");
    f.ln("                  " + d + "_data;");
    f.ln("              if(last_d) mstate <= M_DONE;");
    f.ln("              else       dbeat  <= dbeat + BeatIdxBits'(1);");
    f.ln("            end");
  } else {
    f.ln("            fill_q[int'(dbeat)*BeatBits +: BeatBits] <=");
    f.ln("                " + d + "_data;");
    f.ln("            if(last_d) mstate <= M_DONE;");
    f.ln("            else       dbeat  <= dbeat + BeatIdxBits'(1);");
  }
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        M_DONE: begin");
  f.ln("          mrsp_valid <= 1'b1;");
  f.ln("          mrsp_data  <= fill_q;");
  f.ln("          mrsp_err   <= err_q;");
  f.ln("          mstate     <= M_IDLE;");
  f.ln("        end");
  f.ln();
  f.ln("        default: mstate <= M_IDLE;");
  f.ln("      endcase");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  if(i.sig.has_bce()) {
    f.ln("  // -------------------------------------------------------");
    f.ln("  // TL-C carries B, C and E. This design has no probe path");
    f.ln("  // and no release path, so the channels are emitted and");
    f.ln("  // tied off rather than left out, R-5.");
    f.ln("  // -------------------------------------------------------");
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.local.size() < 2 || g.local[1] != '_') continue;
      const char ch = g.local[0];
      if(ch != 'b' && ch != 'c' && ch != 'e') continue;
      if(g.m_drives != i.master) continue;
      f.ln("  assign " + pad(LinkSig::wire(i.name, g.local), 22) +
           "= " + (g.bits == 1 ? "1'b0" : "'0") + ";");
    }
    f.ln();
    f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
    f.ln("  wire unused_bce = |{");
    std::string acc;
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.local.size() < 2 || g.local[1] != '_') continue;
      const char ch = g.local[0];
      if(ch != 'b' && ch != 'c' && ch != 'e') continue;
      if(g.m_drives == i.master) continue;
      if(!acc.empty()) acc += ",\n";
      acc += "      " + LinkSig::wire(i.name, g.local);
    }
    if(acc.empty()) acc = "      1'b0";
    f.ln(acc);
    f.ln("  };");
    f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
    f.ln();
  }

  f.ln("  // channel D fields a whole line master does not consume");
  f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
  f.ln("  wire unused_d = |{");
  f.ln("      " + d + "_opcode,");
  f.ln("      " + d + "_param,");
  f.ln("      " + d + "_size,");
  f.ln("      " + d + "_source,");
  f.ln("      " + d + "_sink");
  f.ln("  };");
  f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
// The per set metadata: tag, valid, dirty and the replacement state.
//
// The name is meta_array rather than tag_array on purpose. All four
// are addressed by the same set index in the same cycle and they are
// read together, so one module is what they are. Calling it a tag
// array would name a third of its contents.
//
// The four sub arrays declare their own kind and read port. A control
// can only have ONE read latency, so the module presents the slowest:
// if any sub array is registered, every read is registered and the
// combinational ones are registered on the way out. That is stated
// here because nothing in the configuration says it.
// --------------------------------------------------------------------
void RtlCache::meta_array(SvFile &f, const NodeCtx &c)
{
  const std::string mod = c.mod("meta_array");
  const bool dirty = c.has_dirty();
  const bool reg   = c.registered_read("tag")   ||
                     c.registered_read("valid") ||
                     (dirty && c.registered_read("dirty")) ||
                     c.registered_read("repl");
  const bool clr   = c.cleared_on_reset("valid");

  f.note("Metadata of node '" + c.name() + "': tag, valid" +
         (dirty ? ", dirty" : "") + " and the");
  f.note("replacement state, all addressed by the set index.");
  f.note("");
  f.note("Declared storage:");
  f.note("  tag    " + c.array_kind("tag") + ", " +
         (c.registered_read("tag") ? "registered" : "combinational"));
  f.note("  valid  " + c.array_kind("valid") + ", " +
         (c.registered_read("valid") ? "registered" : "combinational") +
         ", " + (clr ? "cleared on reset" : "NOT cleared on reset"));
  if(dirty) {
    f.note("  dirty  " + c.array_kind("dirty") + ", " +
           (c.registered_read("dirty") ? "registered"
                                       : "combinational"));
  }
  f.note("  repl   " + c.array_kind("repl") + ", " +
         (c.registered_read("repl") ? "registered" : "combinational"));
  f.note("");
  f.note("The read port presented is " +
         std::string(reg ? "REGISTERED" : "COMBINATIONAL") +
         ", the slowest of the four.");
  if(!clr) {
    f.note("");
    f.note("The valid bits are NOT cleared on reset, so the control");
    f.note("walks the sets after reset clearing them through inv_en.");
    f.note("That walk is synthesizable; a reset for loop over a memory");
    f.note("of this kind would not be.");
  } else {
    f.note("");
    f.note("The valid bits ARE cleared on reset, which is the non");
    f.note("synthesizable reset clear loop of D-40. It is emitted as");
    f.note("the configuration asks and is correct for a flop file.");
  }
  f.bar();
  RtlPkg::import_of(f, { c.pkg() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic  clk,");
  f.ln("  input  logic  rstn,");
  f.ln();
  f.ln("  // read port, one set, every way");
  f.ln("  input  set_t  rd_set,");
  f.ln("  output tag_t  rd_tag   [Ways],");
  f.ln("  output logic  rd_valid [Ways],");
  if(dirty) f.ln("  output logic  rd_dirty [Ways],");
  f.ln("  output repl_state_t rd_repl,");
  f.ln();
  f.ln("  // allocate one way of one set");
  f.ln("  input  logic  wr_en,");
  f.ln("  input  set_t  wr_set,");
  f.ln("  input  way_t  wr_way,");
  f.ln("  input  tag_t  wr_tag,");
  f.ln("  input  logic  wr_valid,");
  if(dirty) f.ln("  input  logic  wr_dirty,");
  f.ln();
  if(dirty) {
    f.ln("  // set or clear dirty on a way already allocated");
    f.ln("  input  logic  dty_en,");
    f.ln("  input  set_t  dty_set,");
    f.ln("  input  way_t  dty_way,");
    f.ln("  input  logic  dty_val,");
    f.ln();
  }
  f.ln("  // replacement state write back");
  f.ln("  input  logic  rp_en,");
  f.ln("  input  set_t  rp_set,");
  f.ln("  input  repl_state_t rp_val,");
  f.ln();
  f.ln("  // invalidate every way of one set, the post reset walk");
  f.ln("  input  logic  inv_en,");
  f.ln("  input  set_t  inv_set");
  f.ln(");");
  f.ln();

  f.ln("  tag_t        tag_mem  [Ways][SetsPerBank];");
  f.ln("  logic        val_mem  [Ways][SetsPerBank];");
  if(dirty) f.ln("  logic        dty_mem  [Ways][SetsPerBank];");
  f.ln("  repl_state_t repl_mem [SetsPerBank];");
  f.ln();

  // ------------------------------------------------------------------
  // tag write
  // ------------------------------------------------------------------
  f.ln("  always_ff @(posedge clk) begin");
  f.ln("    if(wr_en) tag_mem[wr_way][wr_set] <= wr_tag;");
  f.ln("  end");
  f.ln();

  // ------------------------------------------------------------------
  // valid
  // ------------------------------------------------------------------
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  if(clr) {
    f.ln("      // D-40, the reset clear loop. Correct for a flop");
    f.ln("      // file and not synthesizable for a memory macro.");
    f.ln("      for(int unsigned w = 0; w < Ways; w++) begin");
    f.ln("        for(int unsigned s = 0; s < SetsPerBank; s++) begin");
    f.ln("          val_mem[w][s] <= 1'b0;");
    f.ln("        end");
    f.ln("      end");
  } else {
    f.ln("      // the valid bits are not cleared on reset, the");
    f.ln("      // control walks them through inv_en instead");
    f.ln("      val_mem[0][0] <= val_mem[0][0];");
  }
  f.ln("    end else begin");
  f.ln("      if(inv_en) begin");
  f.ln("        for(int unsigned w = 0; w < Ways; w++) begin");
  f.ln("          val_mem[w][inv_set] <= 1'b0;");
  f.ln("        end");
  f.ln("      end");
  f.ln("      if(wr_en) val_mem[wr_way][wr_set] <= wr_valid;");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  // ------------------------------------------------------------------
  // dirty
  // ------------------------------------------------------------------
  if(dirty) {
    f.ln("  always_ff @(posedge clk or negedge rstn) begin");
    f.ln("    if(!rstn) begin");
    if(c.cleared_on_reset("dirty")) {
      f.ln("      for(int unsigned w = 0; w < Ways; w++) begin");
      f.ln("        for(int unsigned s = 0; s < SetsPerBank; s++) begin");
      f.ln("          dty_mem[w][s] <= 1'b0;");
      f.ln("        end");
      f.ln("      end");
    } else {
      f.ln("      // not cleared on reset, the valid bit gates it");
      f.ln("      dty_mem[0][0] <= dty_mem[0][0];");
    }
    f.ln("    end else begin");
    f.ln("      if(inv_en) begin");
    f.ln("        for(int unsigned w = 0; w < Ways; w++) begin");
    f.ln("          dty_mem[w][inv_set] <= 1'b0;");
    f.ln("        end");
    f.ln("      end");
    f.ln("      if(wr_en)  dty_mem[wr_way][wr_set]   <= wr_dirty;");
    f.ln("      if(dty_en) dty_mem[dty_way][dty_set] <= dty_val;");
    f.ln("    end");
    f.ln("  end");
    f.ln();
  }

  // ------------------------------------------------------------------
  // replacement state
  // ------------------------------------------------------------------
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  if(c.cleared_on_reset("repl")) {
    f.ln("      for(int unsigned s = 0; s < SetsPerBank; s++) begin");
    f.ln("        repl_mem[s] <= ReplReset;");
    f.ln("      end");
  } else {
    f.ln("      // not cleared on reset, the post reset walk seeds it");
    f.ln("      repl_mem[0] <= repl_mem[0];");
  }
  f.ln("    end else begin");
  f.ln("      if(inv_en) repl_mem[inv_set] <= ReplReset;");
  f.ln("      if(rp_en)  repl_mem[rp_set]  <= rp_val;");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  // ------------------------------------------------------------------
  // the read port
  // ------------------------------------------------------------------
  if(reg) {
    f.ln("  always_ff @(posedge clk) begin");
    f.ln("    for(int unsigned w = 0; w < Ways; w++) begin");
    f.ln("      rd_tag[w]   <= tag_mem[w][rd_set];");
    f.ln("      rd_valid[w] <= val_mem[w][rd_set];");
    if(dirty) f.ln("      rd_dirty[w] <= dty_mem[w][rd_set];");
    f.ln("    end");
    f.ln("    rd_repl <= repl_mem[rd_set];");
    f.ln("  end");
  } else {
    f.ln("  always_comb begin");
    f.ln("    for(int unsigned w = 0; w < Ways; w++) begin");
    f.ln("      rd_tag[w]   = tag_mem[w][rd_set];");
    f.ln("      rd_valid[w] = val_mem[w][rd_set];");
    if(dirty) f.ln("      rd_dirty[w] = dty_mem[w][rd_set];");
    f.ln("    end");
    f.ln("    rd_repl = repl_mem[rd_set];");
    f.ln("  end");
  }
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
// The line storage, one line per way per set.
// --------------------------------------------------------------------
void RtlCache::data_array(SvFile &f, const NodeCtx &c)
{
  const std::string mod = c.mod("data_array");
  const bool be  = c.byte_enables("data");
  const bool reg = c.registered_read("data");

  f.note("Line storage of node '" + c.name() + "'. " +
         std::to_string(c.geom().associativity) + " ways of " +
         std::to_string(c.geom().sets_per_bank) + " sets,");
  f.note(std::to_string(c.geom().line_bytes) + " bytes a line.");
  f.note("");
  f.note("Declared " + c.array_kind("data") + ", " +
         (reg ? "registered" : "combinational") + " read, byte " +
         "enables " + (be ? "on" : "off") + ".");
  if(!be) {
    f.note("");
    f.note("With byte enables off the only write is a whole line, "
           "which");
    f.note("is what a fill does. This node takes no partial write.");
  }
  f.bar();
  RtlPkg::import_of(f, { c.pkg() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic  clk,");
  f.ln();
  f.ln("  input  set_t  rd_set,");
  f.ln("  output line_t rd_line [Ways],");
  f.ln();
  f.ln("  input  logic  wr_en,");
  f.ln("  input  set_t  wr_set,");
  f.ln("  input  way_t  wr_way,");
  if(be) {
    f.ln("  input  line_t wr_line,");
    f.ln("  input  logic [LineBytes-1:0] wr_be");
  } else {
    f.ln("  input  line_t wr_line");
  }
  f.ln(");");
  f.ln();
  f.ln("  line_t data_mem [Ways][SetsPerBank];");
  f.ln();
  f.ln("  always_ff @(posedge clk) begin");
  f.ln("    if(wr_en) begin");
  if(be) {
    f.ln("      for(int unsigned b = 0; b < LineBytes; b++) begin");
    f.ln("        if(wr_be[b]) begin");
    f.ln("          data_mem[wr_way][wr_set][b*8 +: 8] <=");
    f.ln("              wr_line[b*8 +: 8];");
    f.ln("        end");
    f.ln("      end");
  } else {
    f.ln("      data_mem[wr_way][wr_set] <= wr_line;");
  }
  f.ln("    end");
  f.ln("  end");
  f.ln();
  if(reg) {
    f.ln("  always_ff @(posedge clk) begin");
    f.ln("    for(int unsigned w = 0; w < Ways; w++) begin");
    f.ln("      rd_line[w] <= data_mem[w][rd_set];");
    f.ln("    end");
    f.ln("  end");
  } else {
    f.ln("  always_comb begin");
    f.ln("    for(int unsigned w = 0; w < Ways; w++) begin");
    f.ln("      rd_line[w] = data_mem[w][rd_set];");
    f.ln("    end");
    f.ln("  end");
  }
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
// The control of one bank. A blocking cache: one request at a time,
// which is what the miss handling in the configuration amounts to
// until an MSHR file is built. mshrs is carried into the package as
// the declared depth and reported, it does not size anything yet.
//
// The states are the whole of the behaviour:
//
//   C_INIT     walk the sets clearing valid, only when the valid
//              bits are not cleared on reset
//   C_IDLE     take a request, present its set to the arrays
//   C_TAG      the arrays have answered, hit or miss is known
//   C_HIT      answer from the array, update replacement
//   C_EVICT    the victim is dirty, write it out
//   C_EVICT_W  wait for the writeback to complete
//   C_FILL     ask for the line
//   C_FILL_W   wait for the line
//   C_DONE     install the line and answer
// --------------------------------------------------------------------
void RtlCache::ctrl(SvFile &f, const NodeCtx &c)
{
  const std::string mod = c.mod("ctrl");
  const bool wr     = c.has_writes();
  const bool dirty  = c.has_dirty();
  const bool be     = c.byte_enables("data");
  const bool banked = c.geom().banks > 1 && c.geom().bank_resolved;
  const bool clr    = c.cleared_on_reset("valid");
  const bool wthru  = c.write_hit() == "write_through";
  const bool ralloc = c.read_miss() != "no_allocate";
  const bool walloc = !wr || c.write_miss() != "no_allocate";
  const bool memwr  = dirty || wthru || (wr && !walloc);

  f.note("Control of one bank of node '" + c.name() + "'.");
  f.note("");
  f.note("Policies this control was built from:");
  f.note("  read miss    " + (c.read_miss().empty()
                              ? std::string("allocate")
                              : c.read_miss()));
  if(wr) {
    f.note("  write miss   " + c.write_miss());
    f.note("  write hit    " + c.write_hit());
  }
  f.note("  replacement  " + c.repl().policy());
  f.note("  mshrs        " + std::to_string(c.mshrs()) +
         " declared, the control is blocking and does not use them");
  f.note("");
  f.note("The victim is the LOWEST NUMBERED INVALID WAY when there is");
  f.note("one, and the replacement victim otherwise. That tie break "
         "is a");
  f.note("generator convention, D-40, and it makes a cold set fill "
         "from");
  f.note("way 0 upward, which is what a testbench can predict.");
  f.bar();
  RtlPkg::import_of(f, { c.pkg() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic  clk,");
  f.ln("  input  logic  rstn,");
  f.ln();
  if(banked) {
    f.ln("  // which bank this is, for the writeback address");
    f.ln("  input  bank_t bank_id,");
    f.ln();
  }
  f.ln("  // the request");
  f.ln("  input  logic  req_valid,");
  f.ln("  output logic  req_ready,");
  f.ln("  input  addr_t req_addr,");
  if(wr) {
    f.ln("  input  logic  req_write,");
    f.ln("  input  word_t req_wdata,");
    f.ln("  input  logic [WordBytes-1:0] req_wstrb,");
  }
  f.ln();
  f.ln("  // the response");
  f.ln("  output logic  rsp_valid,");
  f.ln("  output word_t rsp_data,");
  f.ln("  output logic  rsp_err,");
  f.ln();
  f.ln("  // the metadata array");
  f.ln("  output set_t  meta_rd_set,");
  f.ln("  input  tag_t  meta_rd_tag   [Ways],");
  f.ln("  input  logic  meta_rd_valid [Ways],");
  if(dirty) f.ln("  input  logic  meta_rd_dirty [Ways],");
  f.ln("  input  repl_state_t meta_rd_repl,");
  f.ln("  output logic  meta_wr_en,");
  f.ln("  output set_t  meta_wr_set,");
  f.ln("  output way_t  meta_wr_way,");
  f.ln("  output tag_t  meta_wr_tag,");
  f.ln("  output logic  meta_wr_valid,");
  if(dirty) {
    f.ln("  output logic  meta_wr_dirty,");
    f.ln("  output logic  meta_dty_en,");
    f.ln("  output set_t  meta_dty_set,");
    f.ln("  output way_t  meta_dty_way,");
    f.ln("  output logic  meta_dty_val,");
  }
  f.ln("  output logic  meta_rp_en,");
  f.ln("  output set_t  meta_rp_set,");
  f.ln("  output repl_state_t meta_rp_val,");
  f.ln("  output logic  meta_inv_en,");
  f.ln("  output set_t  meta_inv_set,");
  f.ln();
  f.ln("  // the data array");
  f.ln("  output set_t  data_rd_set,");
  f.ln("  input  line_t data_rd_line [Ways],");
  f.ln("  output logic  data_wr_en,");
  f.ln("  output set_t  data_wr_set,");
  f.ln("  output way_t  data_wr_way,");
  f.ln("  output line_t data_wr_line,");
  if(be) f.ln("  output logic [LineBytes-1:0] data_wr_be,");
  f.ln();
  f.ln("  // the memory side");
  f.ln("  output logic  mreq_valid,");
  f.ln("  input  logic  mreq_ready,");
  f.ln("  output addr_t mreq_addr,");
  if(memwr) {
    f.ln("  output logic  mreq_write,");
    f.ln("  output line_t mreq_wdata,");
  }
  f.ln("  input  logic  mrsp_valid,");
  f.ln("  input  line_t mrsp_data,");
  f.ln("  input  logic  mrsp_err");
  f.ln(");");
  f.ln();

  f.ln("  typedef enum logic [3:0] {");
  f.ln("    C_INIT, C_IDLE, C_TAG, C_HIT, C_EVICT,");
  f.ln("    C_EVICT_W, C_FILL, C_FILL_W, C_DONE");
  f.ln("  } cstate_e;");
  f.ln();
  f.ln("  cstate_e cstate, cnext;");
  f.ln();
  f.ln("  addr_t addr_q;");
  f.ln("  line_t fill_q;");
  f.ln("  way_t  sel_way;");
  f.ln("  logic  err_q;");
  f.ln("  set_t  init_set;");
  if(wr) {
    f.ln("  logic  write_q;");
    f.ln("  word_t wdata_q;");
    f.ln("  logic [WordBytes-1:0] wstrb_q;");
  }
  f.ln();

  // ------------------------------------------------------------------
  // hit detection
  // ------------------------------------------------------------------
  f.ln("  // ---------------------------------------------------------");
  f.ln("  // Hit, and which way hit. One comparator per way, which is");
  f.ln("  // what way_access " + c.array_kind("tag") +
       " parallel lookup asks for.");
  f.ln("  // ---------------------------------------------------------");
  f.ln("  logic [Ways-1:0] hitv;");
  f.ln("  logic            hit;");
  f.ln("  way_t            hit_way;");
  f.ln();
  f.ln("  always_comb begin");
  f.ln("    hitv = '0;");
  f.ln("    for(int unsigned w = 0; w < Ways; w++) begin");
  f.ln("      if(meta_rd_valid[w] &&");
  f.ln("         (meta_rd_tag[w] == tag_of(addr_q))) begin");
  f.ln("        hitv[w] = 1'b1;");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();
  f.ln("  always_comb begin");
  f.ln("    hit     = |hitv;");
  f.ln("    hit_way = '0;");
  f.ln("    for(int unsigned w = 0; w < Ways; w++) begin");
  f.ln("      if(hitv[w]) hit_way = way_t'(w);");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  // ------------------------------------------------------------------
  // victim selection
  // ------------------------------------------------------------------
  f.ln("  // ---------------------------------------------------------");
  f.ln("  // The victim. An invalid way is always taken first, and the");
  f.ln("  // LOWEST numbered one wins the tie. Only when every way is");
  f.ln("  // valid does the replacement state decide. D-40.");
  f.ln("  // ---------------------------------------------------------");
  f.ln("  logic any_inv;");
  f.ln("  way_t inv_way;");
  f.ln("  way_t victim;");
  f.ln();
  f.ln("  always_comb begin");
  f.ln("    any_inv = 1'b0;");
  f.ln("    inv_way = '0;");
  f.ln("    // counting down leaves the lowest numbered invalid way");
  f.ln("    for(int unsigned w = Ways; w > 0; w--) begin");
  f.ln("      if(!meta_rd_valid[w-1]) begin");
  f.ln("        any_inv = 1'b1;");
  f.ln("        inv_way = way_t'(w-1);");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();
  f.ln("  assign victim = any_inv ? inv_way : repl_victim(meta_rd_repl);");
  f.ln();
  if(dirty) {
    f.ln("  wire evict_needed = meta_rd_valid[victim] &&");
    f.ln("                      meta_rd_dirty[victim] && !any_inv;");
  } else {
    f.ln("  // nothing in this node is ever dirty, so nothing is ever");
    f.ln("  // written back and the evict states are never entered.");
    f.ln("  wire evict_needed = 1'b0;");
  }
  f.ln();

  // ------------------------------------------------------------------
  // the word out of a line, and a word placed into one
  // ------------------------------------------------------------------
  f.ln("  // the word this request wants out of a whole line, R-7");
  f.ln("  function automatic word_t word_out(input line_t l);");
  f.ln("    word_out = word_t'(l >> (int'(word_of(addr_q)) * "
       "WordBits));");
  f.ln("  endfunction");
  f.ln();
  if(wr && be) {
    f.ln("  // ---------------------------------------------------"
         "------");
    f.ln("  // The line a fill installs when the request that missed");
    f.ln("  // was a write. Only the bytes the write strobes come "
         "from");
    f.ln("  // the write; every other byte is what memory returned.");
    f.ln("  //");
    f.ln("  // Merging by OR instead would leave the fetched bits");
    f.ln("  // standing under the written ones, and the read that");
    f.ln("  // follows would see neither value.");
    f.ln("  // ---------------------------------------------------"
         "------");
    f.ln("  line_t fill_merged;");
    f.ln();
    f.ln("  always_comb begin");
    f.ln("    fill_merged = fill_q;");
    f.ln("    for(int unsigned b = 0; b < WordBytes; b++) begin");
    f.ln("      if(wstrb_q[b]) begin");
    f.ln("        fill_merged[(int'(word_of(addr_q))*WordBytes + "
         "int'(b))*8 +: 8] =");
    f.ln("            wdata_q[int'(b)*8 +: 8];");
    f.ln("      end");
    f.ln("    end");
    f.ln("  end");
    f.ln();
  }

  // ------------------------------------------------------------------
  // the state machine
  // ------------------------------------------------------------------
  f.ln("  always_comb begin");
  f.ln("    cnext = cstate;");
  f.ln("    unique case (cstate)");
  f.ln("      C_INIT:");
  f.ln("        cnext = (init_set == set_t'(SetsPerBank-1))");
  f.ln("                ? C_IDLE : C_INIT;");
  f.ln("      C_IDLE:");
  f.ln("        cnext = req_valid ? C_TAG : C_IDLE;");
  f.ln("      C_TAG:");
  f.ln("        cnext = hit ? C_HIT");
  f.ln("              : (evict_needed ? C_EVICT : C_FILL);");
  f.ln("      C_HIT:");
  f.ln("        cnext = C_IDLE;");
  f.ln("      C_EVICT:");
  f.ln("        cnext = mreq_ready ? C_EVICT_W : C_EVICT;");
  f.ln("      C_EVICT_W:");
  f.ln("        cnext = mrsp_valid ? C_FILL : C_EVICT_W;");
  f.ln("      C_FILL:");
  f.ln("        cnext = mreq_ready ? C_FILL_W : C_FILL;");
  f.ln("      C_FILL_W:");
  f.ln("        cnext = mrsp_valid ? C_DONE : C_FILL_W;");
  f.ln("      C_DONE:");
  f.ln("        cnext = C_IDLE;");
  f.ln("      default:");
  f.ln("        cnext = C_IDLE;");
  f.ln("    endcase");
  f.ln("  end");
  f.ln();

  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      cstate   <= " + std::string(clr ? "C_IDLE" : "C_INIT") +
       ";");
  f.ln("      init_set <= '0;");
  f.ln("      addr_q   <= '0;");
  f.ln("      fill_q   <= '0;");
  f.ln("      sel_way  <= '0;");
  f.ln("      err_q    <= 1'b0;");
  if(wr) {
    f.ln("      write_q  <= 1'b0;");
    f.ln("      wdata_q  <= '0;");
    f.ln("      wstrb_q  <= '0;");
  }
  f.ln("    end else begin");
  f.ln("      cstate <= cnext;");
  f.ln();
  f.ln("      if(cstate == C_INIT) begin");
  f.ln("        init_set <= init_set + set_t'(1);");
  f.ln("      end");
  f.ln();
  f.ln("      if(cstate == C_IDLE && req_valid) begin");
  f.ln("        addr_q  <= req_addr;");
  f.ln("        err_q   <= 1'b0;");
  if(wr) {
    f.ln("        write_q <= req_write;");
    f.ln("        wdata_q <= req_wdata;");
    f.ln("        wstrb_q <= req_wstrb;");
  }
  f.ln("      end");
  f.ln();
  f.ln("      if(cstate == C_TAG) begin");
  f.ln("        sel_way <= hit ? hit_way : victim;");
  f.ln("      end");
  f.ln();
  f.ln("      if(cstate == C_FILL_W && mrsp_valid) begin");
  f.ln("        fill_q <= mrsp_data;");
  f.ln("        err_q  <= mrsp_err;");
  f.ln("      end");
  f.ln("      if(cstate == C_EVICT_W && mrsp_valid && mrsp_err) begin");
  f.ln("        err_q  <= 1'b1;");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  // ------------------------------------------------------------------
  // array addressing
  // ------------------------------------------------------------------
  f.ln("  // The set presented to the arrays. In C_IDLE it is the set");
  f.ln("  // of the request arriving, so a registered read lands in");
  f.ln("  // C_TAG. Everywhere else it holds the latched set.");
  f.ln("  wire set_t cur_set = (cstate == C_IDLE) ? set_of(req_addr)");
  f.ln("                                          : set_of(addr_q);");
  f.ln();
  f.ln("  assign meta_rd_set = (cstate == C_INIT) ? init_set : cur_set;");
  f.ln("  assign data_rd_set = cur_set;");
  f.ln();
  f.ln("  assign req_ready = (cstate == C_IDLE);");
  f.ln();

  // ------------------------------------------------------------------
  // the invalidate walk
  // ------------------------------------------------------------------
  f.ln("  assign meta_inv_en  = (cstate == C_INIT);");
  f.ln("  assign meta_inv_set = init_set;");
  f.ln();

  // ------------------------------------------------------------------
  // metadata writes
  // ------------------------------------------------------------------
  f.ln("  // ---------------------------------------------------------");
  f.ln("  // Installing a line. " +
       std::string(ralloc ? "A read miss allocates"
                          : "A read miss does NOT allocate") + ", " +
       std::string(walloc ? "a write miss allocates."
                          : "a write miss does NOT."));
  f.ln("  // ---------------------------------------------------------");
  f.ln("  always_comb begin");
  f.ln("    meta_wr_en    = 1'b0;");
  f.ln("    meta_wr_set   = set_of(addr_q);");
  f.ln("    meta_wr_way   = sel_way;");
  f.ln("    meta_wr_tag   = tag_of(addr_q);");
  f.ln("    meta_wr_valid = 1'b1;");
  if(dirty) f.ln("    meta_wr_dirty = 1'b0;");
  f.ln("    if(cstate == C_DONE) begin");
  if(ralloc && walloc) {
    f.ln("      meta_wr_en    = 1'b1;");
  } else if(ralloc) {
    f.ln("      meta_wr_en    = !write_q;");
  } else if(walloc) {
    f.ln("      meta_wr_en    = write_q;");
  } else {
    f.ln("      meta_wr_en    = 1'b0;   // neither miss allocates");
  }
  if(dirty) {
    f.ln("      // a write that allocates is dirty the moment it "
         "lands");
    f.ln("      meta_wr_dirty = " +
         std::string(wthru ? "1'b0" : "write_q") + ";");
  }
  f.ln("    end");
  f.ln("  end");
  f.ln();

  if(dirty) {
    f.ln("  // a write that hit dirties the way it hit");
    f.ln("  assign meta_dty_en  = (cstate == C_HIT) && write_q" +
         std::string(wthru ? " && 1'b0" : "") + ";");
    f.ln("  assign meta_dty_set = set_of(addr_q);");
    f.ln("  assign meta_dty_way = sel_way;");
    f.ln("  assign meta_dty_val = 1'b1;");
    f.ln();
  }

  f.ln("  // every access moves the replacement state of its set");
  f.ln("  assign meta_rp_en  = (cstate == C_HIT) || (cstate == C_DONE);");
  f.ln("  assign meta_rp_set = set_of(addr_q);");
  f.ln("  assign meta_rp_val = repl_update(meta_rd_repl, sel_way);");
  f.ln();

  // ------------------------------------------------------------------
  // data writes
  // ------------------------------------------------------------------
  f.ln("  always_comb begin");
  f.ln("    data_wr_en   = 1'b0;");
  f.ln("    data_wr_set  = set_of(addr_q);");
  f.ln("    data_wr_way  = sel_way;");
  f.ln("    data_wr_line = fill_q;");
  if(be) f.ln("    data_wr_be   = '0;");
  f.ln();
  f.ln("    if(cstate == C_DONE) begin");
  if(ralloc || walloc) {
    f.ln("      data_wr_en   = meta_wr_en;");
    if(wr && be) {
      f.ln("      // only a write that missed merges anything in. A");
      f.ln("      // read installs the line exactly as it arrived.");
      f.ln("      data_wr_line = write_q ? fill_merged : fill_q;");
    } else {
      f.ln("      data_wr_line = fill_q;");
    }
    if(be) f.ln("      data_wr_be   = '1;");
  }
  f.ln("    end");
  if(wr) {
    f.ln();
    f.ln("    if(cstate == C_HIT && write_q) begin");
    f.ln("      data_wr_en   = 1'b1;");
    f.ln("      data_wr_line =");
    f.ln("          line_t'(wdata_q) << (int'(word_of(addr_q)) * "
         "WordBits);");
    if(be) {
      f.ln("      data_wr_be   = '0;");
      f.ln("      data_wr_be[int'(word_of(addr_q))*WordBytes +: "
           "WordBytes] =");
      f.ln("          wstrb_q;");
    }
    f.ln("    end");
  }
  f.ln("  end");
  f.ln();

  // ------------------------------------------------------------------
  // the memory side
  // ------------------------------------------------------------------
  f.ln("  assign mreq_valid = (cstate == C_EVICT) || "
       "(cstate == C_FILL);");
  if(banked) {
    f.ln("  assign mreq_addr  = (cstate == C_EVICT)");
    f.ln("      ? line_addr(meta_rd_tag[victim], bank_id, "
         "set_of(addr_q))");
    f.ln("      : line_base(addr_q);");
  } else {
    f.ln("  assign mreq_addr  = (cstate == C_EVICT)");
    f.ln("      ? line_addr(meta_rd_tag[victim], set_of(addr_q))");
    f.ln("      : line_base(addr_q);");
  }
  if(memwr) {
    f.ln("  assign mreq_write = (cstate == C_EVICT);");
    f.ln("  assign mreq_wdata = data_rd_line[victim];");
  }
  f.ln();

  // ------------------------------------------------------------------
  // the response
  // ------------------------------------------------------------------
  f.ln("  assign rsp_valid = (cstate == C_HIT) || (cstate == C_DONE);");
  f.ln("  assign rsp_err   = err_q;");
  f.ln("  assign rsp_data  = (cstate == C_HIT)");
  f.ln("      ? word_out(data_rd_line[hit_way])");
  f.ln("      : word_out(fill_q);");
  f.ln();

  if(!memwr) {
    f.ln("  // nothing in this node is written back, so the victim's");
    f.ln("  // line is read for no purpose. It is read here so that");
    f.ln("  // the array output is not reported unused.");
    f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
    f.ln("  wire unused_ev = |{data_rd_line[victim], evict_needed};");
    f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
    f.ln();
  }

  f.ln("endmodule");
}

// --------------------------------------------------------------------
// One bank: the two arrays and the control that drives them.
// --------------------------------------------------------------------
void RtlCache::bank(SvFile &f, const NodeCtx &c)
{
  const std::string mod = c.mod("bank");
  const bool wr     = c.has_writes();
  const bool dirty  = c.has_dirty();
  const bool be     = c.byte_enables("data");
  const bool banked = c.geom().banks > 1 && c.geom().bank_resolved;
  const bool memwr  = dirty || c.write_hit() == "write_through" ||
                      (wr && c.write_miss() == "no_allocate");

  f.note("One bank of node '" + c.name() + "'. " +
         std::to_string(c.geom().sets_per_bank) + " sets of " +
         std::to_string(c.geom().associativity) + " ways.");
  if(banked) {
    f.note("");
    f.note("There are " + std::to_string(c.geom().banks) +
           " of these. R-6: the bank is chosen by address bits");
    f.note("[" + std::to_string(c.geom().bank.msb) + ":" +
           std::to_string(c.geom().bank.lsb) +
           "], taken out of the index rather than beside it.");
  }
  f.bar();
  RtlPkg::import_of(f, { c.pkg() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic  clk,");
  f.ln("  input  logic  rstn,");
  if(banked) f.ln("  input  bank_t bank_id,");
  f.ln();
  f.ln("  input  logic  req_valid,");
  f.ln("  output logic  req_ready,");
  f.ln("  input  addr_t req_addr,");
  if(wr) {
    f.ln("  input  logic  req_write,");
    f.ln("  input  word_t req_wdata,");
    f.ln("  input  logic [WordBytes-1:0] req_wstrb,");
  }
  f.ln();
  f.ln("  output logic  rsp_valid,");
  f.ln("  output word_t rsp_data,");
  f.ln("  output logic  rsp_err,");
  f.ln();
  f.ln("  output logic  mreq_valid,");
  f.ln("  input  logic  mreq_ready,");
  f.ln("  output addr_t mreq_addr,");
  if(memwr) {
    f.ln("  output logic  mreq_write,");
    f.ln("  output line_t mreq_wdata,");
  }
  f.ln("  input  logic  mrsp_valid,");
  f.ln("  input  line_t mrsp_data,");
  f.ln("  input  logic  mrsp_err");
  f.ln(");");
  f.ln();

  f.ln("  set_t        meta_rd_set;");
  f.ln("  tag_t        meta_rd_tag   [Ways];");
  f.ln("  logic        meta_rd_valid [Ways];");
  if(dirty) f.ln("  logic        meta_rd_dirty [Ways];");
  f.ln("  repl_state_t meta_rd_repl;");
  f.ln("  logic        meta_wr_en;");
  f.ln("  set_t        meta_wr_set;");
  f.ln("  way_t        meta_wr_way;");
  f.ln("  tag_t        meta_wr_tag;");
  f.ln("  logic        meta_wr_valid;");
  if(dirty) {
    f.ln("  logic        meta_wr_dirty;");
    f.ln("  logic        meta_dty_en;");
    f.ln("  set_t        meta_dty_set;");
    f.ln("  way_t        meta_dty_way;");
    f.ln("  logic        meta_dty_val;");
  }
  f.ln("  logic        meta_rp_en;");
  f.ln("  set_t        meta_rp_set;");
  f.ln("  repl_state_t meta_rp_val;");
  f.ln("  logic        meta_inv_en;");
  f.ln("  set_t        meta_inv_set;");
  f.ln();
  f.ln("  set_t        data_rd_set;");
  f.ln("  line_t       data_rd_line [Ways];");
  f.ln("  logic        data_wr_en;");
  f.ln("  set_t        data_wr_set;");
  f.ln("  way_t        data_wr_way;");
  f.ln("  line_t       data_wr_line;");
  if(be) f.ln("  logic [LineBytes-1:0] data_wr_be;");
  f.ln();

  f.ln("  " + c.mod("meta_array") + " u_meta (");
  f.ln("    .clk           (clk),");
  f.ln("    .rstn          (rstn),");
  f.ln("    .rd_set        (meta_rd_set),");
  f.ln("    .rd_tag        (meta_rd_tag),");
  f.ln("    .rd_valid      (meta_rd_valid),");
  if(dirty) f.ln("    .rd_dirty      (meta_rd_dirty),");
  f.ln("    .rd_repl       (meta_rd_repl),");
  f.ln("    .wr_en         (meta_wr_en),");
  f.ln("    .wr_set        (meta_wr_set),");
  f.ln("    .wr_way        (meta_wr_way),");
  f.ln("    .wr_tag        (meta_wr_tag),");
  f.ln("    .wr_valid      (meta_wr_valid),");
  if(dirty) {
    f.ln("    .wr_dirty      (meta_wr_dirty),");
    f.ln("    .dty_en        (meta_dty_en),");
    f.ln("    .dty_set       (meta_dty_set),");
    f.ln("    .dty_way       (meta_dty_way),");
    f.ln("    .dty_val       (meta_dty_val),");
  }
  f.ln("    .rp_en         (meta_rp_en),");
  f.ln("    .rp_set        (meta_rp_set),");
  f.ln("    .rp_val        (meta_rp_val),");
  f.ln("    .inv_en        (meta_inv_en),");
  f.ln("    .inv_set       (meta_inv_set)");
  f.ln("  );");
  f.ln();

  f.ln("  " + c.mod("data_array") + " u_data (");
  f.ln("    .clk           (clk),");
  f.ln("    .rd_set        (data_rd_set),");
  f.ln("    .rd_line       (data_rd_line),");
  f.ln("    .wr_en         (data_wr_en),");
  f.ln("    .wr_set        (data_wr_set),");
  f.ln("    .wr_way        (data_wr_way),");
  if(be) {
    f.ln("    .wr_line       (data_wr_line),");
    f.ln("    .wr_be         (data_wr_be)");
  } else {
    f.ln("    .wr_line       (data_wr_line)");
  }
  f.ln("  );");
  f.ln();

  f.ln("  " + c.mod("ctrl") + " u_ctrl (");
  f.ln("    .clk           (clk),");
  f.ln("    .rstn          (rstn),");
  if(banked) f.ln("    .bank_id       (bank_id),");
  f.ln("    .req_valid     (req_valid),");
  f.ln("    .req_ready     (req_ready),");
  f.ln("    .req_addr      (req_addr),");
  if(wr) {
    f.ln("    .req_write     (req_write),");
    f.ln("    .req_wdata     (req_wdata),");
    f.ln("    .req_wstrb     (req_wstrb),");
  }
  f.ln("    .rsp_valid     (rsp_valid),");
  f.ln("    .rsp_data      (rsp_data),");
  f.ln("    .rsp_err       (rsp_err),");
  f.ln("    .meta_rd_set   (meta_rd_set),");
  f.ln("    .meta_rd_tag   (meta_rd_tag),");
  f.ln("    .meta_rd_valid (meta_rd_valid),");
  if(dirty) f.ln("    .meta_rd_dirty (meta_rd_dirty),");
  f.ln("    .meta_rd_repl  (meta_rd_repl),");
  f.ln("    .meta_wr_en    (meta_wr_en),");
  f.ln("    .meta_wr_set   (meta_wr_set),");
  f.ln("    .meta_wr_way   (meta_wr_way),");
  f.ln("    .meta_wr_tag   (meta_wr_tag),");
  f.ln("    .meta_wr_valid (meta_wr_valid),");
  if(dirty) {
    f.ln("    .meta_wr_dirty (meta_wr_dirty),");
    f.ln("    .meta_dty_en   (meta_dty_en),");
    f.ln("    .meta_dty_set  (meta_dty_set),");
    f.ln("    .meta_dty_way  (meta_dty_way),");
    f.ln("    .meta_dty_val  (meta_dty_val),");
  }
  f.ln("    .meta_rp_en    (meta_rp_en),");
  f.ln("    .meta_rp_set   (meta_rp_set),");
  f.ln("    .meta_rp_val   (meta_rp_val),");
  f.ln("    .meta_inv_en   (meta_inv_en),");
  f.ln("    .meta_inv_set  (meta_inv_set),");
  f.ln("    .data_rd_set   (data_rd_set),");
  f.ln("    .data_rd_line  (data_rd_line),");
  f.ln("    .data_wr_en    (data_wr_en),");
  f.ln("    .data_wr_set   (data_wr_set),");
  f.ln("    .data_wr_way   (data_wr_way),");
  f.ln("    .data_wr_line  (data_wr_line),");
  if(be) f.ln("    .data_wr_be    (data_wr_be),");
  f.ln("    .mreq_valid    (mreq_valid),");
  f.ln("    .mreq_ready    (mreq_ready),");
  f.ln("    .mreq_addr     (mreq_addr),");
  if(memwr) {
    f.ln("    .mreq_write    (mreq_write),");
    f.ln("    .mreq_wdata    (mreq_wdata),");
  }
  f.ln("    .mrsp_valid    (mrsp_valid),");
  f.ln("    .mrsp_data     (mrsp_data),");
  f.ln("    .mrsp_err      (mrsp_err)");
  f.ln("  );");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
// The node. The slave adapters, the banks, the master adapter, and
// the arbitration that joins them.
//
// ARBITRATION IS A GENERATOR DECISION HERE AND THE CONFIGURATION DOES
// NOT COVER IT. D-27 puts arbitration on an INTERFACE, which is the
// scope that aggregates the ports contending for one link. Every
// interface in this configuration carries exactly one port, so no
// interface has anything to arbitrate. What actually contends on this
// node is one interface against another for the banks behind them,
// and no field describes that. The emitter chooses round robin and
// says so. See R-9 and Q-08.
// --------------------------------------------------------------------
void RtlCache::top(SvFile &f, const NodeCtx &c)
{
  const std::string mod = c.mod();
  const bool wr     = c.has_writes();
  const bool dirty  = c.has_dirty();
  const bool banked = c.geom().banks > 1 && c.geom().bank_resolved;
  const bool memwr  = dirty || c.write_hit() == "write_through" ||
                      (wr && c.write_miss() == "no_allocate");

  std::vector<const NodeCtx::Iface *> sl = c.slaves();
  std::vector<const NodeCtx::Iface *> ms = c.masters();

  const int  nsl   = int(sl.size());
  const int  nbk   = banked ? c.geom().banks : 1;
  const bool marb  = nsl > 1;
  const bool barb  = nbk > 1;
  const int  sbits = idx_bits(nsl);
  const int  bbits = idx_bits(nbk);

  f.note("Node '" + c.name() + "', a " + c.type() + ".");
  f.note("");
  f.note(std::to_string(nsl) + " slave interface" +
         (nsl == 1 ? "" : "s") + ", " + std::to_string(nbk) +
         " bank" + (nbk == 1 ? "" : "s") + ", " +
         std::to_string(ms.size()) + " master interface" +
         (ms.size() == 1 ? "" : "s") + ".");
  if(marb) {
    f.note("");
    f.note("The slave interfaces contend for the banks. Nothing in "
           "the");
    f.note("configuration says how, because D-27 puts arbitration on "
           "an");
    f.note("interface and each of these carries one port. ROUND ROBIN "
           "is");
    f.note("the emitter's choice: a pointer per bank, advanced on "
           "every");
    f.note("grant, so neither side can starve the other.");
  }
  if(barb) {
    f.note("");
    f.note("The banks contend for the one downstream link, and that "
           "is");
    f.note("round robin on the same reasoning.");
  }
  f.bar();
  RtlPkg::import_of(f, { c.pkg() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic                 clk,");
  f.ln("  input  logic                 rstn,");
  for(size_t k = 0; k < c.ifaces().size(); ++k) {
    f.ln();
    f.ln("  // interface '" + c.ifaces()[k].name + "', link '" +
         c.ifaces()[k].link + "'");
    f.lines(iface_ports(c.ifaces()[k],
                        k + 1 == c.ifaces().size()));
  }
  f.ln(");");
  f.ln();

  f.ln("  localparam int unsigned NSlv = " + i2s(nsl) + ";");
  f.ln("  localparam int unsigned NBnk = " + i2s(nbk) + ";");
  f.ln("  typedef logic [" + i2s(sbits - 1) + ":0] src_t;");
  f.ln("  typedef logic [" + i2s(bbits - 1) + ":0] bsel_t;");
  f.ln();

  // ------------------------------------------------------------------
  // the per slave and per bank bundles
  // ------------------------------------------------------------------
  f.ln("  // the internal request bundle, one per slave adapter");
  f.ln("  logic  s_req_valid [NSlv];");
  f.ln("  logic  s_req_ready [NSlv];");
  f.ln("  addr_t s_req_addr  [NSlv];");
  if(wr) {
    f.ln("  logic  s_req_write [NSlv];");
    f.ln("  word_t s_req_wdata [NSlv];");
    f.ln("  logic [WordBytes-1:0] s_req_wstrb [NSlv];");
  }
  f.ln("  logic  s_rsp_valid [NSlv];");
  f.ln("  word_t s_rsp_data  [NSlv];");
  f.ln("  logic  s_rsp_err   [NSlv];");
  f.ln();
  f.ln("  // the same bundle on the bank side");
  f.ln("  logic  b_req_valid [NBnk];");
  f.ln("  logic  b_req_ready [NBnk];");
  f.ln("  addr_t b_req_addr  [NBnk];");
  if(wr) {
    f.ln("  logic  b_req_write [NBnk];");
    f.ln("  word_t b_req_wdata [NBnk];");
    f.ln("  logic [WordBytes-1:0] b_req_wstrb [NBnk];");
  }
  f.ln("  logic  b_rsp_valid [NBnk];");
  f.ln("  word_t b_rsp_data  [NBnk];");
  f.ln("  logic  b_rsp_err   [NBnk];");
  f.ln("  src_t  b_sel       [NBnk];");
  f.ln("  src_t  b_owner     [NBnk];");
  f.ln();
  f.ln("  // the memory side of each bank");
  f.ln("  logic  b_mreq_valid [NBnk];");
  f.ln("  logic  b_mreq_ready [NBnk];");
  f.ln("  addr_t b_mreq_addr  [NBnk];");
  if(memwr) {
    f.ln("  logic  b_mreq_write [NBnk];");
    f.ln("  line_t b_mreq_wdata [NBnk];");
  }
  f.ln("  logic  b_mrsp_valid [NBnk];");
  f.ln("  line_t b_mrsp_data  [NBnk];");
  f.ln("  logic  b_mrsp_err   [NBnk];");
  f.ln();
  f.ln("  // the one downstream request");
  f.ln("  logic  m_req_valid;");
  f.ln("  logic  m_req_ready;");
  f.ln("  addr_t m_req_addr;");
  if(memwr) {
    f.ln("  logic  m_req_write;");
    f.ln("  line_t m_req_wdata;");
  }
  f.ln("  logic  m_rsp_valid;");
  f.ln("  line_t m_rsp_data;");
  f.ln("  logic  m_rsp_err;");
  f.ln("  bsel_t m_sel;");
  f.ln("  bsel_t m_owner;");
  f.ln("  logic  m_grant;");
  f.ln();
  if(marb) f.ln("  src_t  rr_slv [NBnk];");
  if(barb) f.ln("  bsel_t rr_bnk;");
  if(marb || barb) f.ln();

  // ------------------------------------------------------------------
  // the slave adapters
  // ------------------------------------------------------------------
  for(int k = 0; k < nsl; ++k) {
    const NodeCtx::Iface &i = *sl[size_t(k)];
    f.ln("  " + c.mod((i.name + "_slv").c_str()) + " u_" + i.name +
         "_slv (");
    f.ln("    .clk       (clk),");
    f.ln("    .rstn      (rstn),");
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      f.ln("    ." + pad(LinkSig::wire(i.name, g.local), 18) + "(" +
           LinkSig::wire(i.name, g.local) + "),");
    }
    f.ln("    .req_valid (s_req_valid[" + i2s(k) + "]),");
    f.ln("    .req_ready (s_req_ready[" + i2s(k) + "]),");
    f.ln("    .req_addr  (s_req_addr[" + i2s(k) + "]),");
    if(wr) {
      f.ln("    .req_write (s_req_write[" + i2s(k) + "]),");
      f.ln("    .req_wdata (s_req_wdata[" + i2s(k) + "]),");
      f.ln("    .req_wstrb (s_req_wstrb[" + i2s(k) + "]),");
    }
    f.ln("    .rsp_valid (s_rsp_valid[" + i2s(k) + "]),");
    f.ln("    .rsp_data  (s_rsp_data[" + i2s(k) + "]),");
    f.ln("    .rsp_err   (s_rsp_err[" + i2s(k) + "])");
    f.ln("  );");
    f.ln();
  }

  // ------------------------------------------------------------------
  // slave to bank arbitration
  // ------------------------------------------------------------------
  f.ln("  // ---------------------------------------------------------");
  f.ln("  // Which slave each bank takes this cycle.");
  f.ln("  // ---------------------------------------------------------");
  f.ln("  always_comb begin");
  f.ln("    for(int unsigned b = 0; b < NBnk; b++) begin");
  f.ln("      b_req_valid[b] = 1'b0;");
  f.ln("      b_sel[b]       = '0;");
  f.ln("    end");
  f.ln("    for(int unsigned b = 0; b < NBnk; b++) begin");
  f.ln("      for(int unsigned k = 0; k < NSlv; k++) begin");
  if(marb) {
    f.ln("        // start the walk at the round robin pointer, so a");
    f.ln("        // slave that was just granted goes to the back");
    f.ln("        automatic src_t s =");
    f.ln("            (int'(rr_slv[b]) + int'(k) >= int'(NSlv))");
    f.ln("            ? src_t'(int'(rr_slv[b]) + int'(k) - int'(NSlv))");
    f.ln("            : src_t'(int'(rr_slv[b]) + int'(k));");
  } else {
    f.ln("        // one slave, there is nothing to rotate");
    f.ln("        automatic src_t s = src_t'(k);");
  }
  f.ln("        if(!b_req_valid[b] && s_req_valid[s]");
  if(banked) {
    f.ln("           && (bank_of(s_req_addr[s]) == bank_t'(b))) begin");
  } else {
    f.ln("           ) begin");
  }
  f.ln("          b_req_valid[b] = 1'b1;");
  f.ln("          b_sel[b]       = s;");
  f.ln("        end");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  f.ln("  // the request the winning slave presented");
  f.ln("  always_comb begin");
  f.ln("    for(int unsigned b = 0; b < NBnk; b++) begin");
  f.ln("      b_req_addr[b]  = s_req_addr[b_sel[b]];");
  if(wr) {
    f.ln("      b_req_write[b] = s_req_write[b_sel[b]];");
    f.ln("      b_req_wdata[b] = s_req_wdata[b_sel[b]];");
    f.ln("      b_req_wstrb[b] = s_req_wstrb[b_sel[b]];");
  }
  f.ln("    end");
  f.ln("  end");
  f.ln();

  f.ln("  // ready back to the slave that won");
  f.ln("  always_comb begin");
  f.ln("    for(int unsigned s = 0; s < NSlv; s++) begin");
  f.ln("      s_req_ready[s] = 1'b0;");
  f.ln("    end");
  f.ln("    for(int unsigned b = 0; b < NBnk; b++) begin");
  f.ln("      if(b_req_valid[b]) begin");
  f.ln("        s_req_ready[b_sel[b]] = b_req_ready[b];");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  f.ln("  // the response goes back to whoever the bank took it from");
  f.ln("  always_comb begin");
  f.ln("    for(int unsigned s = 0; s < NSlv; s++) begin");
  f.ln("      s_rsp_valid[s] = 1'b0;");
  f.ln("      s_rsp_data[s]  = '0;");
  f.ln("      s_rsp_err[s]   = 1'b0;");
  f.ln("    end");
  f.ln("    for(int unsigned b = 0; b < NBnk; b++) begin");
  f.ln("      if(b_rsp_valid[b]) begin");
  f.ln("        s_rsp_valid[b_owner[b]] = 1'b1;");
  f.ln("        s_rsp_data[b_owner[b]]  = b_rsp_data[b];");
  f.ln("        s_rsp_err[b_owner[b]]   = b_rsp_err[b];");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      for(int unsigned b = 0; b < NBnk; b++) begin");
  f.ln("        b_owner[b] <= '0;");
  if(marb) f.ln("        rr_slv[b]  <= '0;");
  f.ln("      end");
  f.ln("    end else begin");
  f.ln("      for(int unsigned b = 0; b < NBnk; b++) begin");
  f.ln("        if(b_req_valid[b] && b_req_ready[b]) begin");
  f.ln("          b_owner[b] <= b_sel[b];");
  if(marb) {
    f.ln("          rr_slv[b]  <= (b_sel[b] == src_t'(NSlv-1))");
    f.ln("                        ? src_t'(0)");
    f.ln("                        : b_sel[b] + src_t'(1);");
  }
  f.ln("        end");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();

  // ------------------------------------------------------------------
  // the banks
  // ------------------------------------------------------------------
  f.ln("  // ---------------------------------------------------------");
  f.ln("  // The banks. Identical by construction, which is the point");
  f.ln("  // of generating them.");
  f.ln("  // ---------------------------------------------------------");
  f.ln("  for(genvar b = 0; b < int'(NBnk); b++) begin : g_bank");
  f.ln("    " + c.mod("bank") + " u_bank (");
  f.ln("      .clk        (clk),");
  f.ln("      .rstn       (rstn),");
  if(banked) f.ln("      .bank_id    (bank_t'(b)),");
  f.ln("      .req_valid  (b_req_valid[b]),");
  f.ln("      .req_ready  (b_req_ready[b]),");
  f.ln("      .req_addr   (b_req_addr[b]),");
  if(wr) {
    f.ln("      .req_write  (b_req_write[b]),");
    f.ln("      .req_wdata  (b_req_wdata[b]),");
    f.ln("      .req_wstrb  (b_req_wstrb[b]),");
  }
  f.ln("      .rsp_valid  (b_rsp_valid[b]),");
  f.ln("      .rsp_data   (b_rsp_data[b]),");
  f.ln("      .rsp_err    (b_rsp_err[b]),");
  f.ln("      .mreq_valid (b_mreq_valid[b]),");
  f.ln("      .mreq_ready (b_mreq_ready[b]),");
  f.ln("      .mreq_addr  (b_mreq_addr[b]),");
  if(memwr) {
    f.ln("      .mreq_write (b_mreq_write[b]),");
    f.ln("      .mreq_wdata (b_mreq_wdata[b]),");
  }
  f.ln("      .mrsp_valid (b_mrsp_valid[b]),");
  f.ln("      .mrsp_data  (b_mrsp_data[b]),");
  f.ln("      .mrsp_err   (b_mrsp_err[b])");
  f.ln("    );");
  f.ln("  end : g_bank");
  f.ln();

  // ------------------------------------------------------------------
  // bank to master arbitration
  // ------------------------------------------------------------------
  f.ln("  // ---------------------------------------------------------");
  f.ln("  // Which bank owns the downstream link this cycle.");
  f.ln("  // ---------------------------------------------------------");
  f.ln("  always_comb begin");
  f.ln("    m_grant     = 1'b0;");
  f.ln("    m_sel       = '0;");
  f.ln("    for(int unsigned k = 0; k < NBnk; k++) begin");
  if(barb) {
    f.ln("      automatic bsel_t b =");
    f.ln("          (int'(rr_bnk) + int'(k) >= int'(NBnk))");
    f.ln("          ? bsel_t'(int'(rr_bnk) + int'(k) - int'(NBnk))");
    f.ln("          : bsel_t'(int'(rr_bnk) + int'(k));");
  } else {
    f.ln("      automatic bsel_t b = bsel_t'(k);   // one bank");
  }
  f.ln("      if(!m_grant && b_mreq_valid[b]) begin");
  f.ln("        m_grant = 1'b1;");
  f.ln("        m_sel   = b;");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();
  f.ln("  assign m_req_valid = m_grant;");
  f.ln("  assign m_req_addr  = b_mreq_addr[m_sel];");
  if(memwr) {
    f.ln("  assign m_req_write = b_mreq_write[m_sel];");
    f.ln("  assign m_req_wdata = b_mreq_wdata[m_sel];");
  }
  f.ln();
  f.ln("  always_comb begin");
  f.ln("    for(int unsigned b = 0; b < NBnk; b++) begin");
  f.ln("      b_mreq_ready[b] = m_grant && (m_sel == bsel_t'(b))");
  f.ln("                        && m_req_ready;");
  f.ln("      b_mrsp_valid[b] = m_rsp_valid && (m_owner == bsel_t'(b));");
  f.ln("      b_mrsp_data[b]  = m_rsp_data;");
  f.ln("      b_mrsp_err[b]   = m_rsp_err;");
  f.ln("    end");
  f.ln("  end");
  f.ln();
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      m_owner <= '0;");
  if(barb) f.ln("      rr_bnk  <= '0;");
  f.ln("    end else if(m_req_valid && m_req_ready) begin");
  f.ln("      m_owner <= m_sel;");
  if(barb) {
    f.ln("      rr_bnk  <= (m_sel == bsel_t'(NBnk-1))");
    f.ln("                 ? bsel_t'(0) : m_sel + bsel_t'(1);");
  }
  f.ln("    end");
  f.ln("  end");
  f.ln();

  // ------------------------------------------------------------------
  // the master adapter
  // ------------------------------------------------------------------
  for(const NodeCtx::Iface *ip : ms) {
    const NodeCtx::Iface &i = *ip;
    f.ln("  " + c.mod((i.name + "_mst").c_str()) + " u_" + i.name +
         "_mst (");
    f.ln("    .clk        (clk),");
    f.ln("    .rstn       (rstn),");
    f.ln("    .mreq_valid (m_req_valid),");
    f.ln("    .mreq_ready (m_req_ready),");
    f.ln("    .mreq_addr  (m_req_addr),");
    if(memwr) {
      f.ln("    .mreq_write (m_req_write),");
      f.ln("    .mreq_wdata (m_req_wdata),");
    }
    f.ln("    .mrsp_valid (m_rsp_valid),");
    f.ln("    .mrsp_data  (m_rsp_data),");
    f.ln("    .mrsp_err   (m_rsp_err),");
    const std::vector<LinkSig::Sig> &sg = i.sig.sigs();
    for(size_t k = 0; k < sg.size(); ++k) {
      std::string l = "    ." +
                      pad(LinkSig::wire(i.name, sg[k].local), 19) +
                      "(" + LinkSig::wire(i.name, sg[k].local) + ")";
      if(k + 1 < sg.size()) l += ",";
      f.ln(l);
    }
    f.ln("  );");
    f.ln();
  }

  f.ln("endmodule");
}

} // namespace cgen
