// --------------------------------------------------------------------
// FILE:    report.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "report.h"
#include "msg.h"
#include <cstdio>

namespace cgen
{

namespace {
std::string u64(uint64_t v) { return std::to_string(v); }
std::string i2s(int v)      { return std::to_string(v); }
}

// --------------------------------------------------------------------
std::string Report::hex(uint64_t v, int bits)
{
  int digits = (bits + 3) / 4;
  if(digits < 1)  digits = 1;
  if(digits > 16) digits = 16;

  char buf[32];
  std::snprintf(buf, sizeof(buf), "0x%0*llx", digits,
                (unsigned long long)v);
  return std::string(buf);
}

// --------------------------------------------------------------------
std::string Report::field(const char *name, const Model::Field &f,
                          int addr_bits)
{
  std::string s = name;
  while(s.size() < 7) s += " ";
  if(!f.valid) return s + "not applicable";

  return s + "[" + i2s(f.msb) + ":" + i2s(f.lsb) + "] bits " +
         i2s(f.bits) + " shift " + i2s(f.shift) + " mask " +
         hex(f.mask, addr_bits);
}

// --------------------------------------------------------------------
void Report::nodes(const Model &m) const
{
  msg->imsg("");
  msg->imsg("Nodes, " + i2s(int(m.nodes.size())) + " total");

  for(const Model::Node &n : m.nodes) {
    std::string head = "  node " + msg->tq(n.name) + " cache " +
                       msg->tq(n.cache);
    if(!n.node_type.empty()) head += " type " + n.node_type;
    if(!n.indexing.empty())   head += " indexing " + n.indexing;
    msg->imsg(head);

    if(!n.resolved) {
      msg->imsg("    unresolved, no geometry derived");
      continue;
    }

    const Model::Geom &g = n.geom;
    if(!g.valid && g.capacity_bytes == 0) {
      msg->imsg("    no geometry block, nothing to derive");
      continue;
    }

    msg->imsg("    capacity " + u64(g.capacity_bytes) + "  line " +
              u64(g.line_bytes) + "  ways " + i2s(g.associativity) +
              "  banks " + i2s(g.banks));

    if(!g.valid) {
      msg->imsg("    derivation incomplete, see the diagnostics");
      continue;
    }

    msg->imsg("    sets " + u64(g.sets) + "  sets per bank " +
              u64(g.sets_per_bank) + "  bytes per way " +
              u64(g.bytes_per_way));

    std::string beats = g.refill_beats >= 0 ? i2s(g.refill_beats)
                                            : std::string("n/a");
    std::string line  = "    refill beats " + beats;
    if(!g.refill_note.empty()) line += "  (" + g.refill_note + ")";
    msg->imsg(line);

    msg->imsg("    bits offset " + i2s(g.offset_bits) + "  index " +
              i2s(g.index_bits) + "  tag " + i2s(g.tag_bits) +
              "  bank " +
              (g.bank_bits >= 0 ? i2s(g.bank_bits)
                                : std::string("n/a")));

    msg->imsg("    " + field("offset", g.offset, m.pa_bits));
    msg->imsg("    " + field("index",  g.index,  m.pa_bits));
    msg->imsg("    " + field("tag",    g.tag,    m.pa_bits));

    // R-6, CLI-004. Only worth a line when there is a bank to select.
    if(g.banks > 1) {
      if(g.bank_resolved) {
        msg->imsg("    " + field("bank",  g.bank,      m.pa_bits));
        msg->imsg("    " + field("setidx", g.set_index, m.pa_bits));
      } else {
        msg->imsg("    bank  field UNRESOLVED");
      }
      if(!g.bank_note.empty()) msg->imsg("      " + g.bank_note);
    }
  }
}

// --------------------------------------------------------------------
void Report::edges(const Model &m) const
{
  msg->imsg("");
  msg->imsg("Edges, " + i2s(int(m.edges.size())) + " total");

  for(const Model::Edge &e : m.edges) {
    std::string a = e.from;
    if(!e.from_iface.empty()) a += "." + e.from_iface;
    if(!e.from_port.empty())  a += "." + e.from_port;
    std::string b = e.to;
    if(!e.to_iface.empty())   b += "." + e.to_iface;
    if(!e.to_port.empty())    b += "." + e.to_port;
    msg->imsg("  edge " + msg->tq(m.label(e)) + "  " + a + " -> " + b);

    if(!e.link_ok) {
      msg->imsg("    link " + msg->tq(e.from_link) + " -> " +
                msg->tq(e.to_link) + "  unresolved");
      continue;
    }
    std::string l = "    link " + msg->tq(e.link);
    if(!e.protocol.empty())    l += " protocol " + e.protocol;
    if(!e.conformance.empty()) l += " " + e.conformance;
    if(e.width_known)          l += " width " + i2s(e.width_bytes) +
                                    " bytes";
    msg->imsg(l);

    msg->imsg("    port types " +
              (e.from_port_type.empty() ? std::string("?")
                                        : e.from_port_type) +
              " -> " +
              (e.to_port_type.empty() ? std::string("?")
                                      : e.to_port_type));
  }
}

// --------------------------------------------------------------------
// T-7 is recorded rather than reported, D-5 allows a slave port to
// host more than one edge.
// --------------------------------------------------------------------
void Report::ports(const Model &m) const
{
  bool any = false;
  for(const auto &kv : m.occupancy) {
    if(kv.second > 1) any = true;
  }
  if(!any) return;

  msg->imsg("");
  msg->imsg("Shared ports, more than one edge, allowed by D-5");
  for(const auto &kv : m.occupancy) {
    if(kv.second <= 1) continue;
    msg->imsg("  " + kv.first + "  " + i2s(kv.second) + " edges");
  }
}

// --------------------------------------------------------------------
void Report::diagnostics(const DiagList &d) const
{
  msg->imsg("");
  msg->imsg("Diagnostics, " + i2s(int(d.size())) + " total, " +
            i2s(int(d.error_count())) + " errors");
  d.print();
}

// --------------------------------------------------------------------
void Report::summary(const Model &m, const DiagList &d) const
{
  msg->imsg("");
  std::string s = "system ";
  s += m.system_name.empty() ? std::string("<unnamed>") : m.system_name;
  s += "  pa_bits " + (m.has_addressing ? i2s(m.pa_bits)
                                        : std::string("n/a"));
  s += "  va_bits " + (m.has_va_bits ? i2s(m.va_bits)
                                     : std::string("n/a"));
  s += "  page_bytes " + (m.has_page_bytes ? i2s(m.page_bytes)
                                           : std::string("n/a"));
  msg->imsg(s);
  msg->imsg("check " + std::string(d.has_error() ? "FAILED" : "passed"));
}

// --------------------------------------------------------------------
void Report::check(const Model &m, const DiagList &d) const
{
  nodes(m);
  edges(m);
  ports(m);
  diagnostics(d);
  summary(m, d);
}

} // namespace cgen
