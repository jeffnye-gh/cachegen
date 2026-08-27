// --------------------------------------------------------------------
// FILE:    rtl_tb.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "rtl_tb.h"
#include "rtl_agent.h"
#include "rtl_cache.h"
#include "rtl_pkg.h"

namespace cgen
{

namespace {
std::string i2s(int v) { return std::to_string(v); }
}

// --------------------------------------------------------------------
// The shared task set. Included inside a testbench module, so clk and
// rstn resolve to that module's own signals.
//
// Nothing here knows a protocol. Everything protocol specific is
// generated per node.
// --------------------------------------------------------------------
void RtlTb::tasks(SvFile &f)
{
  f.note("The task set every testbench includes. `include this INSIDE");
  f.note("the module, not at file scope: the tasks name clk and rstn");
  f.note("and they resolve to the including module's own signals.");
  f.note("");
  f.note("Nothing here knows a protocol. R-5 asks for the tasks used");
  f.note("by both the unit testbenches and the top level testbench,");
  f.note("and what those have in common is counting, timing and");
  f.note("reporting, not signalling.");
  f.bar();
  f.ln();
  f.ln("  int unsigned cg_pass;");
  f.ln("  int unsigned cg_fail;");
  f.ln("  int unsigned cg_cycle;");
  f.ln("  int unsigned cg_limit;");
  f.ln();
  f.ln("  // cycle counter and the run away guard. The limit is a");
  f.ln("  // simulation control, so it is a plusarg and never a");
  f.ln("  // configuration field. D-41.");
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) cg_cycle <= 0;");
  f.ln("    else      cg_cycle <= cg_cycle + 1;");
  f.ln("  end");
  f.ln();
  f.ln("  task automatic cg_init();");
  f.ln("    cg_pass  = 0;");
  f.ln("    cg_fail  = 0;");
  f.ln("    cg_limit = 100000;");
  f.ln("    void'($value$plusargs(\"cg_limit=%d\", cg_limit));");
  f.ln("  endtask");
  f.ln();
  f.ln("  task automatic cg_tick(input int unsigned n = 1);");
  f.ln("    for(int unsigned k = 0; k < n; k++) begin");
  f.ln("      @(posedge clk);");
  f.ln("    end");
  f.ln("  endtask");
  f.ln();
  f.ln("  task automatic cg_check(input string what,");
  f.ln("                          input logic  ok);");
  f.ln("    if(ok) begin");
  f.ln("      cg_pass = cg_pass + 1;");
  f.ln("      $display(\"PASS %0t %s\", $time, what);");
  f.ln("    end else begin");
  f.ln("      cg_fail = cg_fail + 1;");
  f.ln("      $display(\"FAIL %0t %s\", $time, what);");
  f.ln("    end");
  f.ln("  endtask");
  f.ln();
  f.ln("  task automatic cg_check_eq(input string what,");
  f.ln("                             input longint unsigned got,");
  f.ln("                             input longint unsigned exp);");
  f.ln("    if(got == exp) begin");
  f.ln("      cg_pass = cg_pass + 1;");
  f.ln("      $display(\"PASS %0t %s got 0x%0h\", $time, what, got);");
  f.ln("    end else begin");
  f.ln("      cg_fail = cg_fail + 1;");
  f.ln("      $display(\"FAIL %0t %s got 0x%0h want 0x%0h\",");
  f.ln("               $time, what, got, exp);");
  f.ln("    end");
  f.ln("  endtask");
  f.ln();
  f.ln("  // Wait for a condition or give up. A timeout is a failure");
  f.ln("  // and it is reported as one, it does not stop the run.");
  f.ln("  //");
  f.ln("  // SAMPLING IS ON THE NEGEDGE, and every wait in every");
  f.ln("  // testbench does the same. The design is posedge clocked,");
  f.ln("  // so at the negedge the cycle's values have settled and");
  f.ln("  // are unambiguous. Sampling on the posedge races the");
  f.ln("  // design's own updates and drops one cycle pulses, which");
  f.ln("  // is exactly what a response valid is.");
  f.ln("  task automatic cg_wait(input string what, ref logic sig);");
  f.ln("    int unsigned spent;");
  f.ln("    spent = 0;");
  f.ln("    while(sig !== 1'b1 && spent < cg_limit) begin");
  f.ln("      @(negedge clk);");
  f.ln("      spent = spent + 1;");
  f.ln("    end");
  f.ln("    if(spent >= cg_limit) begin");
  f.ln("      cg_fail = cg_fail + 1;");
  f.ln("      $display(\"FAIL %0t timeout waiting for %s\",");
  f.ln("               $time, what);");
  f.ln("    end");
  f.ln("  endtask");
  f.ln();
  f.ln("  // Reset is released on a negedge, for the reason "
       "cg_wait");
  f.ln("  // gives: a testbench that drives on the same edge the");
  f.ln("  // design clocks on races the design's own updates.");
  f.ln("  task automatic cg_reset(output logic r);");
  f.ln("    r = 1'b0;");
  f.ln("    repeat(8) @(negedge clk);");
  f.ln("    r = 1'b1;");
  f.ln("    @(negedge clk);");
  f.ln("  endtask");
  f.ln();
  f.ln("  function automatic int unsigned cg_report(input string who);");
  f.ln("    $display(\"----------------------------------------\");");
  f.ln("    $display(\"%s: %0d passed, %0d failed, %0d cycles\",");
  f.ln("             who, cg_pass, cg_fail, cg_cycle);");
  f.ln("    if(cg_fail == 0) $display(\"%s: PASS\", who);");
  f.ln("    else             $display(\"%s: FAIL\", who);");
  f.ln("    $display(\"----------------------------------------\");");
  f.ln("    cg_report = cg_fail;");
  f.ln("  endfunction");
}

// --------------------------------------------------------------------
// A TileLink responder for a unit testbench. Smaller than the system
// memory model and generated from the same link bundle, so the unit
// testbench of a node exercises the exact port list the node has.
// --------------------------------------------------------------------
void RtlTb::tb_mem(SvFile &f, const NodeCtx &c,
                   const NodeCtx::Iface &i)
{
  const std::string mod = c.mod("tb_mem");
  const int shift = Replacement::log2i(i.sig.data_bytes());

  f.note("A downstream responder for the unit testbench of node '" +
         c.name() + "'.");
  f.note("");
  f.note("NOT SYNTHESIZABLE and not part of the design. It answers "
         "link");
  f.note("'" + i.link + "' with a sparse store so that a miss in the "
         "node");
  f.note("under test has somewhere to go. Beats come back with the");
  f.note("address baked into them, so a test can predict what a fill");
  f.note("should contain without keeping its own model.");
  f.bar();
  RtlPkg::import_of(f, { c.pkg(), RtlPkg::tl_pkg_name() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic  clk,");
  f.ln("  input  logic  rstn,");
  f.ln();
  std::vector<std::string> pv = RtlCache::iface_ports(i, true);
  // the responder is the SLAVE end of a link this node masters, so
  // every direction is the other way round from the node's own port
  for(std::string &s : pv) {
    if(s.compare(2, 6, "output") == 0)     s.replace(2, 6, "input ");
    else if(s.compare(2, 5, "input") == 0) s.replace(2, 5, "output");
  }
  f.lines(pv);
  f.ln(");");
  f.ln();

  const std::string a = i.name + "_a";
  const std::string d = i.name + "_d";

  f.ln("  localparam int unsigned BeatShift = " + i2s(shift) + ";");
  f.ln();
  f.ln("  beat_t store [longint unsigned];");
  f.ln();
  f.ln("  typedef enum logic [1:0] {");
  f.ln("    T_IDLE, T_WDATA, T_READ, T_ACK");
  f.ln("  } tstate_e;");
  f.ln();
  f.ln("  tstate_e tstate;");
  f.ln("  addr_t   addr_q;");
  f.ln("  logic [2:0] size_q;");
  f.ln("  logic [15:0] beats_q;");
  f.ln("  logic [15:0] beat_i;");
  for(const LinkSig::Sig &g : i.sig.sigs()) {
    if(g.local == "a_source") {
      f.ln("  logic [" + i2s(g.bits - 1) + ":0] source_q;");
    }
  }
  f.ln();
  f.ln("  function automatic logic [15:0] beats_of"
       "(input logic [2:0] s);");
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
  f.ln("  // a beat nobody has written carries its own key, so a test");
  f.ln("  // can predict a fill without keeping a second model");
  f.ln("  function automatic beat_t seed_of"
       "(input longint unsigned k);");
  f.ln("    seed_of = beat_t'(k) ^ beat_t'(64'hcafe_0000_0000_0001);");
  f.ln("  endfunction");
  f.ln();
  f.ln("  wire a_fire = " + a + "_valid && " + a + "_ready;");
  f.ln("  wire d_fire = " + d + "_valid && " + d + "_ready;");
  f.ln();
  f.ln("  assign " + a + "_ready   = (tstate == T_IDLE) ||");
  f.ln("                            (tstate == T_WDATA);");
  f.ln("  assign " + d + "_valid   = (tstate == T_READ) ||");
  f.ln("                            (tstate == T_ACK);");
  f.ln("  assign " + d + "_opcode  = (tstate == T_READ)");
  f.ln("                            ? TlDAccessAckData : TlDAccessAck;");
  f.ln("  assign " + d + "_param   = TlParamZero;");
  f.ln("  assign " + d + "_size    = size_q;");
  f.ln("  assign " + d + "_source  = source_q;");
  f.ln("  assign " + d + "_sink    = '0;");
  f.ln("  assign " + d + "_denied  = 1'b0;");
  f.ln("  assign " + d + "_corrupt = 1'b0;");
  f.ln("  assign " + d + "_data    =");
  f.ln("      store.exists(key_of(addr_q, beat_i))");
  f.ln("      ? store[key_of(addr_q, beat_i)]");
  f.ln("      : seed_of(key_of(addr_q, beat_i));");
  f.ln();
  f.ln("  // The store is an associative array, and IEEE 1800-2023");
  f.ln("  // 6.21 forbids a nonblocking assignment to a dynamically");
  f.ln("  // sized variable. Its writes are therefore blocking and");
  f.ln("  // BLKSEQ is turned off across this process alone.");
  f.ln("  /* verilator lint_off BLKSEQ */");
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      tstate   <= T_IDLE;");
  f.ln("      addr_q   <= '0;");
  f.ln("      size_q   <= 3'd0;");
  f.ln("      beats_q  <= 16'd1;");
  f.ln("      beat_i   <= 16'd0;");
  f.ln("      source_q <= '0;");
  f.ln("      store.delete();");
  f.ln("    end else begin");
  f.ln("      case (tstate)");
  f.ln("        T_IDLE: begin");
  f.ln("          if(a_fire) begin");
  f.ln("            addr_q   <= " + a + "_address;");
  f.ln("            size_q   <= " + a + "_size;");
  f.ln("            source_q <= " + a + "_source;");
  f.ln("            beats_q  <= beats_of(" + a + "_size);");
  f.ln("            if((" + a + "_opcode == TlAPutFullData) ||");
  f.ln("               (" + a + "_opcode == TlAPutPartialData)) begin");
  f.ln("              store[key_of(" + a + "_address, 16'd0)] = " +
       a + "_data;");
  f.ln("              beat_i  <= 16'd1;");
  f.ln("              tstate  <= (beats_of(" + a +
       "_size) > 16'd1)");
  f.ln("                         ? T_WDATA : T_ACK;");
  f.ln("            end else begin");
  f.ln("              beat_i  <= 16'd0;");
  f.ln("              tstate  <= T_READ;");
  f.ln("            end");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        T_WDATA: begin");
  f.ln("          if(a_fire) begin");
  f.ln("            store[key_of(addr_q, beat_i)] = " + a + "_data;");
  f.ln("            if(beat_i == beats_q - 16'd1) begin");
  f.ln("              beat_i <= 16'd0;");
  f.ln("              tstate <= T_ACK;");
  f.ln("            end else begin");
  f.ln("              beat_i <= beat_i + 16'd1;");
  f.ln("            end");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        T_READ: begin");
  f.ln("          if(d_fire) begin");
  f.ln("            if(beat_i == beats_q - 16'd1) begin");
  f.ln("              beat_i <= 16'd0;");
  f.ln("              tstate <= T_IDLE;");
  f.ln("            end else begin");
  f.ln("              beat_i <= beat_i + 16'd1;");
  f.ln("            end");
  f.ln("          end");
  f.ln("        end");
  f.ln();
  f.ln("        T_ACK: begin");
  f.ln("          if(d_fire) tstate <= T_IDLE;");
  f.ln("        end");
  f.ln();
  f.ln("        default: tstate <= T_IDLE;");
  f.ln("      endcase");
  f.ln("    end");
  f.ln("  end");
  f.ln("  /* verilator lint_on BLKSEQ */");
  f.ln();
  if(i.sig.has_bce()) {
    f.ln("  // TL-C channels the responder does not run, tied off");
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.local.size() < 2 || g.local[1] != '_') continue;
      const char ch = g.local[0];
      if(ch != 'b' && ch != 'c' && ch != 'e') continue;
      if(g.m_drives == i.master) continue;   // the node drives it
      f.ln("  assign " + LinkSig::wire(i.name, g.local) + " = " +
           (g.bits == 1 ? "1'b0" : "'0") + ";");
    }
    f.ln();
    f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
    f.ln("  wire unused_bce = |{");
    std::string acc;
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.local.size() < 2 || g.local[1] != '_') continue;
      const char ch = g.local[0];
      if(ch != 'b' && ch != 'c' && ch != 'e') continue;
      if(g.m_drives != i.master) continue;
      if(!acc.empty()) acc += ",\n";
      acc += "      " + LinkSig::wire(i.name, g.local);
    }
    if(acc.empty()) acc = "      1'b0";
    f.ln(acc);
    f.ln("  };");
    f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
    f.ln();
  }
  f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
  f.ln("  wire unused_a = |{" + a + "_param, " + a + "_mask, " +
       a + "_corrupt};");
  f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
// One slave driver task, generated from the same bundle the node's
// port list came from.
// --------------------------------------------------------------------
static void drv_task(SvFile &f, const NodeCtx &c,
                     const NodeCtx::Iface &i)
{
  const bool tl = i.sig.is_tl();
  const std::string n = i.name;
  const std::string vld = tl ? n + "_a_valid" : n + "_valid";
  const std::string rdy = tl ? n + "_a_ready" : n + "_ready";

  f.ln("  // -------------------------------------------------------");
  f.ln("  // Drive one request into interface '" + n + "'.");
  f.ln("  //");
  f.ln("  // EVERY EDGE HERE IS A NEGEDGE. The design clocks on the");
  f.ln("  // posedge, so a testbench that drives or samples there is");
  f.ln("  // racing it: the request can be taken in the same instant");
  f.ln("  // it is presented, and a one cycle response can be gone");
  f.ln("  // before it is looked at. Driving and sampling mid cycle");
  f.ln("  // removes both races and needs no clocking block.");
  f.ln("  // -------------------------------------------------------");
  f.ln("  task automatic drv_" + n + "(input addr_t a,");
  f.ln("                       input logic  wr,");
  f.ln("                       input word_t d,");
  f.ln("                       input logic [WordBytes-1:0] be,");
  f.ln("                       output word_t r);");
  f.ln("    logic ok;");
  f.ln("    int unsigned spin;");
  f.ln();
  f.ln("    @(negedge clk);");
  if(tl) {
    f.ln("    " + n + "_a_valid   = 1'b1;");
    f.ln("    " + n + "_a_opcode  = wr ? TlAPutFullData : TlAGet;");
    f.ln("    " + n + "_a_param   = TlParamZero;");
    f.ln("    " + n + "_a_size    = 3'd" +
         std::to_string(Replacement::log2i(i.sig.data_bytes())) + ";");
    f.ln("    " + n + "_a_source  = '0;");
    f.ln("    " + n + "_a_address = a;");
    f.ln("    " + n + "_a_mask    = wr ? {{WordBytes{1'b0}} | be}"
         " : '1;");
    f.ln("    " + n + "_a_data    = d;");
    f.ln("    " + n + "_a_corrupt = 1'b0;");
    f.ln("    " + n + "_d_ready   = 1'b1;");
  } else {
    // drive whatever the bundle carries, not whatever this node
    // happens to use. The wires exist either way.
    f.ln("    " + n + "_valid = 1'b1;");
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.local == "rw")    f.ln("    " + n + "_rw    = wr;");
      if(g.local == "wdata") f.ln("    " + n + "_wdata = d;");
      if(g.local == "wstrb") f.ln("    " + n + "_wstrb = be;");
    }
    f.ln("    " + n + "_addr  = a;");
  }
  f.ln();
  f.ln("    // the transfer is the posedge that ends a cycle in which");
  f.ln("    // both valid and ready stood");
  f.ln("    ok   = " + rdy + ";");
  f.ln("    spin = 0;");
  f.ln("    while(!ok && spin < cg_limit) begin");
  f.ln("      @(negedge clk);");
  f.ln("      ok   = " + rdy + ";");
  f.ln("      spin = spin + 1;");
  f.ln("    end");
  f.ln("    if(spin >= cg_limit) begin");
  f.ln("      cg_fail = cg_fail + 1;");
  f.ln("      $display(\"FAIL %0t drv_" + n +
       " request never accepted\", $time);");
  f.ln("    end");
  f.ln();
  f.ln("    // past the transfer edge, take the request down");
  f.ln("    @(negedge clk);");
  f.ln("    " + vld + " = 1'b0;");
  f.ln();
  f.ln("    r    = '0;");
  if(tl) {
    f.ln("    ok   = 1'b0;");
  } else {
    f.ln("    // this link declares write_response false, so a write");
    f.ln("    // is complete as soon as it has been accepted");
    f.ln("    ok   = wr;");
  }
  f.ln("    spin = 0;");
  f.ln("    while(!ok && spin < cg_limit) begin");
  if(tl) {
    f.ln("      ok = " + n + "_d_valid;");
    f.ln("      if(ok) r = word_t'(" + n + "_d_data);");
  } else {
    f.ln("      ok = " + n + "_rvalid;");
    f.ln("      if(ok) r = word_t'(" + n + "_rdata);");
  }
  f.ln("      if(!ok) @(negedge clk);");
  f.ln("      spin = spin + 1;");
  f.ln("    end");
  f.ln("    if(spin >= cg_limit) begin");
  f.ln("      cg_fail = cg_fail + 1;");
  f.ln("      $display(\"FAIL %0t drv_" + n +
       " no response\", $time);");
  f.ln("    end");
  if(tl) {
    f.ln("    @(negedge clk);");
    f.ln("    " + n + "_d_ready = 1'b0;");
  }
  f.ln("  endtask");
  f.ln();
}

// --------------------------------------------------------------------
void RtlTb::unit_tb(SvFile &f, const NodeCtx &c)
{
  std::vector<const NodeCtx::Iface *> sl = c.slaves();
  std::vector<const NodeCtx::Iface *> ms = c.masters();

  f.note("Unit testbench for node '" + c.name() + "'.");
  f.note("");
  f.note("The node under test, a driver on each of its " +
         std::to_string(sl.size()) + " slave");
  f.note("interface" + std::string(sl.size() == 1 ? "" : "s") +
         " and a responder on its downstream link. Every port list");
  f.note("here comes from the same link bundle the RTL came from, so");
  f.note("this testbench cannot be driving a different design from "
         "the");
  f.note("one that was emitted.");
  f.bar();
  RtlPkg::import_of(f, { c.pkg(), RtlPkg::tl_pkg_name() });
  f.ln("module " + c.mod("tb") + ";");
  f.ln();
  f.ln("  logic clk;");
  f.ln("  logic rstn;");
  f.ln();
  f.ln("  // Clock period is simulation control, not configuration,");
  f.ln("  // D-41. It is a build time parameter, overridable with");
  f.ln("  // -GHalfPeriod=<n>, and never an input field. A run time");
  f.ln("  // variable here would make the delay statically unknown.");
  f.ln("  parameter int unsigned HalfPeriod = 5;");
  f.ln();
  f.ln("  initial begin");
  f.ln("    clk = 1'b0;");
  f.ln("    forever #HalfPeriod clk = ~clk;");
  f.ln("  end");
  f.ln();

  for(const NodeCtx::Iface &i : c.ifaces()) {
    f.ln("  // interface '" + i.name + "', link '" + i.link + "'");
    f.lines(RtlCache::iface_wires(i, i.name));
    f.ln();
  }

  f.ln("`include \"cgen_tb_tasks.svh\"");
  f.ln();

  f.ln("  " + c.mod() + " u_dut (");
  f.ln("    .clk  (clk),");
  f.ln("    .rstn (rstn),");
  for(size_t k = 0; k < c.ifaces().size(); ++k) {
    f.lines(RtlCache::iface_conn(c.ifaces()[k], c.ifaces()[k].name,
                                 k + 1 == c.ifaces().size()));
  }
  f.ln("  );");
  f.ln();

  for(const NodeCtx::Iface *ip : ms) {
    f.ln("  " + c.mod("tb_mem") + " u_" + ip->name + "_mem (");
    f.ln("    .clk  (clk),");
    f.ln("    .rstn (rstn),");
    f.lines(RtlCache::iface_conn(*ip, ip->name, true));
    f.ln("  );");
    f.ln();
  }

  for(const NodeCtx::Iface *ip : sl) drv_task(f, c, *ip);

  f.ln("`include \"" + c.mod("tests") + ".svh\"");
  f.ln();
  // ------------------------------------------------------------------
  // The testbench sits at the OTHER end of every slave interface, so
  // it owns exactly the signals the node does not. Driving that set
  // from the bundle rather than by hand is what keeps a TL-C link's
  // unused channels driven instead of floating.
  // ------------------------------------------------------------------
  f.ln("  // Everything the node does not drive on a slave interface");
  f.ln("  // is driven here. The channels this design never runs are");
  f.ln("  // held at zero rather than left floating, R-5.");
  f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
  f.ln("  wire unused_tb = |{");
  {
    std::string acc;
    for(const NodeCtx::Iface *ip : sl) {
      for(const LinkSig::Sig &g : ip->sig.sigs()) {
        if(g.m_drives != ip->master) continue;   // testbench drives it
        if(!acc.empty()) acc += ",\n";
        acc += "      " + LinkSig::wire(ip->name, g.local);
      }
    }
    if(acc.empty()) acc = "      1'b0";
    f.ln(acc);
  }
  f.ln("  };");
  f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
  f.ln();

  f.ln("  initial begin");
  f.ln("    cg_init();");
  for(const NodeCtx::Iface *ip : sl) {
    for(const LinkSig::Sig &g : ip->sig.sigs()) {
      if(g.m_drives == ip->master) continue;   // the node owns it
      f.ln("    " + LinkSig::wire(ip->name, g.local) + " = " +
           (g.bits == 1 ? "1'b0" : "'0") + ";");
    }
  }
  f.ln("    cg_reset(rstn);");
  f.ln("    run_tests();");
  f.ln("    cg_tick(8);");
  f.ln("    void'(cg_report(\"" + c.mod("tb") + "\"));");
  f.ln("    $finish;");
  f.ln("  end");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
// The self checking tests of one node.
// --------------------------------------------------------------------
void RtlTb::unit_tests(SvFile &f, const NodeCtx &c)
{
  const NodeCtx::Iface *s0 = c.slaves().empty() ? nullptr
                                                : c.slaves()[0];
  const std::string d0 = s0 ? "drv_" + s0->name : "";
  const bool wr    = c.has_writes();
  const bool cache = c.is_cache();
  const bool bank  = c.geom().banks > 1 && c.geom().bank_resolved;

  f.note("Self checking tests for node '" + c.name() + "'.");
  f.note("");
  f.note("`include this INSIDE the testbench module. The tests use "
         "the");
  f.note("driver task the testbench declares and the address");
  f.note("decomposition the node's own package declares, so a test");
  f.note("cannot disagree with the design about where a field is.");
  f.note("");
  f.note("Every comparison is written with == inside cg_check rather");
  f.note("than through cg_check_eq, because a word on this node is " +
         std::to_string(c.core_data_bits()) + " bits");
  f.note("and cg_check_eq carries 64. A width safe compare is exact");
  f.note("and a truncated one is not.");
  f.bar();
  f.ln();

  if(s0 == nullptr) {
    f.ln("  task automatic run_tests();");
    f.ln("    $display(\"node '" + c.name() +
         "' has no slave interface to drive\");");
    f.ln("  endtask");
    return;
  }

  f.ln("  task automatic run_tests();");
  f.ln("    addr_t a;");
  f.ln("    word_t r0;");
  f.ln("    word_t r1;");
  const bool t5 = wr || c.is_memory();
  if(t5) f.ln("    word_t wv;");
  if(cache) {
    f.ln("    int unsigned t0;");
    f.ln("    int unsigned t1;");
  }
  f.ln();

  // ------------------------------------------------------------------
  // the address decomposition, on every node that has one
  // ------------------------------------------------------------------
  f.ln("    // -----------------------------------------------------");
  f.ln("    // T1. The address decomposition agrees with itself. A");
  f.ln("    // line address rebuilt from its own fields is the line");
  f.ln("    // it came from. D-39, one source for every consumer.");
  f.ln("    // -----------------------------------------------------");
  f.ln("    a = addr_t'(32'h0001_2340);");
  if(bank) {
    f.ln("    cg_check(\"line_addr rebuilds the line it came from\",");
    f.ln("             line_addr(tag_of(a), bank_of(a), set_of(a))");
    f.ln("             == line_base(a));");
  } else {
    f.ln("    cg_check(\"line_addr rebuilds the line it came from\",");
    f.ln("             line_addr(tag_of(a), set_of(a))");
    f.ln("             == line_base(a));");
  }
  f.ln("    cg_check(\"the offset is what line_base took off\",");
  f.ln("             (line_base(a) | addr_t'(offset_of(a))) == a);");
  f.ln();

  if(cache) {
    f.ln("    // ---------------------------------------------------");
    f.ln("    // T2. The replacement encoding, checked against the");
    f.ln("    // package alone. Out of reset every set points at way");
    f.ln("    // 0, which is what makes the fill order below");
    f.ln("    // predictable. Policy is " + c.repl().policy() + ".");
    f.ln("    // ---------------------------------------------------");
    f.ln("    cg_check(\"the reset victim is way 0\",");
    f.ln("             repl_victim(ReplReset) == way_t'(0));");
    f.ln("    cg_check(\"touching a way moves the victim off it\",");
    f.ln("             repl_victim(repl_update(ReplReset, way_t'(0)))");
    f.ln("             != way_t'(0));");
    f.ln();
  }

  // ------------------------------------------------------------------
  // the request path
  // ------------------------------------------------------------------
  f.ln("    // -----------------------------------------------------");
  if(cache) {
    f.ln("    // T3. A cold read misses and the same address then");
    f.ln("    // hits. The hit is checked by data, and by taking");
    f.ln("    // fewer cycles than the miss did.");
  } else {
    f.ln("    // T3. A read reaches the store and comes back, and a");
    f.ln("    // second read of the same address agrees with it.");
  }
  f.ln("    // -----------------------------------------------------");
  f.ln("    a  = addr_t'(32'h0000_1000);");
  if(cache) f.ln("    t0 = cg_cycle;");
  f.ln("    " + d0 + "(a, 1'b0, '0, '0, r0);");
  if(cache) f.ln("    t1 = cg_cycle;");
  f.ln("    " + d0 + "(a, 1'b0, '0, '0, r1);");
  f.ln("    cg_check(\"the second read agrees with the first\",");
  f.ln("             r1 == r0);");
  if(cache) {
    f.ln("    cg_check(\"the hit is quicker than the miss\",");
    f.ln("             (cg_cycle - t1) < (t1 - t0));");
  }
  f.ln();

  if(cache) {
    f.ln("    // ---------------------------------------------------");
    f.ln("    // T4. Every way of one set. Ways lines that share a");
    f.ln("    // set index all fit, so every one of them hits");
    f.ln("    // afterwards. That is the invalid way first tie break");
    f.ln("    // doing its job.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    for(int unsigned w = 0; w < Ways; w++) begin");
    f.ln("      a = addr_t'(32'h0000_2000) +");
    f.ln("          addr_t'(w * (1 << TagLsb));");
    f.ln("      " + d0 + "(a, 1'b0, '0, '0, r0);");
    f.ln("    end");
    f.ln("    for(int unsigned w = 0; w < Ways; w++) begin");
    f.ln("      a  = addr_t'(32'h0000_2000) +");
    f.ln("           addr_t'(w * (1 << TagLsb));");
    f.ln("      t0 = cg_cycle;");
    f.ln("      " + d0 + "(a, 1'b0, '0, '0, r0);");
    f.ln("      cg_check(\"every way of the set still hits\",");
    f.ln("               (cg_cycle - t0) < 16);");
    f.ln("    end");
    f.ln();
  }

  if(t5) {
    f.ln("    // ---------------------------------------------------");
    f.ln("    // T5. A write is visible to the read that follows it.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    a  = addr_t'(32'h0000_3000);");
    f.ln("    wv = word_t'(64'h0badc0de_5a5a5a5a);");
    f.ln("    " + d0 + "(a, 1'b1, wv, '1, r0);");
    f.ln("    " + d0 + "(a, 1'b0, '0, '0, r1);");
    f.ln("    cg_check(\"a read sees the write before it\",");
    f.ln("             r1 == wv);");
    f.ln();
  }

  f.ln("  endtask");
}

// --------------------------------------------------------------------
void RtlTb::sys_tb(SvFile &f, const Model &m,
                   const std::map<std::string, NodeCtx> &nodes,
                   const std::string &sys)
{
  std::vector<const NodeCtx *> ag;
  for(const Model::Node &n : m.nodes) {
    auto it = nodes.find(n.name);
    if(it != nodes.end() && it->second.is_agent()) {
      ag.push_back(&it->second);
    }
  }

  f.note("Top level testbench for system '" + sys + "'.");
  f.note("");
  f.note("The whole design, every node and every edge, driven "
         "through");
  f.note("the command ports of its " + std::to_string(ag.size()) +
         " agents. A request from an agent");
  f.note("crosses its L1, the L2 and the memory model and comes back.");
  f.bar();
  f.ln("// No package is imported here, for the reason the system");
  f.ln("// top gives: every width is a number cgen derived.");
  f.ln();

  f.ln("module " + sys + "_tb;");
  f.ln();
  f.ln("  logic clk;");
  f.ln("  logic rstn;");
  f.ln();
  f.ln("  parameter int unsigned HalfPeriod = 5;");
  f.ln();
  f.ln("  initial begin");
  f.ln("    clk = 1'b0;");
  f.ln("    forever #HalfPeriod clk = ~clk;");
  f.ln("  end");
  f.ln();
  for(const NodeCtx *a : ag) {
    f.ln("  // command port of agent '" + a->name() + "'");
    f.lines(RtlAgent::cmd_wires(*a, a->name()));
    f.ln();
  }
  f.ln("`include \"cgen_tb_tasks.svh\"");
  f.ln();
  f.ln("  " + sys + "_top u_dut (");
  f.ln("    .clk  (clk),");
  f.ln("    .rstn (rstn),");
  for(size_t k = 0; k < ag.size(); ++k) {
    const std::string p = ag[k]->name();
    const bool last = (k + 1 == ag.size());
    f.ln("    ." + p + "_go       (" + p + "_go),");
    f.ln("    ." + p + "_go_write (" + p + "_go_write),");
    f.ln("    ." + p + "_go_addr  (" + p + "_go_addr),");
    f.ln("    ." + p + "_go_wdata (" + p + "_go_wdata),");
    f.ln("    ." + p + "_go_wstrb (" + p + "_go_wstrb),");
    f.ln("    ." + p + "_busy     (" + p + "_busy),");
    f.ln("    ." + p + "_done     (" + p + "_done),");
    f.ln("    ." + p + "_rdata    (" + p + "_rdata)" +
         (last ? "" : ","));
  }
  f.ln("  );");
  f.ln();

  // one task per agent, all the same shape
  for(const NodeCtx *a : ag) {
    const std::string p = a->name();
    const int w = a->ifaces()[0].sig.data_bits();
    f.ln("  // one request through agent '" + p + "'");
    f.ln("  task automatic go_" + p + "(input logic [" +
         i2s(a->ifaces()[0].sig.addr_bits() - 1) + ":0] a,");
    f.ln("                     input logic wr,");
    f.ln("                     input logic [" + i2s(w - 1) +
         ":0] d,");
    f.ln("                     output logic [" + i2s(w - 1) +
         ":0] r);");
    f.ln("    logic ok;");
    f.ln("    int unsigned spin;");
    f.ln("    // negedge throughout, see the note on cg_wait");
    f.ln("    @(negedge clk);");
    f.ln("    " + p + "_go       = 1'b1;");
    f.ln("    " + p + "_go_write = wr;");
    f.ln("    " + p + "_go_addr  = a;");
    f.ln("    " + p + "_go_wdata = d;");
    f.ln("    " + p + "_go_wstrb = '1;");
    f.ln("    ok   = 1'b0;");
    f.ln("    spin = 0;");
    f.ln("    while(!ok && spin < cg_limit) begin");
    f.ln("      @(negedge clk);");
    f.ln("      ok   = " + p + "_done;");
    f.ln("      spin = spin + 1;");
    f.ln("    end");
    f.ln("    r = " + p + "_rdata;");
    f.ln("    " + p + "_go = 1'b0;");
    f.ln("    if(spin >= cg_limit) begin");
    f.ln("      cg_fail = cg_fail + 1;");
    f.ln("      $display(\"FAIL %0t go_" + p +
         " never completed\", $time);");
    f.ln("    end");
    f.ln("    spin = 0;");
    f.ln("    while(" + p + "_busy && spin < cg_limit) begin");
    f.ln("      @(negedge clk);");
    f.ln("      spin = spin + 1;");
    f.ln("    end");
    f.ln("  endtask");
    f.ln();
  }

  f.ln("`include \"" + sys + "_tests.svh\"");
  f.ln();
  f.ln("  initial begin");
  f.ln("    cg_init();");
  for(const NodeCtx *a : ag) {
    const std::string p = a->name();
    f.ln("    " + p + "_go       = 1'b0;");
    f.ln("    " + p + "_go_write = 1'b0;");
    f.ln("    " + p + "_go_addr  = '0;");
    f.ln("    " + p + "_go_wdata = '0;");
    f.ln("    " + p + "_go_wstrb = '0;");
  }
  f.ln("    cg_reset(rstn);");
  f.ln("    run_tests();");
  f.ln("    cg_tick(16);");
  f.ln("    void'(cg_report(\"" + sys + "_tb\"));");
  f.ln("    $finish;");
  f.ln("  end");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
void RtlTb::sys_tests(SvFile &f, const Model &m,
                      const std::map<std::string, NodeCtx> &nodes,
                      const std::string &sys)
{
  std::vector<const NodeCtx *> ag;
  for(const Model::Node &n : m.nodes) {
    auto it = nodes.find(n.name);
    if(it != nodes.end() && it->second.is_agent()) {
      ag.push_back(&it->second);
    }
  }

  f.note("Self checking tests for system '" + sys + "'.");
  f.note("");
  f.note("`include this INSIDE the system testbench module.");
  f.bar();
  f.ln();
  f.ln("  task automatic run_tests();");
  for(const NodeCtx *a : ag) {
    const int w = a->ifaces()[0].sig.data_bits();
    f.ln("    logic [" + i2s(w - 1) + ":0] " + a->name() + "_r0;");
    f.ln("    logic [" + i2s(w - 1) + ":0] " + a->name() + "_r1;");
  }
  f.ln("    int unsigned t0;");
  f.ln("    int unsigned t1;");
  f.ln();

  for(const NodeCtx *a : ag) {
    const std::string p = a->name();
    f.ln("    // ---------------------------------------------------");
    f.ln("    // agent '" + p + "': a cold read reaches memory and "
         "the");
    f.ln("    // same address then hits in its L1.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    t0 = cg_cycle;");
    f.ln("    go_" + p + "(32'h0000_4000, 1'b0, '0, " + p + "_r0);");
    f.ln("    t1 = cg_cycle;");
    f.ln("    go_" + p + "(32'h0000_4000, 1'b0, '0, " + p + "_r1);");
    f.ln("    cg_check_eq(\"" + p +
         " hit returns what the miss filled\",");
    f.ln("                {32'd0, " + p + "_r1}, {32'd0, " + p +
         "_r0});");
    f.ln("    cg_check(\"" + p + " hit is quicker than the miss\",");
    f.ln("             (cg_cycle - t1) < (t1 - t0));");
    f.ln("    cg_check(\"" + p +
         " went idle after its last request\",");
    f.ln("             !" + p + "_busy);");
    f.ln();
  }

  if(ag.size() >= 2) {
    f.ln("    // ---------------------------------------------------");
    f.ln("    // Both agents against one address. They run through");
    f.ln("    // different L1s into the same L2, which is where the");
    f.ln("    // round robin between up_i and up_d gets exercised.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    go_" + ag[0]->name() + "(32'h0000_5000, 1'b0, '0, " +
         ag[0]->name() + "_r0);");
    f.ln("    go_" + ag[1]->name() + "(32'h0000_5000, 1'b0, '0, " +
         ag[1]->name() + "_r0);");
    f.ln("    cg_check(\"both agents completed against one line\",");
    f.ln("             1'b1);");
    f.ln();
  }

  f.ln("  endtask");
}

// --------------------------------------------------------------------
// A responder on the ad hoc processor port, so that an agent has
// something to talk to in its own unit testbench.
// --------------------------------------------------------------------
void RtlTb::tb_slv(SvFile &f, const NodeCtx &c,
                   const NodeCtx::Iface &i)
{
  const std::string n = i.name;

  f.note("A responder for the unit testbench of agent '" + c.name() +
         "'.");
  f.note("");
  f.note("NOT SYNTHESIZABLE and not part of the design. It answers");
  f.note("link '" + i.link + "' so the agent has somewhere to send a");
  f.note("request. A word nobody has written reads back as its own");
  f.note("address, so a test can predict it.");
  f.bar();
  RtlPkg::import_of(f, { c.pkg() });
  f.ln("module " + c.mod("tb_slv") + " (");
  f.ln("  input  logic  clk,");
  f.ln("  input  logic  rstn,");
  f.ln();
  std::vector<std::string> pv = RtlCache::iface_ports(i, true);
  for(std::string &s : pv) {
    if(s.compare(2, 6, "output") == 0)     s.replace(2, 6, "input ");
    else if(s.compare(2, 5, "input") == 0) s.replace(2, 5, "output");
  }
  f.lines(pv);
  f.ln(");");
  f.ln();
  f.ln("  logic [WordBits-1:0] store [longint unsigned];");
  f.ln();
  f.ln("  wire fire = " + LinkSig::wire(n, "valid") + " && " +
       LinkSig::wire(n, "ready") + ";");
  f.ln();
  f.ln("  assign " + LinkSig::wire(n, "ready") + " = 1'b1;");
  f.ln();
  f.ln("  function automatic longint unsigned key_of");
  f.ln("      (input logic [AddrBits-1:0] a);");
  f.ln("    key_of = longint'({32'd0, a}) >> $clog2(WordBytes);");
  f.ln("  endfunction");
  f.ln();
  f.ln("  // The store is an associative array, and IEEE 1800-2023");
  f.ln("  // 6.21 forbids a nonblocking assignment to a dynamically");
  f.ln("  // sized variable. Its writes are therefore blocking and");
  f.ln("  // BLKSEQ is turned off across this process alone.");
  f.ln("  /* verilator lint_off BLKSEQ */");
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      " + LinkSig::wire(n, "rvalid") + " <= 1'b0;");
  f.ln("      " + LinkSig::wire(n, "rdata")  + " <= '0;");
  f.ln("      store.delete();");
  f.ln("    end else begin");
  f.ln("      " + LinkSig::wire(n, "rvalid") + " <= 1'b0;");
  f.ln("      if(fire) begin");
  f.ln("        if(" + LinkSig::wire(n, "rw") + ") begin");
  f.ln("          store[key_of(" + LinkSig::wire(n, "addr") + ")] =");
  f.ln("              " + LinkSig::wire(n, "wdata") + ";");
  f.ln("        end else begin");
  f.ln("          " + LinkSig::wire(n, "rvalid") + " <= 1'b1;");
  f.ln("          " + LinkSig::wire(n, "rdata") + " <=");
  f.ln("              store.exists(key_of(" +
       LinkSig::wire(n, "addr") + "))");
  f.ln("              ? store[key_of(" + LinkSig::wire(n, "addr") +
       ")]");
  f.ln("              : " + LinkSig::wire(n, "addr") + ";");
  f.ln("        end");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln("  /* verilator lint_on BLKSEQ */");
  f.ln();
  f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
  f.ln("  wire unused = |{" + LinkSig::wire(n, "wstrb") + "};");
  f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
// An agent's unit testbench. An agent drives rather than answers, so
// the responder is on the other side and the testbench works the
// command port instead of a link.
// --------------------------------------------------------------------
void RtlTb::agent_tb(SvFile &f, const NodeCtx &c)
{
  const NodeCtx::Iface &i = c.ifaces()[0];

  f.note("Unit testbench for agent '" + c.name() + "'.");
  f.note("");
  f.note("The agent under test and a responder on link '" + i.link +
         "'.");
  f.note("The agent is the master, so the testbench works its command");
  f.note("port and the responder answers the link.");
  f.bar();
  RtlPkg::import_of(f, { c.pkg() });
  f.ln("module " + c.mod("tb") + ";");
  f.ln();
  f.ln("  logic clk;");
  f.ln("  logic rstn;");
  f.ln();
  f.ln("  parameter int unsigned HalfPeriod = 5;");
  f.ln();
  f.ln("  initial begin");
  f.ln("    clk = 1'b0;");
  f.ln("    forever #HalfPeriod clk = ~clk;");
  f.ln("  end");
  f.ln();
  f.lines(RtlAgent::cmd_wires(c, "cmd"));
  f.ln();
  f.ln("  // interface '" + i.name + "', link '" + i.link + "'");
  f.lines(RtlCache::iface_wires(i, i.name));
  f.ln();
  f.ln("`include \"cgen_tb_tasks.svh\"");
  f.ln();
  f.ln("  " + c.mod() + " u_dut (");
  f.ln("    .clk          (clk),");
  f.ln("    .rstn         (rstn),");
  f.ln("    .cmd_go       (cmd_go),");
  f.ln("    .cmd_go_write (cmd_go_write),");
  f.ln("    .cmd_go_addr  (cmd_go_addr),");
  f.ln("    .cmd_go_wdata (cmd_go_wdata),");
  f.ln("    .cmd_go_wstrb (cmd_go_wstrb),");
  f.ln("    .cmd_busy     (cmd_busy),");
  f.ln("    .cmd_done     (cmd_done),");
  f.ln("    .cmd_rdata    (cmd_rdata),");
  f.lines(RtlCache::iface_conn(i, i.name, true));
  f.ln("  );");
  f.ln();
  f.ln("  " + c.mod("tb_slv") + " u_slv (");
  f.ln("    .clk  (clk),");
  f.ln("    .rstn (rstn),");
  f.lines(RtlCache::iface_conn(i, i.name, true));
  f.ln("  );");
  f.ln();
  f.ln("  // one request through the command port");
  f.ln("  task automatic go(input logic [AddrBits-1:0] a,");
  f.ln("                    input logic wr,");
  f.ln("                    input logic [WordBits-1:0] d,");
  f.ln("                    output logic [WordBits-1:0] r);");
  f.ln("    logic ok;");
  f.ln("    int unsigned spin;");
  f.ln("    // negedge throughout, see the note on cg_wait");
  f.ln("    @(negedge clk);");
  f.ln("    cmd_go       = 1'b1;");
  f.ln("    cmd_go_write = wr;");
  f.ln("    cmd_go_addr  = a;");
  f.ln("    cmd_go_wdata = d;");
  f.ln("    cmd_go_wstrb = '1;");
  f.ln("    ok   = 1'b0;");
  f.ln("    spin = 0;");
  f.ln("    while(!ok && spin < cg_limit) begin");
  f.ln("      @(negedge clk);");
  f.ln("      ok   = cmd_done;");
  f.ln("      spin = spin + 1;");
  f.ln("    end");
  f.ln("    r      = cmd_rdata;");
  f.ln("    cmd_go = 1'b0;");
  f.ln("    if(spin >= cg_limit) begin");
  f.ln("      cg_fail = cg_fail + 1;");
  f.ln("      $display(\"FAIL %0t go never completed\", $time);");
  f.ln("    end");
  f.ln("    // the agent leaves done only once the command is");
  f.ln("    // withdrawn, so the request is not over until it is idle");
  f.ln("    spin = 0;");
  f.ln("    while(cmd_busy && spin < cg_limit) begin");
  f.ln("      @(negedge clk);");
  f.ln("      spin = spin + 1;");
  f.ln("    end");
  f.ln("  endtask");
  f.ln();
  f.ln("`include \"" + c.mod("tests") + ".svh\"");
  f.ln();
  f.ln("  initial begin");
  f.ln("    cg_init();");
  f.ln("    cmd_go       = 1'b0;");
  f.ln("    cmd_go_write = 1'b0;");
  f.ln("    cmd_go_addr  = '0;");
  f.ln("    cmd_go_wdata = '0;");
  f.ln("    cmd_go_wstrb = '0;");
  f.ln("    cg_reset(rstn);");
  f.ln("    run_tests();");
  f.ln("    cg_tick(8);");
  f.ln("    void'(cg_report(\"" + c.mod("tb") + "\"));");
  f.ln("    $finish;");
  f.ln("  end");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
void RtlTb::agent_tests(SvFile &f, const NodeCtx &c)
{
  f.note("Self checking tests for agent '" + c.name() + "'.");
  f.bar();
  f.ln();
  f.ln("  task automatic run_tests();");
  f.ln("    logic [AddrBits-1:0] a;");
  f.ln("    logic [WordBits-1:0] r0;");
  f.ln("    logic [WordBits-1:0] r1;");
  f.ln();
  f.ln("    // an unwritten word reads back as its own address");
  f.ln("    a = AddrBits'(32'h0000_1000);");
  f.ln("    go(a, 1'b0, '0, r0);");
  f.ln("    cg_check(\"a read returns the responder's seed\",");
  f.ln("             r0 == WordBits'(a));");
  f.ln();
  f.ln("    // a write is visible to the read that follows it");
  f.ln("    go(a, 1'b1, WordBits'(32'hdead_beef), r1);");
  f.ln("    go(a, 1'b0, '0, r1);");
  f.ln("    cg_check(\"a read sees the write before it\",");
  f.ln("             r1 == WordBits'(32'hdead_beef));");
  f.ln();
  f.ln("    cg_check(\"the agent went idle after the last request\",");
  f.ln("             !cmd_busy);");
  f.ln("  endtask");
}

} // namespace cgen
