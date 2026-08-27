// --------------------------------------------------------------------
// FILE:    rtl_pkg.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "rtl_pkg.h"

namespace cgen
{

namespace {

std::string u(uint64_t v) { return std::to_string(v); }
std::string i(int v)      { return std::to_string(v); }

// One localparam, padded so a block of them lines up. A name and a
// value that will not fit in 80 columns together are split across
// two lines rather than run over.
std::string lp(const std::string &name, const std::string &val,
               const char *type = "int unsigned")
{
  std::string s = "  localparam " + std::string(type) + " " + name;
  while(s.size() < 40) s += ' ';

  const std::string one = s + "= " + val + ";";
  if(one.size() <= 80) return one;

  return "  localparam " + std::string(type) + " " + name + " =\n" +
         "      " + val + ";";
}

} // namespace

// --------------------------------------------------------------------
// A file scope wildcard import is what the project style asks for:
// before the module declaration, never inside the module header.
// Verilator's IMPORTSTAR objects to a wildcard reaching $unit scope.
//
// The style rule wins on emitted text, so the import stays where it
// is and the warning is turned off across those lines and nowhere
// else. This is one of the enumerated R-8 waivers.
// --------------------------------------------------------------------
void RtlPkg::import_of(SvFile &f, const std::vector<std::string> &pkgs)
{
  if(pkgs.empty()) return;
  f.ln("// The import is at file scope and before the module, which "
       "is what");
  f.ln("// the project verilog style asks for. Verilator's IMPORTSTAR "
       "objects");
  f.ln("// to a wildcard in $unit scope, so it is turned off across "
       "these");
  f.ln("// lines and nowhere else.");
  f.ln("/* verilator lint_off IMPORTSTAR */");
  for(const std::string &p : pkgs) f.ln("import " + p + "::*;");
  f.ln("/* verilator lint_on IMPORTSTAR */");
  f.ln();
}

// --------------------------------------------------------------------
// TileLink 1.9.3, Table 15 through Table 19. The opcode space is
// stated in full rather than only the codes this design drives, so
// that a testbench decoding a channel has every name available.
// Nothing consumes most of them, hence the UNUSEDPARAM block.
// --------------------------------------------------------------------
void RtlPkg::tilelink(SvFile &f)
{
  f.note("The TileLink 1.9.3 constant set, shared by every link in "
         "the");
  f.note("system. The opcode space is emitted whole so that a "
         "consumer");
  f.note("decoding a channel has every name, not only the ones this");
  f.note("design happens to drive.");
  f.bar();
  f.ln("package " + tl_pkg_name() + ";");
  f.ln();
  f.ln("  // A generated package states the protocol, and one module");
  f.ln("  // consumes a subset of it. The constants are the artifact,");
  f.ln("  // not dead code, so UNUSEDPARAM is off across the package.");
  f.ln("  /* verilator lint_off UNUSEDPARAM */");
  f.ln();

  f.ln("  // Channel A opcodes, Table 15");
  f.lines({
    lp("TlAPutFullData",    "3'd0", "logic [2:0]"),
    lp("TlAPutPartialData", "3'd1", "logic [2:0]"),
    lp("TlAArithmeticData", "3'd2", "logic [2:0]"),
    lp("TlALogicalData",    "3'd3", "logic [2:0]"),
    lp("TlAGet",            "3'd4", "logic [2:0]"),
    lp("TlAIntent",         "3'd5", "logic [2:0]"),
    lp("TlAAcquireBlock",   "3'd6", "logic [2:0]"),
    lp("TlAAcquirePerm",    "3'd7", "logic [2:0]")
  });
  f.ln();

  f.ln("  // Channel B opcodes, Table 16");
  f.lines({
    lp("TlBPutFullData",    "3'd0", "logic [2:0]"),
    lp("TlBPutPartialData", "3'd1", "logic [2:0]"),
    lp("TlBArithmeticData", "3'd2", "logic [2:0]"),
    lp("TlBLogicalData",    "3'd3", "logic [2:0]"),
    lp("TlBGet",            "3'd4", "logic [2:0]"),
    lp("TlBIntent",         "3'd5", "logic [2:0]"),
    lp("TlBProbeBlock",     "3'd6", "logic [2:0]"),
    lp("TlBProbePerm",      "3'd7", "logic [2:0]")
  });
  f.ln();

  f.ln("  // Channel C opcodes, Table 17");
  f.lines({
    lp("TlCAccessAck",     "3'd0", "logic [2:0]"),
    lp("TlCAccessAckData", "3'd1", "logic [2:0]"),
    lp("TlCHintAck",       "3'd2", "logic [2:0]"),
    lp("TlCProbeAck",      "3'd4", "logic [2:0]"),
    lp("TlCProbeAckData",  "3'd5", "logic [2:0]"),
    lp("TlCRelease",       "3'd6", "logic [2:0]"),
    lp("TlCReleaseData",   "3'd7", "logic [2:0]")
  });
  f.ln();

  f.ln("  // Channel D opcodes, Table 18");
  f.lines({
    lp("TlDAccessAck",     "3'd0", "logic [2:0]"),
    lp("TlDAccessAckData", "3'd1", "logic [2:0]"),
    lp("TlDHintAck",       "3'd2", "logic [2:0]"),
    lp("TlDGrant",         "3'd4", "logic [2:0]"),
    lp("TlDGrantData",     "3'd5", "logic [2:0]"),
    lp("TlDReleaseAck",    "3'd6", "logic [2:0]")
  });
  f.ln();

  f.ln("  // Cap, grow, prune and report parameters, TL-C");
  f.lines({
    lp("TlCapToT",   "3'd0", "logic [2:0]"),
    lp("TlCapToB",   "3'd1", "logic [2:0]"),
    lp("TlCapToN",   "3'd2", "logic [2:0]"),
    lp("TlGrowNtoB", "3'd0", "logic [2:0]"),
    lp("TlGrowNtoT", "3'd1", "logic [2:0]"),
    lp("TlGrowBtoT", "3'd2", "logic [2:0]"),
    lp("TlPruneTtoB", "3'd0", "logic [2:0]"),
    lp("TlPruneTtoN", "3'd1", "logic [2:0]"),
    lp("TlPruneBtoN", "3'd2", "logic [2:0]"),
    lp("TlReportTtoT", "3'd3", "logic [2:0]"),
    lp("TlReportBtoB", "3'd4", "logic [2:0]"),
    lp("TlReportNtoN", "3'd5", "logic [2:0]")
  });
  f.ln();
  f.ln("  localparam logic [2:0] TlParamZero = 3'd0;");
  f.ln();
  f.ln("  /* verilator lint_on UNUSEDPARAM */");
  f.ln();
  f.ln("endpackage");
}

// --------------------------------------------------------------------
// One link's widths. The bundle in link_sig.cpp is what the port
// lists are built from; this package is what a consumer reads when
// it needs a width by name rather than by position.
// --------------------------------------------------------------------
void RtlPkg::link(SvFile &f, const std::string &name, const LinkSig &s)
{
  f.note("Link '" + name + "', protocol " + s.protocol() +
         (s.is_tl() ? ", conformance " + s.conformance() : ""));
  f.note("");
  f.note("The signal bundle this link carries is derived once by cgen");
  f.note("and emitted into every module that touches the link, so the");
  f.note("two ends of an edge cannot drift apart. This package holds");
  f.note("the widths under names a consumer can use.");
  f.bar();
  f.ln("package " + link_pkg(name) + ";");
  f.ln();
  f.ln("  /* verilator lint_off UNUSEDPARAM */");
  f.ln();
  // Every one of these is wildcard imported into $unit alongside
  // the other link packages and the node packages, so a bare
  // AddrBits would collide with the next link's. The link's own
  // name is the prefix that makes each unique.
  std::string pre;
  {
    bool up = true;
    for(char ch : name) {
      if(ch == '_') { up = true; continue; }
      pre += up ? char(toupper(ch)) : ch;
      up = false;
    }
  }

  f.ln("  // the link name prefixes every constant, because these");
  f.ln("  // packages share a scope with each other and with the");
  f.ln("  // node packages");
  f.lines({
    lp(pre + "AddrBits",  i(s.addr_bits())),
    lp(pre + "DataBits",  i(s.data_bits())),
    lp(pre + "DataBytes", i(s.data_bytes()))
  });
  f.ln();

  f.ln("  // one entry per signal of the bundle, master's view");
  for(const LinkSig::Sig &g : s.sigs()) {
    std::string n = g.local;
    for(char &c : n) if(c == '_') c = ' ';
    // CamelCase the local name so the constant reads as a constant
    std::string cam;
    bool up = true;
    for(char c : n) {
      if(c == ' ') { up = true; continue; }
      cam += up ? char(toupper(c)) : c;
      up = false;
    }
    f.ln(lp(pre + "W" + cam, i(g.bits)) + "   // " +
         (g.m_drives ? "master drives" : "slave drives"));
  }
  f.ln();
  f.ln("  /* verilator lint_on UNUSEDPARAM */");
  f.ln();
  f.ln("endpackage");
}

// --------------------------------------------------------------------
// One node. Geometry, the address decomposition of D-39, the R-7
// addressing unit and the replacement encoding.
// --------------------------------------------------------------------
void RtlPkg::node(SvFile &f, const NodeCtx &c)
{
  const Model::Geom &g = c.geom();
  const int pa  = c.pa_bits();
  const bool banked = g.banks > 1 && g.bank_resolved;

  // ------------------------------------------------------------------
  // An agent carries interfaces and nothing else, D-15, so it has no
  // geometry and no replacement. Its package is only what the link
  // it drives needs, and emitting the cache shape here would put a
  // zero width line type into it.
  // ------------------------------------------------------------------
  if(!g.valid) {
    const NodeCtx::Iface *i0 = c.ifaces().empty() ? nullptr
                                                  : &c.ifaces()[0];
    const int ab = i0 ? i0->sig.addr_bits() : pa;
    const int wb = i0 ? i0->sig.data_bits() : 32;

    f.note("Node '" + c.name() + "', " + c.type() + ".");
    f.note("");
    f.note("No geometry: this node type does not have one, D-17. The");
    f.note("package carries the widths of the link it drives and");
    f.note("nothing else.");
    f.bar();
    f.ln("package " + c.pkg() + ";");
    f.ln();
    f.ln("  /* verilator lint_off UNUSEDPARAM */");
    f.ln();
    f.lines({
      lp("AddrBits",  i(ab)),
      lp("WordBits",  i(wb)),
      lp("WordBytes", i(wb / 8)),
      lp("PaBits",    i(pa))
    });
    f.ln();
    f.ln("  typedef logic [AddrBits-1:0] addr_t;");
    f.ln("  typedef logic [WordBits-1:0] word_t;");
    f.ln();
    f.ln("  /* verilator lint_on UNUSEDPARAM */");
    f.ln();
    f.ln("endpackage");
    return;
  }

  f.note("Node '" + c.name() + "', " + c.type() + ".");
  f.note("");
  f.note("Every number below was DERIVED by cgen from the "
         "configuration.");
  f.note("None of them appears in the input, D-37. The address");
  f.note("decomposition is built once here and read by the cache");
  f.note("control, the testbench and the self checking tests, D-39.");
  f.bar();
  f.ln("package " + c.pkg() + ";");
  f.ln();
  f.ln("  // A generated package states the whole derived geometry and");
  f.ln("  // one module consumes a subset of it. The constants are the");
  f.ln("  // artifact, so UNUSEDPARAM is off across the package.");
  f.ln("  /* verilator lint_off UNUSEDPARAM */");
  f.ln();

  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.ln("  // Geometry");
  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.lines({
    lp("PaBits",        i(pa)),
    lp("CapacityBytes", u(g.capacity_bytes)),
    lp("LineBytes",     u(g.line_bytes)),
    lp("LineBits",      i(int(g.line_bytes) * 8)),
    lp("Ways",          i(g.associativity)),
    lp("Banks",         i(g.banks)),
    lp("Sets",          u(g.sets)),
    lp("SetsPerBank",   u(g.sets_per_bank)),
    lp("BytesPerWay",   u(g.bytes_per_way))
  });
  f.ln();

  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.ln("  // Address decomposition, D-39. offset + index + tag == "
       "pa_bits,");
  f.ln("  // which is what leaves a bank select no room of its own.");
  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.lines({
    lp("OffsetBits", i(g.offset_bits)),
    lp("IndexBits",  i(g.index_bits)),
    lp("TagBits",    i(g.tag_bits)),
    lp("BankBits",   i(g.bank_bits)),
    lp("SetIdxBits", i(c.set_idx_bits())),
    lp("WayBits",    i(c.way_bits()))
  });
  f.ln();
  f.lines({
    lp("OffsetLsb", i(g.offset.lsb)),
    lp("OffsetMsb", i(g.offset.msb)),
    lp("IndexLsb",  i(g.index.lsb)),
    lp("IndexMsb",  i(g.index.msb)),
    lp("TagLsb",    i(g.tag.lsb)),
    lp("TagMsb",    i(g.tag.msb))
  });
  f.ln();

  if(banked) {
    f.ln("  // R-6. The bank select. The index already spans the "
         "whole set");
    f.ln("  // space, so the select is taken OUT of the index rather "
         "than");
    f.ln("  // sitting beside it. " + g.bank_position +
         ", so it is the");
    f.ln("  // " + std::string(g.bank_position == "above_index"
                               ? "top" : "bottom") +
         " " + i(g.bank_bits) + " bits of the index field.");
    f.lines({
      lp("BankLsb",   i(g.bank.lsb)),
      lp("BankMsb",   i(g.bank.msb)),
      lp("SetIdxLsb", i(g.set_index.lsb)),
      lp("SetIdxMsb", i(g.set_index.msb))
    });
    f.ln();
  } else if(g.banks > 1) {
    f.ln("  // R-6. THE BANK FIELD IS UNRESOLVED and no bounds are");
    f.ln("  // emitted for it. " + g.bank_note);
    f.ln();
  } else {
    f.lines({
      lp("SetIdxLsb", i(g.index.lsb)),
      lp("SetIdxMsb", i(g.index.msb))
    });
    f.ln();
  }

  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.ln("  // R-7. THE OFFSET UNIT IS THE BYTE. The offset field of an");
  f.ln("  // address is a byte offset within the line.");
  f.ln("  //");
  f.ln("  // AddrUnitBytes is the size of ONE ADDRESS UNIT in bytes.");
  f.ln("  // It is 1, which is byte addressing. Every word count and");
  f.ln("  // every word index below derives from it and from nothing");
  f.ln("  // else, so reversing this design to word based addressing");
  f.ln("  // is one edit, on the AddrUnitBytes line, in this one file.");
  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.ln(lp("AddrUnitBytes", "1"));
  f.ln();
  f.lines({
    lp("WordBytes",    i(c.core_data_bits() / 8)),
    lp("WordBits",     "WordBytes * 8"),
    lp("WordsPerLine", "LineBytes / WordBytes"),
    lp("WordIdxBits",  "$clog2(WordsPerLine)")
  });
  // wrapped where they are written, because the node prefix is added
  // at write time and lp() cannot see the length these end up at
  f.ln("  localparam int unsigned WordIdxLsb =");
  f.ln("      $clog2(WordBytes / AddrUnitBytes);");
  f.ln(lp("WordIdxMsb", "WordIdxLsb + WordIdxBits - 1"));
  f.ln();
  f.lines({
    lp("BeatBits",  i(c.mem_data_bits() > 0 ? c.mem_data_bits()
                                            : c.core_data_bits())),
    lp("BeatBytes", i((c.mem_data_bits() > 0 ? c.mem_data_bits()
                                             : c.core_data_bits()) / 8)),
    lp("Beats",     i(c.refill_beats() > 0 ? c.refill_beats() : 1))
  });
  f.ln("  localparam int unsigned BeatIdxBits =");
  f.ln("      $clog2(Beats) > 0 ? $clog2(Beats) : 1;");
  f.ln();

  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.ln("  // Types");
  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.ln("  typedef logic [PaBits-1:0]     addr_t;");
  f.ln("  typedef logic [TagBits-1:0]    tag_t;");
  f.ln("  typedef logic [SetIdxBits-1:0] set_t;");
  f.ln("  typedef logic [WayBits-1:0]    way_t;");
  f.ln("  typedef logic [LineBits-1:0]   line_t;");
  f.ln("  typedef logic [WordBits-1:0]   word_t;");
  f.ln("  typedef logic [BeatBits-1:0]   beat_t;");
  if(banked) f.ln("  typedef logic [BankBits-1:0]   bank_t;");
  f.ln();

  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.ln("  // The decomposition itself. Written as a shift and a cast");
  f.ln("  // rather than a part select on purpose: a part select "
       "inside a");
  f.ln("  // function leaves the other bits of the argument unread, "
       "and");
  f.ln("  // that draws an UNUSEDSIGNAL warning from the linter. The");
  f.ln("  // two forms are the same hardware.");
  f.ln("  // -----------------------------------------------------"
       "-----------");
  f.ln("  function automatic tag_t tag_of(input addr_t a);");
  f.ln("    tag_of = tag_t'((a & {PaBits{1'b1}}) >> TagLsb);");
  f.ln("  endfunction");
  f.ln();
  f.ln("  function automatic set_t set_of(input addr_t a);");
  f.ln("    set_of = set_t'((a & {PaBits{1'b1}}) >> SetIdxLsb);");
  f.ln("  endfunction");
  f.ln();
  if(banked) {
    f.ln("  function automatic bank_t bank_of(input addr_t a);");
    f.ln("    bank_of = bank_t'((a & {PaBits{1'b1}}) >> BankLsb);");
    f.ln("  endfunction");
    f.ln();
  }
  f.ln("  function automatic logic [OffsetBits-1:0]");
  f.ln("      offset_of(input addr_t a);");
  f.ln("    offset_of = OffsetBits'((a & {PaBits{1'b1}}) >> "
       "OffsetLsb);");
  f.ln("  endfunction");
  f.ln();
  f.ln("  function automatic logic [WordIdxBits-1:0]");
  f.ln("      word_of(input addr_t a);");
  f.ln("    word_of = WordIdxBits'((a & {PaBits{1'b1}}) >> "
       "WordIdxLsb);");
  f.ln("  endfunction");
  f.ln();
  f.ln("  // the line this address falls in, offset cleared");
  f.ln("  function automatic addr_t line_base(input addr_t a);");
  f.ln("    line_base = ((a & {PaBits{1'b1}}) >> OffsetBits) "
       "<< OffsetBits;");
  f.ln("  endfunction");
  f.ln();
  f.ln("  // the address a tag and a set stand for, the inverse of");
  f.ln("  // tag_of and set_of. A writeback needs it.");
  if(banked) {
    f.ln("  function automatic addr_t line_addr");
    f.ln("      (input tag_t t, input bank_t b, input set_t s);");
    f.ln("    line_addr = (addr_t'(t) << TagLsb)");
    f.ln("              | (addr_t'(b) << BankLsb)");
    f.ln("              | (addr_t'(s) << SetIdxLsb);");
    f.ln("  endfunction");
  } else {
    f.ln("  function automatic addr_t line_addr");
    f.ln("      (input tag_t t, input set_t s);");
    f.ln("    line_addr = (addr_t'(t) << TagLsb)");
    f.ln("              | (addr_t'(s) << SetIdxLsb);");
    f.ln("  endfunction");
  }
  f.ln();

  if(c.is_cache()) {
    f.lines(c.repl().package_text());
    f.ln();
  }

  f.ln("  /* verilator lint_on UNUSEDPARAM */");
  f.ln();
  f.ln("endpackage");
}

} // namespace cgen
