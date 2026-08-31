// --------------------------------------------------------------------
// FILE:    rtl_tb.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "rtl_tb.h"
#include "feature_table.h"
#include "rtl_agent.h"
#include "rtl_cache.h"
#include "rtl_pkg.h"
#include <algorithm>

namespace cgen
{

namespace {
std::string i2s(int v) { return std::to_string(v); }

// ------------------------------------------------------------------
// R-8. One registration, from the emitter of a test to the feature
// table. The test names the node it drives and the FIELD it
// exercises, so a test that stops being emitted stops claiming the
// field, in the same change.
// ------------------------------------------------------------------
void unit_covers(const NodeCtx &c, const std::string &rel,
                 const std::string &test)
{
  cov_test(c.name(), rel, Features::Level::Unit,
           c.mod("tb"), test);
}

void top_covers(const std::string &node, const std::string &rel,
                const std::string &bench, const std::string &test)
{
  cov_test(node, rel, Features::Level::Top, bench, test);
}

// ------------------------------------------------------------------
// The node one edge leads to, empty when the node has no downstream
// edge. The whole chain from an agent to memory is walked this way,
// so the tests are laid out by the topology and not by a guess about
// what a cache hierarchy looks like.
// ------------------------------------------------------------------
std::string downstream(const Model &m, const std::string &from)
{
  for(const Model::Edge &e : m.edges) {
    if(e.from == from) return e.to;
  }
  return "";
}

const NodeCtx *ctx_of(const std::map<std::string, NodeCtx> &nodes,
                      const std::string &name)
{
  auto it = nodes.find(name);
  return it == nodes.end() ? nullptr : &it->second;
}

// hex literal of an address, 32'h0001_0000 style
std::string addr_lit(int bits, uint64_t v)
{
  static const char *dig = "0123456789abcdef";
  const int n = (bits + 3) / 4;
  std::string h;
  for(int k = n - 1; k >= 0; --k) {
    h += dig[(v >> (4 * k)) & 0xf];
    if(k && (k % 4) == 0) h += '_';
  }
  return i2s(bits) + "'h" + h;
}

} // namespace

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
// THE RESPONDER OF A PIPELINED NODE'S UNIT TESTBENCH. The node under
// test may have one fill in flight per miss handling register, so a
// responder that holds one at a time cannot show what it does. This
// one holds one per source and lets a test choose the order the
// beats come back in.
//
//   tb_hold   channel A is refused, so a fill cannot start
//   tb_dhold  channel D is held, so fills accumulate and none
//             completes
//   tb_ilv    beats are handed out round robin, so the beats of two
//             fills INTERLEAVE
//   tb_rev    the HIGHEST numbered pending source is served first,
//             so a fill issued later completes before one issued
//             earlier
//
// With none of them set it serves the lowest numbered pending fill
// and keeps the link until that line is done, which is the ordinary
// case and is deterministic.
//
// READ ONLY. A pipelined node here is one that does not write, so
// this responder answers Get and reports anything else rather than
// carrying a store it can never be asked to fill.
// --------------------------------------------------------------------
void RtlTb::tb_mem_pipe(SvFile &f, const NodeCtx &c,
                        const NodeCtx::Iface &i)
{
  const std::string mod = c.mod("tb_mem");
  const int shift = Replacement::log2i(i.sig.data_bytes());
  int src_bits = 0;
  for(const LinkSig::Sig &g : i.sig.sigs()) {
    if(g.local == "a_source") src_bits = g.bits;
  }
  if(src_bits <= 0) src_bits = 1;
  const int nsrc = 1 << src_bits;

  f.note("A downstream responder for the unit testbench of node '" +
         c.name() + "'.");
  f.note("");
  f.note("NOT SYNTHESIZABLE and not part of the design. Beats come "
         "back");
  f.note("with the address baked into them, so a test can predict "
         "what a");
  f.note("fill should contain without keeping its own model.");
  f.note("");
  f.note("IT HOLDS ONE FILL PER SOURCE, " + i2s(nsrc) +
         " of them, because the node");
  f.note("under test may have that many in flight. A master holds at "
         "most");
  f.note("one request per source, so the source IS the index and no");
  f.note("search is needed.");
  f.note("");
  f.note("Four inputs let a test choose what the memory side does:");
  f.note("");
  f.note("  tb_hold   channel A is refused, so no fill can start");
  f.note("  tb_dhold  channel D is held, so fills accumulate and");
  f.note("            none of them completes");
  f.note("  tb_ilv    beats go out round robin, so the beats of two");
  f.note("            fills INTERLEAVE");
  f.note("  tb_rev    the HIGHEST numbered pending source is served");
  f.note("            first, so a fill issued later completes first");
  f.note("");
  f.note("With none of them set it serves the lowest numbered "
         "pending");
  f.note("fill and keeps the link until that line is done.");
  f.bar();
  RtlPkg::import_of(f, { c.pkg(), RtlPkg::tl_pkg_name() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic  clk,");
  f.ln("  input  logic  rstn,");
  f.ln("  input  logic  tb_hold,");
  f.ln("  input  logic  tb_dhold,");
  f.ln("  input  logic  tb_ilv,");
  f.ln("  input  logic  tb_rev,");
  f.ln();
  std::vector<std::string> pv = RtlCache::iface_ports(i, true);
  for(std::string &s : pv) {
    if(s.compare(2, 6, "output") == 0)     s.replace(2, 6, "input ");
    else if(s.compare(2, 5, "input") == 0) s.replace(2, 5, "output");
  }
  f.lines(pv);
  f.ln(");");
  f.ln();

  const std::string a = i.name + "_a";
  const std::string d = i.name + "_d";
  const std::string sb = i2s(src_bits);

  f.ln("  localparam int unsigned BeatShift = " + i2s(shift) + ";");
  f.ln("  localparam int unsigned NSrc      = " + i2s(nsrc) + ";");
  f.ln();
  f.ln("  typedef logic [" + i2s(src_bits - 1) + ":0] src_t;");
  f.ln();
  f.ln("  // one pending fill per source");
  f.ln("  logic        p_val  [NSrc];");
  f.ln("  addr_t       p_addr [NSrc];");
  f.ln("  logic [2:0]  p_size [NSrc];");
  f.ln("  logic [15:0] p_beat [NSrc];");
  f.ln("  logic [15:0] p_beats[NSrc];");
  f.ln();
  f.ln("  // where the round robin walk starts, so consecutive beats");
  f.ln("  // go to different fills when tb_ilv is set");
  f.ln("  src_t rr;");
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
  f.ln("  // a beat carries its own key, so a test can predict a fill");
  f.ln("  // without keeping a second model. The tests file carries");
  f.ln("  // the same two functions, emitted from the same place.");
  f.ln("  function automatic beat_t seed_of"
       "(input longint unsigned k);");
  f.ln("    seed_of = beat_t'(k) ^ beat_t'(64'hcafe_0000_0000_0001);");
  f.ln("  endfunction");
  f.ln();
  f.ln("  wire a_fire = " + a + "_valid && " + a + "_ready;");
  f.ln("  wire d_fire = " + d + "_valid && " + d + "_ready;");
  f.ln();
  f.ln("  // ---------------------------------------------------------");
  f.ln("  // CHANNEL A. A request is taken unless the source it names");
  f.ln("  // already has a fill pending, so several may be in flight");
  f.ln("  // and nothing here serialises them.");
  f.ln("  // ---------------------------------------------------------");
  f.ln("  assign " + a + "_ready = rstn && !tb_hold &&");
  f.ln("                            !p_val[" + a + "_source];");
  f.ln();
  f.ln("  // ---------------------------------------------------------");
  f.ln("  // CHANNEL D. Which pending fill gets this cycle's beat.");
  f.ln("  // ---------------------------------------------------------");
  f.ln("  logic d_any;");
  f.ln("  src_t d_sel;");
  f.ln();
  f.ln("  always_comb begin");
  f.ln("    d_any = 1'b0;");
  f.ln("    d_sel = '0;");
  f.ln("    if(tb_ilv) begin");
  f.ln("      // start one past the source the last beat went to");
  f.ln("      for(int unsigned k = 0; k < NSrc; k++) begin");
  f.ln("        automatic src_t s = src_t'(");
  f.ln("            (int'(rr) + int'(k) >= int'(NSrc))");
  f.ln("            ? (int'(rr) + int'(k) - int'(NSrc))");
  f.ln("            : (int'(rr) + int'(k)));");
  f.ln("        if(!d_any && p_val[s]) begin");
  f.ln("          d_any = 1'b1;");
  f.ln("          d_sel = s;");
  f.ln("        end");
  f.ln("      end");
  f.ln("    end else if(tb_rev) begin");
  f.ln("      // counting up leaves the HIGHEST pending source");
  f.ln("      for(int unsigned s = 0; s < NSrc; s++) begin");
  f.ln("        if(p_val[s]) begin");
  f.ln("          d_any = 1'b1;");
  f.ln("          d_sel = src_t'(s);");
  f.ln("        end");
  f.ln("      end");
  f.ln("    end else begin");
  f.ln("      // counting down leaves the LOWEST pending source");
  f.ln("      for(int unsigned s = NSrc; s > 0; s--) begin");
  f.ln("        if(p_val[s-1]) begin");
  f.ln("          d_any = 1'b1;");
  f.ln("          d_sel = src_t'(s-1);");
  f.ln("        end");
  f.ln("      end");
  f.ln("    end");
  f.ln();
  f.ln("    // unless a test asked for interleaving, a line that has");
  f.ln("    // beats out already keeps the link until it is done");
  f.ln("    if(!tb_ilv) begin");
  f.ln("      for(int unsigned s = NSrc; s > 0; s--) begin");
  f.ln("        if(p_val[s-1] && (p_beat[s-1] != 16'd0)) begin");
  f.ln("          d_any = 1'b1;");
  f.ln("          d_sel = src_t'(s-1);");
  f.ln("        end");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();
  f.ln("  assign " + d + "_valid   = d_any && !tb_dhold;");
  f.ln("  assign " + d + "_opcode  = TlDAccessAckData;");
  f.ln("  assign " + d + "_param   = TlParamZero;");
  f.ln("  assign " + d + "_size    = p_size[d_sel];");
  f.ln("  assign " + d + "_source  = " + sb + "'(d_sel);");
  f.ln("  assign " + d + "_sink    = '0;");
  f.ln("  assign " + d + "_denied  = 1'b0;");
  f.ln("  assign " + d + "_corrupt = 1'b0;");
  f.ln("  assign " + d + "_data    =");
  f.ln("      seed_of(key_of(p_addr[d_sel], p_beat[d_sel]));");
  f.ln();
  f.ln("  always_ff @(posedge clk or negedge rstn) begin");
  f.ln("    if(!rstn) begin");
  f.ln("      for(int unsigned s = 0; s < NSrc; s++) begin");
  f.ln("        p_val  [s] <= 1'b0;");
  f.ln("        p_addr [s] <= '0;");
  f.ln("        p_size [s] <= 3'd0;");
  f.ln("        p_beat [s] <= 16'd0;");
  f.ln("        p_beats[s] <= 16'd1;");
  f.ln("      end");
  f.ln("      rr <= '0;");
  f.ln("    end else begin");
  f.ln("      // a_fire needs the source free and d_fire needs it");
  f.ln("      // taken, so the two cannot name one source in one");
  f.ln("      // cycle and these writes cannot collide");
  f.ln("      if(a_fire) begin");
  f.ln("        p_val  [" + a + "_source] <= 1'b1;");
  f.ln("        p_addr [" + a + "_source] <= " + a + "_address;");
  f.ln("        p_size [" + a + "_source] <= " + a + "_size;");
  f.ln("        p_beat [" + a + "_source] <= 16'd0;");
  f.ln("        p_beats[" + a + "_source] <= beats_of(" + a +
       "_size);");
  f.ln("      end");
  f.ln();
  f.ln("      if(d_fire) begin");
  f.ln("        rr <= (d_sel == src_t'(NSrc-1))");
  f.ln("              ? src_t'(0) : d_sel + src_t'(1);");
  f.ln("        if(p_beat[d_sel] == p_beats[d_sel] - 16'd1) begin");
  f.ln("          p_beat[d_sel] <= 16'd0;");
  f.ln("          p_val [d_sel] <= 1'b0;");
  f.ln("        end else begin");
  f.ln("          p_beat[d_sel] <= p_beat[d_sel] + 16'd1;");
  f.ln("        end");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln();
  f.ln("  // THIS RESPONDER ANSWERS READS. The node under test is");
  f.ln("  // read only on its core boundary and writes nothing back,");
  f.ln("  // so anything but a Get is a defect in the master rather");
  f.ln("  // than a case to carry.");
  f.ln("  always_ff @(posedge clk) begin");
  f.ln("    if(a_fire && (" + a + "_opcode != TlAGet)) begin");
  f.ln("      $error(\"" + mod + ": opcode %0d is not a Get\",");
  f.ln("             " + a + "_opcode);");
  f.ln("    end");
  f.ln("  end");
  f.ln();
  if(i.sig.has_bce()) {
    f.ln("  // TL-C channels the responder does not run, tied off");
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(g.local.size() < 2 || g.local[1] != '_') continue;
      const char ch = g.local[0];
      if(ch != 'b' && ch != 'c' && ch != 'e') continue;
      if(g.m_drives == i.master) continue;
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
       a + "_data, " + a + "_corrupt};");
  f.ln("  /* verilator lint_on UNUSEDSIGNAL */");
  f.ln();
  f.ln("endmodule");
}

// --------------------------------------------------------------------
// A TileLink responder for a unit testbench. Smaller than the system
// memory model and generated from the same link bundle, so the unit
// testbench of a node exercises the exact port list the node has.
// --------------------------------------------------------------------
void RtlTb::tb_mem(SvFile &f, const NodeCtx &c,
                   const NodeCtx::Iface &i)
{
  // A pipelined node needs a responder that holds many fills at
  // once. See tb_mem_pipe.
  if(c.pipelined()) {
    tb_mem_pipe(f, c, i);
    return;
  }

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
  f.note("");
  f.note("tb_hold STOPS IT TAKING REQUESTS. A test that has to show");
  f.note("what the node does while a fill is outstanding needs the");
  f.note("fill to stay outstanding, and the only way to arrange that");
  f.note("from a test is to hold the responder off. It refuses on");
  f.note("channel A, so nothing is half answered while it is high.");
  f.bar();
  RtlPkg::import_of(f, { c.pkg(), RtlPkg::tl_pkg_name() });
  f.ln("module " + mod + " (");
  f.ln("  input  logic  clk,");
  f.ln("  input  logic  rstn,");
  f.ln("  input  logic  tb_hold,");
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
  f.ln("  assign " + a + "_ready   = !tb_hold &&");
  f.ln("                            ((tstate == T_IDLE) ||");
  f.ln("                             (tstate == T_WDATA));");
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
  f.ln("      (store.exists(key_of(addr_q, beat_i)) != 0)");
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
  const bool nb = !tl && c.nonblocking() && c.core_iface() == &i;
  const std::string n = i.name;
  const std::string vld = tl ? n + "_a_valid" : n + "_valid";
  const std::string rdy = tl ? n + "_a_ready" : n + "_ready";
  const std::string qual = c.reserve_qual();

  // ------------------------------------------------------------------
  // A NON BLOCKING PORT NEEDS MORE THAN ONE DRIVER. The tasks below
  // present a request WITHOUT waiting for its answer, so a test can
  // hold several in flight, and a collector standing on the response
  // port records every answer by the identifier it carries. drv_<n>
  // is then written on top of them and keeps the shape every other
  // node's driver has, so the tests that do not care about ordering
  // are unchanged.
  // ------------------------------------------------------------------
  if(nb) {
    f.ln("  // -------------------------------------------------------");
    f.ln("  // The response collector. IT STANDS ON THE PORT rather");
    f.ln("  // than being called, because a response may arrive for an");
    f.ln("  // identifier no task is currently waiting on, which is");
    f.ln("  // what out of order return means.");
    f.ln("  //");
    f.ln("  // nb_ord is the ARRIVAL ORDER of each identifier's");
    f.ln("  // answer. Two identifiers whose order is the reverse of");
    f.ln("  // the order they were presented in is the whole of the");
    f.ln("  // out of order proof.");
    f.ln("  // -------------------------------------------------------");
    f.ln("  int unsigned nb_n;");
    f.ln("  logic        nb_seen [MaxOutstanding];");
    f.ln("  word_t       nb_data [MaxOutstanding];");
    f.ln("  logic        nb_err  [MaxOutstanding];");
    f.ln("  int unsigned nb_ord  [MaxOutstanding];");
    f.ln("  int unsigned nb_fill;   // fills the responder was asked "
         "for");
    if(c.pipelined()) {
      f.ln();
      f.ln("  // WHEN each identifier was accepted and when it was");
      f.ln("  // answered, in cycles, so a test can MEASURE a latency");
      f.ln("  // rather than assume one.");
      f.ln("  int unsigned nb_acc [MaxOutstanding];");
      f.ln("  int unsigned nb_rsp [MaxOutstanding];");
    }
    f.ln();
    f.ln("  always_ff @(posedge clk or negedge rstn) begin");
    f.ln("    if(!rstn) begin");
    f.ln("      nb_n    <= 0;");
    f.ln("      nb_fill <= 0;");
    f.ln("    end else begin");
    f.ln("      if(" + n + "_rvalid) begin");
    f.ln("        nb_seen[" + n + "_rid] <= 1'b1;");
    f.ln("        nb_data[" + n + "_rid] <= word_t'(" + n +
         "_rdata);");
    if(i.sig.err_ret()) {
      f.ln("        nb_err [" + n + "_rid] <= " + n + "_rerr;");
    }
    f.ln("        nb_ord [" + n + "_rid] <= nb_n;");
    if(c.pipelined()) {
      f.ln("        nb_rsp [" + n + "_rid] <= cg_cycle;");
    }
    f.ln("        nb_n <= nb_n + 1;");
    f.ln("      end");
    if(c.pipelined()) {
      f.ln("      if(" + n + "_valid && " + n + "_ready) begin");
      f.ln("        nb_acc[" + n + "_id] <= cg_cycle;");
      f.ln("      end");
    }
    const NodeCtx::Iface *mp = c.masters().empty() ? nullptr
                                                   : c.masters()[0];
    if(mp != nullptr && mp->sig.is_tl()) {
      f.ln("      // one count of what actually left for memory, so a");
      f.ln("      // merge can be shown to have asked for ONE fill");
      f.ln("      if(" + mp->name + "_a_valid && " + mp->name +
           "_a_ready) begin");
      f.ln("        nb_fill <= nb_fill + 1;");
      f.ln("      end");
    }
    f.ln("    end");
    f.ln("  end");
    f.ln();
    const NodeCtx::Iface *mm = c.masters().empty() ? nullptr
                                                  : c.masters()[0];
    if(c.pipelined() && mm != nullptr && mm->sig.is_tl()) {
      const std::string ma = mm->name + "_a";
      const std::string md = mm->name + "_d";
      f.ln("  // -----------------------------------------------------");
      f.ln("  // THE MEMORY SIDE, COUNTED AT THE LINK. A fill is in");
      f.ln("  // flight from the cycle its A beat is taken to the");
      f.ln("  // cycle ITS last D beat arrives, so this counts what");
      f.ln("  // the node actually has outstanding rather than what");
      f.ln("  // it is allowed to.");
      f.ln("  //");
      f.ln("  // nb_ilv counts the beats that arrived for one fill");
      f.ln("  // WHILE ANOTHER was part way through its own line,");
      f.ln("  // which is the whole of what interleaving is. A");
      f.ln("  // responder that finishes one line before starting the");
      f.ln("  // next leaves it at zero.");
      f.ln("  // -----------------------------------------------------");
      f.ln("  int unsigned nb_flt;    // in flight right now");
      f.ln("  int unsigned nb_pk;     // the most there ever were");
      f.ln("  int unsigned nb_ilv;    // interleaved beats");
      f.ln("  int unsigned nb_bt [Mshrs];  // beats of each fill so far");
      f.ln();
      f.ln("  wire nb_a_fire = " + ma + "_valid && " + ma + "_ready;");
      f.ln("  wire nb_d_fire = " + md + "_valid && " + md + "_ready;");
      f.ln("  wire nb_d_last = nb_d_fire &&");
      f.ln("      (nb_bt[" + md + "_source] == Beats-1);");
      f.ln();
      f.ln("  logic        nb_other;");
      f.ln("  int unsigned nb_flt_nx;");
      f.ln();
      f.ln("  always_comb begin");
      f.ln("    nb_other = 1'b0;");
      f.ln("    for(int unsigned m = 0; m < Mshrs; m++) begin");
      f.ln("      if((m != int'(" + md + "_source)) &&");
      f.ln("         (nb_bt[m] != 0)) nb_other = 1'b1;");
      f.ln("    end");
      f.ln();
      f.ln("    // one net change, so an A and the last D of another");
      f.ln("    // fill in the same cycle do not lose each other");
      f.ln("    nb_flt_nx = nb_flt;");
      f.ln("    if(nb_a_fire) nb_flt_nx = nb_flt_nx + 1;");
      f.ln("    if(nb_d_last) nb_flt_nx = nb_flt_nx - 1;");
      f.ln("  end");
      f.ln();
      f.ln("  always_ff @(posedge clk or negedge rstn) begin");
      f.ln("    if(!rstn) begin");
      f.ln("      nb_flt <= 0;");
      f.ln("      nb_pk  <= 0;");
      f.ln("      nb_ilv <= 0;");
      f.ln("      for(int unsigned m = 0; m < Mshrs; m++) begin");
      f.ln("        nb_bt[m] <= 0;");
      f.ln("      end");
      f.ln("    end else begin");
      f.ln("      nb_flt <= nb_flt_nx;");
      f.ln("      if(nb_flt_nx > nb_pk) nb_pk <= nb_flt_nx;");
      f.ln("      if(nb_d_fire && nb_other) nb_ilv <= nb_ilv + 1;");
      f.ln("      if(nb_d_fire) begin");
      f.ln("        nb_bt[" + md + "_source] <=");
      f.ln("            nb_d_last ? 0 : nb_bt[" + md + "_source] + 1;");
      f.ln("      end");
      f.ln("    end");
      f.ln("  end");
      f.ln();
    }
    f.ln("  // forget every answer so far, so a test starts clean");
    f.ln("  task automatic nb_clear();");
    f.ln("    nb_n    = 0;");
    f.ln("    nb_fill = 0;");
    f.ln("    for(int unsigned k = 0; k < MaxOutstanding; k++) begin");
    f.ln("      nb_seen[k] = 1'b0;");
    f.ln("      nb_data[k] = '0;");
    f.ln("      nb_err [k] = 1'b0;");
    f.ln("      nb_ord [k] = 0;");
    if(c.pipelined()) {
      f.ln("      nb_acc [k] = 0;");
      f.ln("      nb_rsp [k] = 0;");
    }
    f.ln("    end");
    if(c.pipelined() && mm != nullptr && mm->sig.is_tl()) {
      f.ln("    nb_pk  = 0;");
      f.ln("    nb_ilv = 0;");
    }
    f.ln("  endtask");
    f.ln();
    f.ln("  // -------------------------------------------------------");
    f.ln("  // Present one request and return as soon as it has been");
    f.ln("  // ACCEPTED, not when it has been answered. valid is left");
    f.ln("  // standing, so calls back to back are accepted one per");
    f.ln("  // cycle. Call nb_idle when there are no more.");
    f.ln("  // -------------------------------------------------------");
    f.ln("  task automatic nb_req(input addr_t a,");
    f.ln("                        input req_id_t id,");
    f.ln("                        input logic pf,");
    f.ln("                        output logic took);");
    f.ln("    int unsigned spin;");
    f.ln("    " + n + "_valid = 1'b1;");
    f.ln("    " + n + "_addr  = a;");
    f.ln("    " + n + "_id    = id;");
    if(!qual.empty()) f.ln("    " + n + "_" + qual + " = pf;");
    f.ln("    spin = 0;");
    f.ln("    while(!" + rdy + " && spin < cg_limit) begin");
    f.ln("      @(negedge clk);");
    f.ln("      spin = spin + 1;");
    f.ln("    end");
    f.ln("    took = " + rdy + ";");
    f.ln("    // the posedge that ends this cycle is the transfer");
    f.ln("    @(negedge clk);");
    f.ln("  endtask");
    f.ln();
    f.ln("  // take the request down and stop asking");
    f.ln("  task automatic nb_idle();");
    f.ln("    " + n + "_valid = 1'b0;");
    if(!qual.empty()) f.ln("    " + n + "_" + qual + " = 1'b0;");
    f.ln("    @(negedge clk);");
    f.ln("  endtask");
    f.ln();
    f.ln("  // -------------------------------------------------------");
    f.ln("  // Ready, WITH NO REQUEST PRESENTED. The node's ready must");
    f.ln("  // not read valid, so this is a legitimate question and");
    f.ln("  // the answer is what a request would meet.");
    f.ln("  // -------------------------------------------------------");
    f.ln("  task automatic nb_ready(input logic pf, output logic r);");
    f.ln("    " + n + "_valid = 1'b0;");
    if(!qual.empty()) f.ln("    " + n + "_" + qual + " = pf;");
    f.ln("    @(negedge clk);");
    f.ln("    r = " + rdy + ";");
    if(!qual.empty()) f.ln("    " + n + "_" + qual + " = 1'b0;");
    f.ln("  endtask");
    f.ln();
    f.ln("  // -------------------------------------------------------");
    f.ln("  // Present a request for a BOUNDED number of cycles and");
    f.ln("  // say whether it was taken. A refusal is the answer this");
    f.ln("  // is asked for, so it gives up quickly and a timeout is");
    f.ln("  // not counted as a failure.");
    f.ln("  // -------------------------------------------------------");
    f.ln("  task automatic nb_try(input addr_t a,");
    f.ln("                        input req_id_t id,");
    f.ln("                        input logic pf,");
    f.ln("                        input int unsigned n,");
    f.ln("                        output logic took);");
    f.ln("    int unsigned spin;");
    f.ln("    " + n + "_valid = 1'b1;");
    f.ln("    " + n + "_addr  = a;");
    f.ln("    " + n + "_id    = id;");
    if(!qual.empty()) f.ln("    " + n + "_" + qual + " = pf;");
    f.ln("    took = 1'b0;");
    f.ln("    spin = 0;");
    f.ln("    while(!took && spin < n) begin");
    f.ln("      took = " + rdy + ";");
    f.ln("      if(!took) @(negedge clk);");
    f.ln("      spin = spin + 1;");
    f.ln("    end");
    f.ln("    if(took) @(negedge clk);   // past the transfer edge");
    f.ln("    " + n + "_valid = 1'b0;");
    if(!qual.empty()) f.ln("    " + n + "_" + qual + " = 1'b0;");
    f.ln("    @(negedge clk);");
    f.ln("  endtask");
    f.ln();
    f.ln("  // wait until an identifier's answer has arrived");
    f.ln("  task automatic nb_wait(input req_id_t id,");
    f.ln("                         output logic ok);");
    f.ln("    int unsigned spin;");
    f.ln("    spin = 0;");
    f.ln("    while(!nb_seen[id] && spin < cg_limit) begin");
    f.ln("      @(negedge clk);");
    f.ln("      spin = spin + 1;");
    f.ln("    end");
    f.ln("    ok = nb_seen[id];");
    f.ln("  endtask");
    f.ln();
  }

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
  if(nb) {
    // ONE REQUEST AT A TIME, on one identifier, so the tests that do
    // not care about ordering keep the driver every other node has.
    // The identifier is fixed rather than rotated because it is
    // retired by the response this task waits for, and a fixed one
    // cannot be in flight twice by construction.
    f.ln("  //");
    f.ln("  // This is the BLOCKING form, on identifier 0. It is what");
    f.ln("  // the tests that do not care about ordering use. The");
    f.ln("  // nb_ tasks above are the ones that hold several in");
    f.ln("  // flight.");
    f.ln("  task automatic drv_" + n + "(input addr_t a,");
    f.ln("                       input logic  wr,");
    f.ln("                       input word_t d,");
    f.ln("                       input logic [WordBytes-1:0] be,");
    f.ln("                       output word_t r);");
    f.ln("    logic ok;");
    f.ln("    int unsigned spin;");
    f.ln();
    f.ln("    @(negedge clk);");
    f.ln("    " + n + "_valid = 1'b1;");
    f.ln("    " + n + "_addr  = a;");
    f.ln("    " + n + "_id    = '0;");
    if(!qual.empty()) f.ln("    " + n + "_" + qual + " = 1'b0;");
    f.ln();
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
    f.ln("    @(negedge clk);");
    f.ln("    " + n + "_valid = 1'b0;");
    f.ln();
    f.ln("    // the answer is the one carrying this identifier, and");
    f.ln("    // nothing else about the response says which it is");
    f.ln("    r    = '0;");
    f.ln("    ok   = 1'b0;");
    f.ln("    spin = 0;");
    f.ln("    while(!ok && spin < cg_limit) begin");
    f.ln("      ok = " + n + "_rvalid && (" + n + "_rid == '0);");
    f.ln("      if(ok) r = word_t'(" + n + "_rdata);");
    f.ln("      if(!ok) @(negedge clk);");
    f.ln("      spin = spin + 1;");
    f.ln("    end");
    f.ln("    if(spin >= cg_limit) begin");
    f.ln("      cg_fail = cg_fail + 1;");
    f.ln("      $display(\"FAIL %0t drv_" + n +
         " no response\", $time);");
    f.ln("    end");
    f.ln();
    f.ln("    // PAST THE PRESENTATION. The identifier is retired by");
    f.ln("    // the response being presented and may be reissued in");
    f.ln("    // the NEXT cycle, so a caller that reissued the moment");
    f.ln("    // this returned would be presenting it twice in one.");
    f.ln("    @(negedge clk);");
    f.ln();
    f.ln("    // the write arguments are what every other node's");
    f.ln("    // driver takes; this link declares no write channel");
    f.ln("    /* verilator lint_off UNUSEDSIGNAL */");
    f.ln("    if(wr || (|d) || (|be)) begin");
    f.ln("      cg_fail = cg_fail + 1;");
    f.ln("      $display(\"FAIL %0t drv_" + n +
         " asked for a write on a read only link\",");
    f.ln("               $time);");
    f.ln("    end");
    f.ln("    /* verilator lint_on UNUSEDSIGNAL */");
    f.ln("  endtask");
    f.ln();
    return;
  }
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

  if(!ms.empty()) {
    f.ln("  // held high, the responder takes no request. See the note");
    f.ln("  // on the responder for why a test needs that.");
    f.ln("  logic tb_hold;");
    if(c.pipelined()) {
      f.ln();
      f.ln("  // the rest of what a test can ask the responder for:");
      f.ln("  // hold channel D so fills accumulate, interleave their");
      f.ln("  // beats, and serve the highest numbered source first");
      f.ln("  // so a later fill completes before an earlier one");
      f.ln("  logic tb_dhold;");
      f.ln("  logic tb_ilv;");
      f.ln("  logic tb_rev;");
    }
    f.ln();
  }
  for(const NodeCtx::Iface *ip : ms) {
    if(!ip->sig.is_tl()) continue;
    f.ln("  " + c.mod("tb_mem") + " u_" + ip->name + "_mem (");
    f.ln("    .clk     (clk),");
    f.ln("    .rstn    (rstn),");
    f.ln("    .tb_hold (tb_hold),");
    if(c.pipelined()) {
      f.ln("    .tb_dhold(tb_dhold),");
      f.ln("    .tb_ilv  (tb_ilv),");
      f.ln("    .tb_rev  (tb_rev),");
    }
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
  if(!ms.empty()) {
    f.ln("    tb_hold = 1'b0;");
    if(c.pipelined()) {
      f.ln("    tb_dhold = 1'b0;");
      f.ln("    tb_ilv   = 1'b0;");
      f.ln("    tb_rev   = 1'b0;");
    }
  }
  for(const NodeCtx::Iface *ip : sl) {
    for(const LinkSig::Sig &g : ip->sig.sigs()) {
      if(g.m_drives == ip->master) continue;   // the node owns it
      f.ln("    " + LinkSig::wire(ip->name, g.local) + " = " +
           (g.bits == 1 ? "1'b0" : "'0") + ";");
    }
  }
  if(c.nonblocking() && c.core_iface() != nullptr) {
    f.ln("    nb_clear();");
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
  const NodeCtx::Iface *m0 = c.masters().empty() ? nullptr
                                                 : c.masters()[0];
  const bool pipe  = c.pipelined() && m0 != nullptr && m0->sig.is_tl();

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

  // ------------------------------------------------------------------
  // WHAT THE DOWNSTREAM RESPONDER WOULD RETURN. A fill checked only
  // against another fill cannot tell a line reassembled in the wrong
  // order from one reassembled in the right one, so the tests need
  // the expected line itself. The two functions below are emitted
  // from the same place the responder's own copy is, so a test
  // cannot predict a fill the responder would not give.
  // ------------------------------------------------------------------
  if(pipe) {
    const int shift = Replacement::log2i(m0->sig.data_bytes());
    f.ln("  // the beat the responder returns for one line address");
    f.ln("  // and one beat index, and the whole line built from");
    f.ln("  // them. Emitted from the same place the responder's own");
    f.ln("  // copy is.");
    f.ln("  function automatic beat_t tb_seed(input addr_t base,");
    f.ln("                                    input int unsigned n);");
    f.ln("    longint unsigned k;");
    f.ln("    k = (longint'({32'd0, base}) >> " + i2s(shift) + ") +");
    f.ln("        longint'(n);");
    f.ln("    tb_seed = beat_t'(k) ^ beat_t'(64'hcafe_0000_0000_0001);");
    f.ln("  endfunction");
    f.ln();
    f.ln("  function automatic line_t tb_line(input addr_t base);");
    f.ln("    tb_line = '0;");
    f.ln("    for(int unsigned b = 0; b < Beats; b++) begin");
    f.ln("      tb_line[b*BeatBits +: BeatBits] =");
    f.ln("          tb_seed(line_base(base), b);");
    f.ln("    end");
    f.ln("  endfunction");
    f.ln();
  }

  if(s0 == nullptr) {
    f.ln("  task automatic run_tests();");
    f.ln("    $display(\"node '" + c.name() +
         "' has no slave interface to drive\");");
    f.ln("  endtask");
    return;
  }

  const bool nb = c.nonblocking() && c.core_iface() == s0;

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
  if(nb) {
    f.ln("    int unsigned k;");
    f.ln("    logic        took;");
    f.ln("    logic        all_ok;");
    f.ln("    logic        rdy_dm;");
    f.ln("    logic        rdy_pf;");
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

  // R-8. What T1 covers. The decomposition is built from the whole
  // geometry, so the round trip exercises every field of it.
  unit_covers(c, "/geometry/capacity_bytes",
              "line_addr rebuilds the line it came from");
  unit_covers(c, "/geometry/line_bytes",
              "the offset is what line_base took off");
  unit_covers(c, "/geometry/associativity",
              "line_addr rebuilds the line it came from");
  if(bank) {
    unit_covers(c, "/geometry/banks",
                "line_addr rebuilds the line it came from");
    unit_covers(c, "/geometry/bank_interleave_granularity",
                "line_addr rebuilds the line it came from");
  }

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
    unit_covers(c, "/policies/replacement",
                "touching a way moves the victim off it");
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
    unit_covers(c, "/policies/read_miss",
                "the hit is quicker than the miss");
  } else if(c.is_memory()) {
    unit_covers(c, "/timing/read_latency_cycles",
                "the second read agrees with the first");
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
    unit_covers(c, "/geometry/associativity",
                "every way of the set still hits");
    unit_covers(c, "/policies/replacement",
                "every way of the set still hits");
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
    // R-8. Only a CACHE's write path exercises these. A memory
    // takes the same test and declares none of them, and claiming
    // them there would put a test against a field the memory model
    // never reads.
    if(wr && cache) {
      unit_covers(c, "/policies/write_miss",
                  "a read sees the write before it");
      unit_covers(c, "/policies/write_hit",
                  "a read sees the write before it");
      unit_covers(c, "/storage/data_array/byte_enables",
                  "a read sees the write before it");
      if(c.has_dirty()) {
        unit_covers(c, "/storage/dirty_bits/kind",
                    "a read sees the write before it");
      }
    }
  }

  // ------------------------------------------------------------------
  // R-8. THE NON BLOCKING CORE PORT. Nothing above can see any of it:
  // every test so far issues one request and waits for it, which is
  // the behaviour a blocking port already had. What is below needs
  // several requests in flight at once, and it arranges that by
  // holding the downstream responder off so a fill cannot complete.
  // ------------------------------------------------------------------
  if(nb) {
    const std::string n  = s0->name;
    const uint64_t line  = c.geom().line_bytes;
    // Every address below is DERIVED. The stride between them is one
    // line, so consecutive ones alternate banks where the node has
    // two, and none of them collides with the addresses the tests
    // above have already warmed.
    const uint64_t b_out = 0x00040000;   // the outstanding set
    const uint64_t b_ord = 0x00050000;   // the out of order pair
    const uint64_t b_mrg = 0x00060000;   // the merged line
    const uint64_t b_tgt = 0x00070000;   // the register filled up
    const uint64_t b_alt = 0x00071000;   // an unrelated line
    const uint64_t b_pf  = 0x00080000;   // the prefetch reserve set
    const uint64_t b_pfl = 0x00090000;   // the prefetch itself
    const int      ab    = c.pa_bits();

    auto lit = [&](uint64_t v) {
      char buf[64];
      snprintf(buf, sizeof(buf), "%d'h%08llx", ab,
               static_cast<unsigned long long>(v));
      return std::string(buf);
    };

    f.ln("    // -----------------------------------------------------");
    f.ln("    // T6. EVERY OUTSTANDING REQUEST IS ACCEPTED BEFORE ANY");
    f.ln("    // OF THEM IS ANSWERED. The responder is held off, so no");
    f.ln("    // fill can complete and every request stays in flight;");
    f.ln("    // a blocking port would have stopped at the first one.");
    f.ln("    // -----------------------------------------------------");
    f.ln("    tb_hold = 1'b1;");
    f.ln("    nb_clear();");
    f.ln("    all_ok = 1'b1;");
    f.ln("    for(k = 0; k < MaxOutstanding; k++) begin");
    f.ln("      nb_req(" + lit(b_out) + " + addr_t'(k) * "
         "addr_t'(LineBytes),");
    f.ln("             req_id_t'(k), 1'b0, took);");
    f.ln("      if(!took) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    nb_idle();");
    f.ln("    cg_check(\"every outstanding request was accepted\",");
    f.ln("             all_ok);");
    f.ln("    cg_check(\"and none of them had been answered yet\",");
    f.ln("             nb_n == 0);");
    f.ln();
    f.ln("    // let them all come back, each on its own identifier");
    f.ln("    tb_hold = 1'b0;");
    f.ln("    all_ok  = 1'b1;");
    f.ln("    for(k = 0; k < MaxOutstanding; k++) begin");
    f.ln("      nb_wait(req_id_t'(k), took);");
    f.ln("      if(!took) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    cg_check(\"every identifier was answered\", all_ok);");
    f.ln("    cg_check(\"one answer per identifier and no more\",");
    f.ln("             nb_n == MaxOutstanding);");
    f.ln("    cg_check(\"and two of them carry different lines\",");
    f.ln("             nb_data[0] != nb_data[1]);");
    f.ln("    all_ok = 1'b1;");
    f.ln("    for(k = 0; k < MaxOutstanding; k++) begin");
    f.ln("      if(nb_err[k]) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    cg_check(\"and no answer carried an error\", all_ok);");
    f.ln();
    unit_covers(c, "/miss_handling/mshrs",
                "every outstanding request was accepted");

    f.ln("    // -----------------------------------------------------");
    f.ln("    // T7. AN ANSWER OUT OF ORDER. One line is warmed so a");
    f.ln("    // request for it hits, and a cold line in the other");
    f.ln("    // bank is left stuck at the held responder. The hit is");
    f.ln("    // presented SECOND and answered FIRST, and the");
    f.ln("    // identifier is the only thing that says so.");
    f.ln("    // -----------------------------------------------------");
    f.ln("    " + d0 + "(" + lit(b_ord + line) + ", 1'b0, '0, '0, "
         "r0);");
    f.ln("    tb_hold = 1'b1;");
    f.ln("    nb_clear();");
    f.ln("    nb_req(" + lit(b_ord) + ",        req_id_t'(1), 1'b0, "
         "took);");
    f.ln("    nb_req(" + lit(b_ord + line) + ", req_id_t'(2), 1'b0, "
         "took);");
    f.ln("    nb_idle();");
    f.ln("    nb_wait(req_id_t'(2), took);");
    f.ln("    cg_check(\"the request presented second was answered\",");
    f.ln("             took);");
    f.ln("    cg_check(\"while the one presented first was still "
         "outstanding\",");
    f.ln("             !nb_seen[1]);");
    f.ln("    tb_hold = 1'b0;");
    f.ln("    nb_wait(req_id_t'(1), took);");
    f.ln("    cg_check(\"the first was answered after it\", took);");
    f.ln("    cg_check(\"so the two answers came back out of order\",");
    f.ln("             nb_ord[2] < nb_ord[1]);");
    f.ln();
    if(bank) {
      unit_covers(c, "/geometry/banks",
                  "so the two answers came back out of order");
    }

    f.ln("    // -----------------------------------------------------");
    f.ln("    // T8. TWO REQUESTS FOR ONE LINE ARE ANSWERED");
    f.ln("    // SEPARATELY. They share one register, so ONE line is");
    f.ln("    // fetched, and each is answered in its own cycle");
    f.ln("    // carrying its own identifier and the same 'LineBits'");
    f.ln("    // bits. The requester sees nothing of the merge.");
    f.ln("    // -----------------------------------------------------");
    f.ln("    tb_hold = 1'b1;");
    f.ln("    nb_clear();");
    f.ln("    all_ok = 1'b1;");
    f.ln("    nb_req(" + lit(b_mrg) + ", req_id_t'(3), 1'b0, took);");
    f.ln("    if(!took) all_ok = 1'b0;");
    f.ln("    nb_req(" + lit(b_mrg) + ", req_id_t'(4), 1'b0, took);");
    f.ln("    if(!took) all_ok = 1'b0;");
    f.ln("    nb_idle();");
    f.ln("    cg_check(\"both requests for one line were accepted\",");
    f.ln("             all_ok);");
    f.ln("    tb_hold = 1'b0;");
    f.ln("    nb_wait(req_id_t'(3), took);");
    f.ln("    all_ok = took;");
    f.ln("    nb_wait(req_id_t'(4), took);");
    f.ln("    cg_check(\"and both were answered\", all_ok && took);");
    f.ln("    cg_check(\"each with its own identifier and one "
         "line\",");
    f.ln("             nb_data[3] == nb_data[4]);");
    f.ln("    cg_check(\"one line was fetched for the two of them\",");
    f.ln("             nb_fill == 1);");
    f.ln();
    unit_covers(c, "/miss_handling/mshr_targets",
                "one line was fetched for the two of them");

    f.ln("    // -----------------------------------------------------");
    f.ln("    // T9. THE TARGET AFTER THE LAST ONE IS REFUSED. Filling");
    f.ln("    // one register's targets takes ready down, and it stays");
    f.ln("    // down for a request naming an UNRELATED line, because");
    f.ln("    // ready reads the occupancy and never the address.");
    f.ln("    // -----------------------------------------------------");
    f.ln("    tb_hold = 1'b1;");
    f.ln("    nb_clear();");
    f.ln("    all_ok = 1'b1;");
    f.ln("    for(k = 0; k < MshrTargets; k++) begin");
    f.ln("      nb_req(" + lit(b_tgt) + ", req_id_t'(k), 1'b0, took);");
    f.ln("      if(!took) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    nb_idle();");
    f.ln("    cg_check(\"every target of one register was accepted\",");
    f.ln("             all_ok);");
    f.ln("    nb_ready(1'b0, rdy_dm);");
    f.ln("    cg_check(\"the target after the last one is refused\",");
    f.ln("             !rdy_dm);");
    f.ln("    nb_try(" + lit(b_alt) + ", req_id_t'(8), 1'b0, 8, "
         "took);");
    f.ln("    cg_check(\"and so is a request to an unrelated line\",");
    f.ln("             !took);");
    f.ln("    tb_hold = 1'b0;");
    f.ln("    all_ok  = 1'b1;");
    f.ln("    for(k = 0; k < MshrTargets; k++) begin");
    f.ln("      nb_wait(req_id_t'(k), took);");
    f.ln("      if(!took) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    cg_check(\"every one of them was answered separately\",");
    f.ln("             all_ok && (nb_n == MshrTargets));");
    f.ln("    nb_ready(1'b0, rdy_dm);");
    f.ln("    cg_check(\"and ready came back once it had drained\",");
    f.ln("             rdy_dm);");
    f.ln();

    if(c.prefetch_reserve() > 0) {
      const std::string q = c.reserve_qual();
      const int res = c.prefetch_reserve();
      f.ln("    // ---------------------------------------------------");
      f.ln("    // T10. THE '" + q + "' RESERVE. With fewer than " +
           std::to_string(res) + " registers");
      f.ln("    // free a request carrying the bit is refused and a");
      f.ln("    // request without it is not. That difference is the");
      f.ln("    // whole of what the bit does.");
      f.ln("    // ---------------------------------------------------");
      f.ln("    tb_hold = 1'b1;");
      f.ln("    nb_clear();");
      f.ln("    all_ok = 1'b1;");
      f.ln("    for(k = 0; k < Mshrs - 1; k++) begin");
      f.ln("      nb_req(" + lit(b_pf) + " + addr_t'(k) * "
           "addr_t'(LineBytes),");
      f.ln("             req_id_t'(k), 1'b0, took);");
      f.ln("      if(!took) all_ok = 1'b0;");
      f.ln("    end");
      f.ln("    nb_idle();");
      f.ln("    cg_check(\"the file was filled to one free "
           "register\",");
      f.ln("             all_ok);");
      f.ln("    nb_ready(1'b1, rdy_pf);");
      f.ln("    nb_ready(1'b0, rdy_dm);");
      f.ln("    cg_check(\"a " + q + " is refused inside the "
           "reserve\",");
      f.ln("             !rdy_pf);");
      f.ln("    cg_check(\"and a request without the bit is not\",");
      f.ln("             rdy_dm);");
      f.ln();
      f.ln("    tb_hold = 1'b0;");
      f.ln("    all_ok  = 1'b1;");
      f.ln("    for(k = 0; k < Mshrs - 1; k++) begin");
      f.ln("      nb_wait(req_id_t'(k), took);");
      f.ln("      if(!took) all_ok = 1'b0;");
      f.ln("    end");
      f.ln("    cg_check(\"the file drained\", all_ok);");
      f.ln("    nb_ready(1'b1, rdy_pf);");
      f.ln("    cg_check(\"a " + q + " is accepted outside the "
           "reserve\",");
      f.ln("             rdy_pf);");
      f.ln("    nb_clear();");
      f.ln("    nb_req(" + lit(b_pfl) + ", req_id_t'(0), 1'b1, "
           "took);");
      f.ln("    nb_idle();");
      f.ln("    nb_wait(req_id_t'(0), rdy_pf);");
      f.ln("    cg_check(\"and it is answered like any other "
           "request\",");
      f.ln("             took && rdy_pf);");
      f.ln();
    }

  // ------------------------------------------------------------------
  // T11 THROUGH T16, THE PIPELINE AND THE FILLS IN FLIGHT. Every one
  // of them passes on a queue in front of a blocking cache only if
  // the design behind the queue actually pipelines and actually
  // holds more than one fill; T6 through T10 above do not, which is
  // why these are here.
  //
  // The stride is TWO lines, so every address in one group lands in
  // the SAME bank on a node with two of them. A hit under a miss in
  // a DIFFERENT bank proves nothing about a pipeline: two blocking
  // banks already do that, and T7 already shows it.
  // ------------------------------------------------------------------
  if(pipe) {
    const uint64_t b_str = 0x000a0000;   // the hit stream
    const uint64_t b_lat = 0x000a4000;   // the measured latency
    const uint64_t b_hum = 0x000a8000;   // hit under miss, the warm one
    const uint64_t b_hux = 0x000ac000;   // hit under miss, the cold one
    const uint64_t b_flt = 0x000b0000;   // the fills in flight
    const uint64_t b_ilv = 0x000b8000;   // the interleaved pair
    const uint64_t b_ooo = 0x000bc000;   // the out of order pair
    const uint64_t bstep = 2 * line;     // one bank, consecutive sets
    const int      nstr  = 4;            // the length of the stream

    f.ln("    // ---------------------------------------------------");
    f.ln("    // THE SEED THE RESPONDER RETURNS, so a fill can be");
    f.ln("    // checked against what it should contain rather than");
    f.ln("    // only against another fill. Emitted from the same");
    f.ln("    // place the responder's own copy is.");
    f.ln("    // ---------------------------------------------------");
    f.ln();

    f.ln("    // ---------------------------------------------------");
    f.ln("    // T11. A HIT STREAM, ONE ANSWER PER CYCLE. " +
         i2s(nstr) + " warm");
    f.ln("    // lines in ONE bank are asked for back to back, and");
    f.ln("    // the answers come out on consecutive cycles. A bank");
    f.ln("    // that serves one access at a time cannot do that,");
    f.ln("    // however many registers stand in front of it.");
    f.ln("    // L1I-5, hit throughput one request per cycle.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    for(k = 0; k < " + i2s(nstr) + "; k++) begin");
    f.ln("      " + d0 + "(" + lit(b_str) + " + addr_t'(k) * "
         "addr_t'(" + i2s(bstep) + "),");
    f.ln("               1'b0, '0, '0, r0);");
    f.ln("    end");
    f.ln("    nb_clear();");
    f.ln("    all_ok = 1'b1;");
    f.ln("    for(k = 0; k < " + i2s(nstr) + "; k++) begin");
    f.ln("      nb_req(" + lit(b_str) + " + addr_t'(k) * "
         "addr_t'(" + i2s(bstep) + "),");
    f.ln("             req_id_t'(k), 1'b0, took);");
    f.ln("      if(!took) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    nb_idle();");
    f.ln("    cg_check(\"a hit stream is accepted one per cycle\",");
    f.ln("             all_ok);");
    f.ln("    for(k = 0; k < " + i2s(nstr) + "; k++) begin");
    f.ln("      nb_wait(req_id_t'(k), took);");
    f.ln("      if(!took) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    cg_check(\"and every one of them was answered\",");
    f.ln("             all_ok);");
    f.ln();
    f.ln("    // the answers land on consecutive cycles, which is one");
    f.ln("    // hit a cycle sustained across the whole stream");
    f.ln("    all_ok = 1'b1;");
    f.ln("    for(k = 1; k < " + i2s(nstr) + "; k++) begin");
    f.ln("      if(nb_rsp[k] != nb_rsp[k-1] + 1) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    cg_check_eq(\"MEASURED hits per " + i2s(nstr) +
         " cycles on a hit stream\",");
    f.ln("                longint'(nb_rsp[" + i2s(nstr - 1) +
         "]) - longint'(nb_rsp[0]) + 1,");
    f.ln("                longint'(" + i2s(nstr) + "));");
    f.ln("    cg_check(\"a hit stream is answered one per cycle\",");
    f.ln("             all_ok);");
    f.ln();
    f.ln("    // and none of them waited on the one in front");
    f.ln("    all_ok = 1'b1;");
    f.ln("    for(k = 0; k < " + i2s(nstr) + "; k++) begin");
    f.ln("      if((nb_rsp[k] - nb_acc[k]) != ReadLatency) begin");
    f.ln("        all_ok = 1'b0;");
    f.ln("      end");
    f.ln("    end");
    f.ln("    cg_check(\"and none of them waited on the one in "
         "front\",");
    f.ln("             all_ok);");
    f.ln();

    f.ln("    // ---------------------------------------------------");
    f.ln("    // T12. THE HIT LATENCY IS read_latency_cycles,");
    f.ln("    // MEASURED. ReadLatency is the configuration's number,");
    f.ln("    // carried into the package, so this compares the");
    f.ln("    // design against what was asked for and not against a");
    f.ln("    // literal in a test.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    // the package's own three numbers agree: the answer");
    f.ln("    // is registered out of the compare, and every stage");
    f.ln("    // between that and ReadLatency is a pad stage");
    f.ln("    cg_check(\"the pipeline depth adds up to ReadLatency\",");
    f.ln("             (CmpStage + PipePad + 1) == ReadLatency);");
    f.ln("    " + d0 + "(" + lit(b_lat) + ", 1'b0, '0, '0, r0);");
    f.ln("    nb_clear();");
    f.ln("    nb_req(" + lit(b_lat) + ", req_id_t'(5), 1'b0, took);");
    f.ln("    nb_idle();");
    f.ln("    nb_wait(req_id_t'(5), took);");
    f.ln("    cg_check(\"the warm line was answered\", took);");
    f.ln("    cg_check_eq(\"MEASURED hit latency in cycles\",");
    f.ln("                longint'(nb_rsp[5]) - longint'(nb_acc[5]),");
    f.ln("                longint'(ReadLatency));");
    f.ln();

    // R-8. What T11 and T12 cover. The two timing fields shape the
    // pipeline the hit walks down, and the latency measured at the
    // port is what they shape it into.
    unit_covers(c, "/timing/read_latency_cycles",
                "MEASURED hit latency in cycles");
    unit_covers(c, "/timing/tag_compare_stage",
                "MEASURED hit latency in cycles");

    f.ln("    // ---------------------------------------------------");
    f.ln("    // T13. A HIT UNDER A MISS, IN THE SAME BANK. The cold");
    f.ln("    // line is asked for FIRST and the responder is held,");
    f.ln("    // so its fill cannot complete. The warm line is in the");
    f.ln("    // SAME bank and is answered anyway, at the same");
    f.ln("    // latency it would have had on its own. A blocking");
    f.ln("    // bank is stuck waiting for the fill and cannot.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    " + d0 + "(" + lit(b_hum) + ", 1'b0, '0, '0, r0);");
    f.ln("    cg_check(\"the two lines are in one bank\",");
    f.ln("             bank_of(" + lit(b_hum) + ") == bank_of(" +
         lit(b_hux) + "));");
    f.ln("    tb_hold = 1'b1;");
    f.ln("    nb_clear();");
    f.ln("    nb_req(" + lit(b_hux) + ", req_id_t'(6), 1'b0, took);");
    f.ln("    nb_req(" + lit(b_hum) + ", req_id_t'(7), 1'b0, took);");
    f.ln("    nb_idle();");
    f.ln("    nb_wait(req_id_t'(7), took);");
    f.ln("    cg_check(\"the hit was answered under the miss\", took);");
    f.ln("    cg_check(\"while the miss was still outstanding\",");
    f.ln("             !nb_seen[6]);");
    f.ln("    cg_check_eq(\"and at the hit latency, not the miss's\",");
    f.ln("                longint'(nb_rsp[7]) - longint'(nb_acc[7]),");
    f.ln("                longint'(ReadLatency));");
    f.ln("    tb_hold = 1'b0;");
    f.ln("    nb_wait(req_id_t'(6), took);");
    f.ln("    cg_check(\"and the miss completed after it\",");
    f.ln("             took && (nb_ord[7] < nb_ord[6]));");
    f.ln();

    f.ln("    // ---------------------------------------------------");
    f.ln("    // T14. Mshrs FILLS IN FLIGHT AT ONCE, counted at the");
    f.ln("    // memory side. Channel D is held, so no fill can");
    f.ln("    // complete and every one that left is still out. A");
    f.ln("    // master that carries one fill at a time peaks at one");
    f.ln("    // however many registers are behind it. L1I-23.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    tb_dhold = 1'b1;");
    f.ln("    nb_clear();");
    f.ln("    all_ok = 1'b1;");
    f.ln("    for(k = 0; k < Mshrs; k++) begin");
    f.ln("      nb_req(" + lit(b_flt) + " + addr_t'(k) * "
         "addr_t'(LineBytes),");
    f.ln("             req_id_t'(k), 1'b0, took);");
    f.ln("      if(!took) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    nb_idle();");
    f.ln("    cg_check(\"every one of them was accepted\", all_ok);");
    f.ln("    // one fill leaves a cycle, so give them room to");
    f.ln("    cg_tick(4 * Mshrs);");
    f.ln("    cg_check_eq(\"MEASURED fills in flight at once\",");
    f.ln("                longint'(nb_pk), longint'(Mshrs));");
    f.ln("    cg_check(\"and not one of them had completed\",");
    f.ln("             nb_n == 0);");
    f.ln("    tb_dhold = 1'b0;");
    f.ln("    all_ok   = 1'b1;");
    f.ln("    for(k = 0; k < Mshrs; k++) begin");
    f.ln("      nb_wait(req_id_t'(k), took);");
    f.ln("      if(!took) all_ok = 1'b0;");
    f.ln("    end");
    f.ln("    cg_check(\"and all of them then came back\", all_ok);");
    f.ln();

    f.ln("    // ---------------------------------------------------");
    f.ln("    // T15. TWO FILLS WHOSE BEATS INTERLEAVE. The responder");
    f.ln("    // hands out beats round robin, so a beat of one line");
    f.ln("    // arrives while the other line is part way through.");
    f.ln("    // Both have to be reassembled per source, and both");
    f.ln("    // answers are checked against the line the responder");
    f.ln("    // would return rather than only against each other.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    tb_dhold = 1'b1;");
    f.ln("    tb_ilv   = 1'b1;");
    f.ln("    nb_clear();");
    f.ln("    nb_req(" + lit(b_ilv) + ", req_id_t'(1), 1'b0, took);");
    f.ln("    nb_req(" + lit(b_ilv + bstep) + ", req_id_t'(2), 1'b0, "
         "took);");
    f.ln("    nb_idle();");
    f.ln("    cg_tick(16);");
    f.ln("    cg_check_eq(\"two fills are outstanding together\",");
    f.ln("                longint'(nb_pk), 2);");
    f.ln("    tb_dhold = 1'b0;");
    f.ln("    nb_wait(req_id_t'(1), took);");
    f.ln("    all_ok = took;");
    f.ln("    nb_wait(req_id_t'(2), took);");
    f.ln("    cg_check(\"both interleaved fills were answered\",");
    f.ln("             all_ok && took);");
    f.ln("    cg_check(\"the beats of the two did interleave\",");
    f.ln("             nb_ilv > 0);");
    f.ln("    cg_check(\"the first line was reassembled correctly\",");
    f.ln("             nb_data[1] == word_t'(tb_line(" + lit(b_ilv) +
         ")));");
    f.ln("    cg_check(\"and so was the second\",");
    f.ln("             nb_data[2] == word_t'(tb_line(" +
         lit(b_ilv + bstep) + ")));");
    f.ln("    tb_ilv = 1'b0;");
    f.ln();

    f.ln("    // ---------------------------------------------------");
    f.ln("    // T16. A LATER FILL COMPLETES FIRST. The responder is");
    f.ln("    // told to serve the highest numbered source first, so");
    f.ln("    // the fill issued SECOND comes back FIRST. The right");
    f.ln("    // register has to retire: each identifier gets the");
    f.ln("    // line ITS request asked for.");
    f.ln("    // ---------------------------------------------------");
    f.ln("    tb_dhold = 1'b1;");
    f.ln("    tb_rev   = 1'b1;");
    f.ln("    nb_clear();");
    f.ln("    nb_req(" + lit(b_ooo) + ", req_id_t'(1), 1'b0, took);");
    f.ln("    nb_req(" + lit(b_ooo + bstep) + ", req_id_t'(2), 1'b0, "
         "took);");
    f.ln("    nb_idle();");
    f.ln("    cg_tick(16);");
    f.ln("    cg_check_eq(\"both fills are outstanding together\",");
    f.ln("                longint'(nb_pk), 2);");
    f.ln("    tb_dhold = 1'b0;");
    f.ln("    nb_wait(req_id_t'(2), took);");
    f.ln("    cg_check(\"the fill issued second came back\", took);");
    f.ln("    cg_check(\"before the one issued first\", !nb_seen[1]);");
    f.ln("    nb_wait(req_id_t'(1), took);");
    f.ln("    cg_check(\"and the first came back after it\",");
    f.ln("             took && (nb_ord[2] < nb_ord[1]));");
    f.ln("    cg_check(\"the first line went to the register that "
         "asked for it\",");
    f.ln("             nb_data[1] == word_t'(tb_line(" + lit(b_ooo) +
         ")));");
    f.ln("    cg_check(\"and the same for the second\",");
    f.ln("             nb_data[2] == word_t'(tb_line(" +
         lit(b_ooo + bstep) + ")));");
    f.ln("    tb_rev = 1'b0;");
    f.ln();
  }
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

  // ------------------------------------------------------------------
  // R-9. The memory model's own image and peek functions, reached
  // hierarchically. The model owns the store, so it owns the dump:
  // the testbench asking for the image cannot write a format the
  // model disagrees with.
  // ------------------------------------------------------------------
  const Model::Node *memn = nullptr;
  for(const Model::Node &n : m.nodes) {
    const NodeCtx *c = ctx_of(nodes, n.name);
    if(c != nullptr && c->is_memory()) { memn = &n; break; }
  }

  if(memn != nullptr) {
    const NodeCtx *mc = ctx_of(nodes, memn->name);
    const NodeCtx::Iface &mi = mc->ifaces().front();
    const std::string path = "u_dut.u_" + memn->name + ".u_" +
                             mi.name + "_slv";
    const int beat = mi.sig.data_bits();
    // ------------------------------------------------------------
    // THE AGENTS NEED NOT BE THE SAME WIDTH. A word out of the
    // memory is compared against what an agent read back, so the
    // width that belongs here is the NARROWEST agent's: the memory
    // holds a word of that size at that address, and a wider agent
    // returns it in the low bits of a whole line.
    // ------------------------------------------------------------
    int wbits = 0;
    int abits = 0;
    for(const NodeCtx *a : ag) {
      const int w = a->ifaces()[0].sig.data_bits();
      if(wbits == 0 || w < wbits) wbits = w;
      abits = std::max(abits, a->ifaces()[0].sig.addr_bits());
    }
    if(wbits == 0) wbits = 32;
    if(abits == 0) abits = 32;
    const int wsh   = Replacement::log2i(wbits / 8);
    const int wsel  = Replacement::log2i(beat / wbits);

    f.ln("  // ----------------------------------------------------");
    f.ln("  // R-9. The memory image and the two ways of asking the");
    f.ln("  // store a question. All three live in the memory model,");
    f.ln("  // which owns the store, and are reached from here.");
    f.ln("  //");
    f.ln("  // The images go under IMAGES in the top level Makefile.");
    f.ln("  // ----------------------------------------------------");
    f.ln("  task automatic mem_image(input string point);");
    f.ln("    " + path + ".cg_dump_image(");
    f.ln("        {\"images/" + sys + "_\", point, \".mem\"}, point);");
    f.ln("  endtask");
    f.ln();
    f.ln("  function automatic logic mem_has(input logic [" +
         i2s(abits - 1) + ":0] a);");
    f.ln("    mem_has = " + path + ".cg_has(a);");
    f.ln("  endfunction");
    f.ln();
    f.ln("  // one word out of the beat the memory holds");
    f.ln("  function automatic logic [" + i2s(wbits - 1) +
         ":0] mem_word");
    f.ln("      (input logic [" + i2s(abits - 1) + ":0] a);");
    f.ln("    logic [" + i2s(beat - 1) + ":0] b;");
    f.ln("    b = " + path + ".cg_peek(a);");
    if(wsel > 0) {
      f.ln("    mem_word = b[" + i2s(wbits) + " * int'(a[" +
           i2s(wsh + wsel - 1) + ":" + i2s(wsh) + "]) +: " +
           i2s(wbits) + "];");
    } else {
      f.ln("    mem_word = b[" + i2s(wbits - 1) + ":0];");
    }
    f.ln("  endfunction");
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
// R-8. THE TOP LEVEL TESTS.
//
// Seven checks proved the nodes connect. What belongs here is
// behaviour visible ONLY WHEN NODES INTERACT, and the list of it is
// derived from the configuration rather than from a general idea of
// what a cache does:
//
//   the agent's link width against the cache's line, which is the
//     multi-beat fill CLI-004 found a deadlock in
//   write_hit write_back, which is invisible below the memory: the
//     written line has to NOT be there, and then to be there after
//     an eviction
//   the associativity and the replacement policy, at system scale,
//     which is what forces that eviction
//   the memory's read_latency_cycles, which only a miss that reaches
//     memory can measure
//   range_check, which only the terminal node can refuse
//   two upstream interfaces against one L2, which is the contention
//     no interface can see on its own
//
// Every check registers the field it exercises, so the R-8 table is
// generated from the tests that were actually emitted.
// --------------------------------------------------------------------
void RtlTb::sys_tests(SvFile &f, const Model &m,
                      const std::map<std::string, NodeCtx> &nodes,
                      const std::string &sys)
{
  const std::string bench = sys + "_tb";

  // ------------------------------------------------------------------
  // The chain, walked from each agent rather than assumed. An agent
  // feeds one cache, that cache feeds the next, and the last node is
  // the memory.
  // ------------------------------------------------------------------
  struct Path {
    const NodeCtx *agent{nullptr};
    std::string    l1;          // the cache the agent feeds
    const NodeCtx *l1c{nullptr};
  };

  std::vector<Path> paths;
  for(const Model::Node &n : m.nodes) {
    const NodeCtx *c = ctx_of(nodes, n.name);
    if(c == nullptr || !c->is_agent()) continue;
    Path p;
    p.agent = c;
    p.l1    = downstream(m, n.name);
    p.l1c   = ctx_of(nodes, p.l1);
    paths.push_back(p);
  }

  // the cache the L1s share, and the memory behind it
  std::string shared_name;
  const NodeCtx *shared = nullptr;
  const NodeCtx *memc   = nullptr;
  std::string memn;

  if(!paths.empty() && paths[0].l1c != nullptr) {
    shared_name = downstream(m, paths[0].l1);
    shared      = ctx_of(nodes, shared_name);
  }
  for(const Model::Node &n : m.nodes) {
    const NodeCtx *c = ctx_of(nodes, n.name);
    if(c != nullptr && c->is_memory()) { memc = c; memn = n.name; break; }
  }

  // the agent whose cache can take a write
  const Path *wp = nullptr;
  for(const Path &p : paths) {
    if(p.l1c != nullptr && p.l1c->has_writes() && p.l1c->has_dirty()) {
      wp = &p;
      break;
    }
  }

  // ------------------------------------------------------------------
  // Every address below is DERIVED. The stride that lands two lines
  // in one set is the index span of the deepest cache on the path,
  // and the number of lines that forces a line out of every level is
  // twice the widest associativity plus one.
  // ------------------------------------------------------------------
  // ------------------------------------------------------------------
  // THE AGENTS NEED NOT BE THE SAME WIDTH. wbits is the NARROWEST of
  // them, because it is the width every cross agent comparison and
  // every comparison against the memory has to be made at: a wider
  // agent reads a whole line and the narrow one reads a word out of
  // the front of it. Each agent's own r0 and r1 are declared at its
  // OWN width, so nothing is truncated on the way in.
  // ------------------------------------------------------------------
  int abits = 0;
  int wbits = 0;
  for(const Path &p : paths) {
    const int w = p.agent->ifaces()[0].sig.data_bits();
    if(wbits == 0 || w < wbits) wbits = w;
    abits = std::max(abits, p.agent->ifaces()[0].sig.addr_bits());
  }
  if(abits == 0) abits = 32;
  if(wbits == 0) wbits = 32;

  uint64_t stride = 0;
  int      ways   = 1;
  uint64_t line   = 64;
  for(const Path &p : paths) {
    if(p.l1c == nullptr || !p.l1c->geom().valid) continue;
    const Model::Geom &g = p.l1c->geom();
    stride = std::max(stride, uint64_t(1) << (g.index.msb + 1));
    ways   = std::max(ways, g.associativity);
    line   = g.line_bytes;
  }
  if(shared != nullptr && shared->geom().valid) {
    const Model::Geom &g = shared->geom();
    stride = std::max(stride, uint64_t(1) << (g.index.msb + 1));
    ways   = std::max(ways, g.associativity);
  }
  if(stride == 0) stride = 4096;

  const int      evictions = 2 * ways + 1;
  const uint64_t base      = stride;   // one stride in, never zero
  const uint64_t hot       = base + 4 * stride * uint64_t(evictions);

  f.note("Self checking tests for system '" + sys + "', THE TOP");
  f.note("LEVEL. See the note above RtlTb::sys_tests for what belongs");
  f.note("here and what belongs in a unit testbench.");
  f.note("");
  f.note("`include this INSIDE the system testbench module.");
  f.note("");
  f.note("Every address is derived. The stride that lands two lines");
  f.note("in one set is " + std::to_string(stride) + ", the index span of the deepest");
  f.note("cache on the path, and " + std::to_string(evictions) +
         " lines at that stride force a");
  f.note("line out of every level of a " + std::to_string(ways) +
         " way cache.");
  f.bar();
  f.ln();
  f.ln("  task automatic run_tests();");
  for(const Path &p : paths) {
    const std::string a = p.agent->name();
    const int w = p.agent->ifaces()[0].sig.data_bits();
    f.ln("    logic [" + i2s(w - 1) + ":0] " + a + "_r0;");
    f.ln("    logic [" + i2s(w - 1) + ":0] " + a + "_r1;");
  }
  f.ln("    logic [" + i2s(wbits - 1) + ":0] wv;");
  f.ln("    logic [" + i2s(abits - 1) + ":0] ea;");
  f.ln("    int unsigned t0;");
  f.ln("    int unsigned t1;");
  f.ln("    int unsigned k;");
  f.ln();

  const bool images = memc != nullptr;

  if(images) {
    f.ln("    // -----------------------------------------------------");
    f.ln("    // R-9. The image before anything runs. Every later");
    f.ln("    // image is a difference against this one.");
    f.ln("    // -----------------------------------------------------");
    f.ln("    mem_image(\"after_reset\");");
    f.ln("    cg_check(\"the store is empty before any request\",");
    f.ln("             !mem_has(" + addr_lit(abits, base) + "));");
    f.ln("    cg_check(\"and it is empty everywhere, not just there\",");
    f.ln("             !mem_has(" + addr_lit(abits, hot) + "));");
    f.ln();
  }

  // ------------------------------------------------------------------
  // one cold read and one hit through every agent
  //
  // EVERY AGENT GETS ITS OWN ADDRESS. They share the cache below
  // their L1s, so one agent's cold miss warms the line for the next
  // one, and the second agent's "cold" miss would then never reach
  // memory. The read latency check below would measure the shared
  // cache instead of the memory and quietly pass on some
  // configurations and fail on others.
  // ------------------------------------------------------------------
  for(size_t pi = 0; pi < paths.size(); ++pi) {
    const Path       &p  = paths[pi];
    const std::string a  = p.agent->name();
    const uint64_t    ab = base + uint64_t(pi) * stride;

    f.ln("    // -----------------------------------------------------");
    f.ln("    // Agent '" + a + "': a cold read reaches memory and the");
    f.ln("    // same address then hits in '" + p.l1 + "'. The address");
    f.ln("    // is this agent's own, see the note above.");
    f.ln("    // -----------------------------------------------------");
    f.ln("    t0 = cg_cycle;");
    f.ln("    go_" + a + "(" + addr_lit(abits, ab) + ", 1'b0, '0, " +
         a + "_r0);");
    f.ln("    t1 = cg_cycle;");
    f.ln("    go_" + a + "(" + addr_lit(abits, ab) + ", 1'b0, '0, " +
         a + "_r1);");
    f.ln("    cg_check(\"" + a +
         " hit returns what the miss filled\",");
    f.ln("             " + a + "_r1 == " + a + "_r0);");
    f.ln("    cg_check(\"" + a + " hit is quicker than the miss\",");
    f.ln("             (cg_cycle - t1) < (t1 - t0));");
    f.ln("    cg_check(\"" + a + " went idle after its last request\",");
    f.ln("             !" + a + "_busy);");

    if(p.l1c != nullptr) {
      top_covers(p.l1, "/policies/read_miss", bench,
                 a + " hit is quicker than the miss");
      top_covers(p.l1, "/geometry/capacity_bytes", bench,
                 a + " hit returns what the miss filled");
    }

    // the miss crossed the whole path, so it cannot be quicker than
    // the memory's own declared read latency
    if(memc != nullptr && memc->read_latency() > 0) {
      f.ln("    cg_check(\"" + a +
           " miss covers the memory read latency\",");
      f.ln("             (t1 - t0) >= " +
           std::to_string(memc->read_latency()) + ");");
      top_covers(memn, "/timing/read_latency_cycles", bench,
                 a + " miss covers the memory read latency");
    }
    f.ln();
  }

  // ------------------------------------------------------------------
  // the write path, the beat structure, the write back and the
  // eviction, all through the one agent whose cache takes writes
  // ------------------------------------------------------------------
  if(wp != nullptr) {
    const std::string a = wp->agent->name();
    const uint64_t    half = line / 2;

    f.ln("    // -----------------------------------------------------");
    f.ln("    // A write miss allocates in '" + wp->l1 +
         "' and the read");
    f.ln("    // after it sees the value. Nothing below the cache has");
    f.ln("    // seen it yet, which is what write_back means.");
    f.ln("    // -----------------------------------------------------");
    f.ln("    wv = " + i2s(wbits) + "'hc0de_0001;");
    f.ln("    go_" + a + "(" + addr_lit(abits, hot) + ", 1'b1, wv, " +
         a + "_r0);");
    f.ln("    go_" + a + "(" + addr_lit(abits, hot) + ", 1'b0, '0, " +
         a + "_r1);");
    f.ln("    cg_check(\"a write miss allocates and the read after "
         "it sees it\",");
    f.ln("             " + a + "_r1 == wv);");
    top_covers(wp->l1, "/policies/write_miss", bench,
               "a write miss allocates and the read after it sees it");
    top_covers(wp->l1, "/policies/write_hit", bench,
               "a write miss allocates and the read after it sees it");
    f.ln();

    if(half >= 4) {
      f.ln("    // ---------------------------------------------------");
      f.ln("    // The line is " + std::to_string(line) +
           " bytes and the downstream beat is");
      f.ln("    // narrower than that, so a fill is several beats. A");
      f.ln("    // write into the second beat must leave the first");
      f.ln("    // beat's word alone. CLI-004's deadlock lived here.");
      f.ln("    // ---------------------------------------------------");
      f.ln("    ea = " + addr_lit(abits, hot + half) + ";");
      f.ln("    go_" + a + "(ea, 1'b1, " + i2s(wbits) +
           "'hc0de_0002, " + a + "_r0);");
      f.ln("    go_" + a + "(" + addr_lit(abits, hot) +
           ", 1'b0, '0, " + a + "_r1);");
      f.ln("    cg_check(\"a write in one beat leaves the other beat "
           "alone\",");
      f.ln("             " + a + "_r1 == wv);");
      f.ln("    go_" + a + "(ea, 1'b0, '0, " + a + "_r1);");
      f.ln("    cg_check(\"the word in the second beat reads back\",");
      f.ln("             " + a + "_r1 == " + i2s(wbits) +
           "'hc0de_0002);");
      top_covers(wp->l1, "/geometry/line_bytes", bench,
                 "a write in one beat leaves the other beat alone");
      top_covers(wp->l1, "/storage/data_array/byte_enables", bench,
                 "a write in one beat leaves the other beat alone");
      if(shared != nullptr) {
        top_covers(shared_name, "/geometry/line_bytes", bench,
                   "the word in the second beat reads back");
      }
      f.ln();
    }

    if(images) {
      f.ln("    // ---------------------------------------------------");
      f.ln("    // write_back, the half no unit test can see. The");
      f.ln("    // written line is dirty upstream and the memory has");
      f.ln("    // never been told about it.");
      f.ln("    // ---------------------------------------------------");
      f.ln("    cg_check(\"the written line has not reached memory\",");
      f.ln("             !mem_has(" + addr_lit(abits, hot) + "));");
      f.ln("    mem_image(\"after_write\");");
      top_covers(wp->l1, "/policies/write_hit", bench,
                 "the written line has not reached memory");
      f.ln();

      f.ln("    // ---------------------------------------------------");
      f.ln("    // and the other half. " + std::to_string(evictions) +
           " lines at the set stride");
      f.ln("    // push it out of every level, and the write back");
      f.ln("    // carries it to memory with its data intact.");
      f.ln("    // ---------------------------------------------------");
      f.ln("    for(k = 0; k < " + std::to_string(evictions) +
           "; k++) begin");
      // The stride arithmetic is done at the address width. k is an
      // int, so casting the whole expression afterwards would compute
      // it in 32 bits and widen the answer, which stops being the
      // same answer once pa_bits passes 32.
      f.ln("      ea = " + addr_lit(abits, hot) + " +");
      f.ln("           (" + i2s(abits) + "'(k) + " + i2s(abits) +
           "'d1) * " + i2s(abits) + "'d" +
           std::to_string(stride) + ";");
      f.ln("      go_" + a + "(ea, 1'b1, " + i2s(wbits) +
           "'hbeef_0000 + " + i2s(wbits) + "'(k), " + a + "_r0);");
      f.ln("    end");
      f.ln("    cg_check(\"the evicted dirty line reached memory\",");
      f.ln("             mem_has(" + addr_lit(abits, hot) + "));");
      f.ln("    cg_check(\"and it carries what was written to it\",");
      f.ln("             mem_word(" + addr_lit(abits, hot) +
           ") == wv);");
      f.ln("    mem_image(\"after_evict\");");
      top_covers(wp->l1, "/geometry/associativity", bench,
                 "the evicted dirty line reached memory");
      top_covers(wp->l1, "/policies/replacement", bench,
                 "the evicted dirty line reached memory");
      top_covers(wp->l1, "/storage/dirty_bits/kind", bench,
                 "and it carries what was written to it");
      if(shared != nullptr) {
        top_covers(shared_name, "/geometry/associativity", bench,
                   "the evicted dirty line reached memory");
        top_covers(shared_name, "/policies/replacement", bench,
                   "the evicted dirty line reached memory");
        top_covers(shared_name, "/policies/write_hit", bench,
                   "and it carries what was written to it");
        top_covers(shared_name, "/storage/dirty_bits/kind", bench,
                   "and it carries what was written to it");
      }
      f.ln();
    }

    // ----------------------------------------------------------------
    // the bank select, R-7. Two lines that differ only in the bank
    // bit land in different banks of the shared cache and both are
    // served.
    // ----------------------------------------------------------------
    if(shared != nullptr && shared->geom().bank_resolved &&
       shared->geom().banks > 1) {
      const uint64_t bit = uint64_t(1) << shared->geom().bank.lsb;
      f.ln("    // ---------------------------------------------------");
      f.ln("    // R-7. '" + shared_name + "' interleaves by line, so "
           "the bank");
      f.ln("    // select is bit " +
           std::to_string(shared->geom().bank.lsb) +
           ", immediately above the offset.");
      f.ln("    // These two addresses differ in that bit and nothing");
      f.ln("    // else, so they sit in DIFFERENT BANKS of the SAME");
      f.ln("    // set, and both have to be served.");
      f.ln("    // ---------------------------------------------------");
      f.ln("    ea = " + addr_lit(abits, base + 8 * stride) + ";");
      f.ln("    go_" + a + "(ea, 1'b1, " + i2s(wbits) +
           "'ha11c_0000, " + a + "_r0);");
      f.ln("    go_" + a + "(ea + " + i2s(abits) + "'d" +
           std::to_string(bit) + ", 1'b1, " + i2s(wbits) +
           "'ha11c_0001, " + a + "_r0);");
      f.ln("    go_" + a + "(ea, 1'b0, '0, " + a + "_r0);");
      f.ln("    go_" + a + "(ea + " + i2s(abits) + "'d" +
           std::to_string(bit) + ", 1'b0, '0, " + a + "_r1);");
      f.ln("    cg_check(\"the line below the bank select reads "
           "back\",");
      f.ln("             " + a + "_r0 == " + i2s(wbits) +
           "'ha11c_0000);");
      f.ln("    cg_check(\"the line in the other bank reads back\",");
      f.ln("             " + a + "_r1 == " + i2s(wbits) +
           "'ha11c_0001);");
      f.ln("    cg_check(\"and the two banks did not answer with one "
           "line\",");
      f.ln("             " + a + "_r0 != " + a + "_r1);");
      top_covers(shared_name, "/geometry/banks", bench,
                 "the line in the other bank reads back");
      top_covers(shared_name, "/geometry/bank_interleave_granularity",
                 bench, "and the two banks did not answer with one line");
      f.ln();
    }
  }

  // ------------------------------------------------------------------
  // range_check, which only the terminal node can refuse
  // ------------------------------------------------------------------
  if(memc != nullptr && memc->range_check() && !paths.empty()) {
    const uint64_t cap = memc->geom().capacity_bytes;
    if(cap > 0 && abits < 64 &&
       cap < (uint64_t(1) << (abits < 63 ? abits : 63))) {
      const std::string a = wp != nullptr ? wp->agent->name()
                                          : paths[0].agent->name();
      f.ln("    // ---------------------------------------------------");
      f.ln("    // range_check on '" + memn + "'. Its capacity is " +
           std::to_string(cap));
      f.ln("    // bytes and the address space is " + i2s(abits) +
           " bits, so an address");
      f.ln("    // at the capacity is outside it and is DENIED rather");
      f.ln("    // than quietly wrapped. The request has to come back,");
      f.ln("    // denied or not: a refusal that hangs is a deadlock.");
      f.ln("    // ---------------------------------------------------");
      f.ln("    go_" + a + "(" + addr_lit(abits, cap) + ", 1'b0, '0, " +
           a + "_r0);");
      f.ln("    cg_check(\"a read past the memory capacity still "
           "completes\",");
      f.ln("             !" + a + "_busy);");
      top_covers(memn, "/range_check", bench,
                 "a read past the memory capacity still completes");
      top_covers(memn, "/geometry/capacity_bytes", bench,
                 "a read past the memory capacity still completes");
      f.ln();
    }
  }

  // ------------------------------------------------------------------
  // two upstream interfaces against one shared cache
  // ------------------------------------------------------------------
  if(paths.size() >= 2 && shared != nullptr) {
    f.ln("    // -----------------------------------------------------");
    f.ln("    // Both agents against one address. They run through");
    f.ln("    // different L1s into '" + shared_name + "', which is "
         "where the");
    f.ln("    // arbitration between its upstream interfaces is the");
    f.ln("    // only thing that can go wrong. No interface can see");
    f.ln("    // this on its own.");
    f.ln("    // -----------------------------------------------------");
    const uint64_t two = base + uint64_t(paths.size() + 1) * stride;
    for(const Path &p : paths) {
      f.ln("    go_" + p.agent->name() + "(" + addr_lit(abits, two) +
           ", 1'b0, '0, " + p.agent->name() + "_r0);");
    }
    f.ln("    cg_check(\"both agents completed against one line\",");
    f.ln("             " + [&]{
           std::string e;
           for(size_t k = 0; k < paths.size(); ++k) {
             if(k) e += " && ";
             e += "!" + paths[k].agent->name() + "_busy";
           }
           return e;
         }() + ");");
    // The two agents may read different widths of the same line, so
    // the comparison is made at the NARROWER of them. Both addresses
    // are line aligned, so the narrow agent's word is the front of
    // the wide one's line and there is nothing else to compare.
    f.ln("    cg_check(\"and they agree about what is at it\",");
    f.ln("             " + i2s(wbits) + "'(" +
         paths[0].agent->name() + "_r0) == " + i2s(wbits) + "'(" +
         paths[1].agent->name() + "_r0));");
    for(const Path &p : paths) {
      top_covers(p.l1, "/indexing", bench,
                 "and they agree about what is at it");
    }
    top_covers(shared_name, "/indexing", bench,
               "both agents completed against one line");
    f.ln();
  }

  if(images) {
    f.ln("    // -----------------------------------------------------");
    f.ln("    // R-9. The final image.");
    f.ln("    // -----------------------------------------------------");
    f.ln("    mem_image(\"final\");");
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
  if(!i.sig.read_only()) {
    f.ln("        if(" + LinkSig::wire(n, "rw") + ") begin");
    f.ln("          store[key_of(" + LinkSig::wire(n, "addr") +
         ")] =");
    f.ln("              " + LinkSig::wire(n, "wdata") + ";");
    f.ln("        end else begin");
  } else {
    f.ln("        // the link declares no write channel, so every");
    f.ln("        // request is a read and the store is read only");
    f.ln("        begin");
  }
  f.ln("          " + LinkSig::wire(n, "rvalid") + " <= 1'b1;");
  if(i.sig.id_bits() > 0) {
    f.ln("          " + LinkSig::wire(n, "rid") + " <= " +
         LinkSig::wire(n, "id") + ";");
  }
  f.ln("          " + LinkSig::wire(n, "rdata") + " <=");
  f.ln("              (store.exists(key_of(" +
       LinkSig::wire(n, "addr") + ")) != 0)");
  // A read of an address never written answers with the address
  // itself, which makes an unbacked read visible in a waveform. The
  // address and the read data are independently sized, so the value
  // is cast to the data width rather than assumed equal to it.
  f.ln("              ? store[key_of(" + LinkSig::wire(n, "addr") +
       ")]");
  f.ln("              : WordBits'(" + LinkSig::wire(n, "addr") +
       ");");
  f.ln("        end");
  f.ln("      end");
  f.ln("    end");
  f.ln("  end");
  f.ln("  /* verilator lint_on BLKSEQ */");
  f.ln();
  if(i.sig.err_ret()) {
    f.ln("  // this responder has no error to return");
    f.ln("  assign " + LinkSig::wire(n, "rerr") + " = 1'b0;");
    f.ln();
  }
  {
    std::vector<std::string> unread;
    for(const LinkSig::Sig &g : i.sig.sigs()) {
      if(!g.m_drives) continue;      // the responder drives it
      if(g.local == "valid" || g.local == "addr" ||
         g.local == "rw" || g.local == "wdata") continue;
      if(g.local == "id" && i.sig.id_bits() > 0) continue;
      unread.push_back(LinkSig::wire(n, g.local));
    }
    if(!unread.empty()) {
      f.ln("  /* verilator lint_off UNUSEDSIGNAL */");
      f.ln("  wire unused = |{");
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
  const NodeCtx::Iface *i0 = c.ifaces().empty() ? nullptr
                                                : &c.ifaces()[0];
  const bool rdonly = i0 != nullptr && i0->sig.read_only();

  if(rdonly) {
    // The link declares no write channel, so there is no write to
    // see. What is left to check is that the agent runs a SECOND
    // request at all: it is one outstanding, so the second one
    // cannot start until the first has been answered, and a driver
    // that never released would hang here rather than pass.
    f.ln("    // this link declares no write channel, so the second");
    f.ln("    // request is another read. A DIFFERENT ADDRESS, so a");
    f.ln("    // driver that answered from the first request's data");
    f.ln("    // would be caught rather than agreed with.");
    f.ln("    a = AddrBits'(32'h0000_2000);");
    f.ln("    go(a, 1'b0, '0, r1);");
    f.ln("    cg_check(\"a second read returns its own seed\",");
    f.ln("             r1 == WordBits'(a));");
    f.ln("    cg_check(\"and it is not the first read's answer\",");
    f.ln("             r1 != r0);");
  } else {
    f.ln("    // a write is visible to the read that follows it");
    f.ln("    go(a, 1'b1, WordBits'(32'hdead_beef), r1);");
    f.ln("    go(a, 1'b0, '0, r1);");
    f.ln("    cg_check(\"a read sees the write before it\",");
    f.ln("             r1 == WordBits'(32'hdead_beef));");
  }
  f.ln();
  f.ln("    cg_check(\"the agent went idle after the last request\",");
  f.ln("             !cmd_busy);");
  f.ln("  endtask");
}

} // namespace cgen
