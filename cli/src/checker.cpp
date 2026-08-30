// --------------------------------------------------------------------
// FILE:    checker.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "checker.h"
#include "diag_codes.h"
#include "field_use.h"
#include "msg.h"
#include <algorithm>

using nlohmann::json;
using Kind = cgen::SymbolTable::Kind;

namespace cgen
{

namespace {

// ------------------------------------------------------------------
// Join a name list for a message.
// ------------------------------------------------------------------
std::string join(const std::vector<std::string> &v, const char *sep)
{
  std::string s;
  for(size_t i = 0; i < v.size(); ++i) {
    if(i) s += sep;
    s += v[i];
  }
  return s;
}

bool has_member(const json &g, const std::string &k)
{
  return g.is_object() && g.contains(k);
}

std::string node_type_of(const json &c)
{
  if(c.is_object() && c.contains("node_type") &&
     c["node_type"].is_string()) {
    return c["node_type"].get<std::string>();
  }
  return "";
}

} // namespace

// --------------------------------------------------------------------
std::string Checker::role_of(const std::string &port_type) const
{
  const SymbolTable::Entry *e = syms_.find(Kind::PortType, port_type);
  if(e == nullptr || e->body == nullptr)             return "";
  if(!e->body->is_object() || !e->body->contains("role")) return "";
  if(!(*e->body)["role"].is_string())                return "";
  cfg_read(e->file, e->path + "/role");
  return (*e->body)["role"].get<std::string>();
}

// --------------------------------------------------------------------
// T-3, both ends of every edge carry the port type the link declares.
// An end whose port type never resolved is skipped, T-1 reported it.
// --------------------------------------------------------------------
void Checker::port_types(Model &m)
{
  for(const Model::Edge &e : m.edges) {
    if(!e.link_ok) continue;

    struct End {
      const std::string *have;
      const std::string *want;
      const char        *key;
      const std::string *node;
    };
    const End ends[2] = {
      { &e.from_port_type, &e.link_master_type, "from_port", &e.from },
      { &e.to_port_type,   &e.link_slave_type,  "to_port",   &e.to   }
    };

    for(const End &en : ends) {
      if(en.have->empty() || en.want->empty())            continue;
      if(syms_.find(Kind::PortType, *en.have) == nullptr) continue;
      if(syms_.find(Kind::PortType, *en.want) == nullptr) continue;
      if(*en.have == *en.want)                            continue;

      diags_.error(e.file, e.path + "/" + en.key, code::t3_port_type,
                   "edge " + msg->tq(m.label(e)) + " " + en.key +
                   " on node " + msg->tq(*en.node) + " is port type " +
                   msg->tq(*en.have) + " but link " + msg->tq(e.link) +
                   " declares " + msg->tq(*en.want));
    }
  }
}

// --------------------------------------------------------------------
// T-4, the from end of an edge is the master and the to end is the
// slave, D-29. Master is the requesting side, TileLink 1.9.3.
// --------------------------------------------------------------------
void Checker::port_roles(Model &m)
{
  for(const Model::Edge &e : m.edges) {
    struct End {
      const std::string *type;
      const char        *want;
      const char        *key;
      const std::string *node;
    };
    const End ends[2] = {
      { &e.from_port_type, "master", "from_port", &e.from },
      { &e.to_port_type,   "slave",  "to_port",   &e.to   }
    };

    for(const End &en : ends) {
      if(en.type->empty()) continue;

      std::string role = role_of(*en.type);
      if(role.empty())     continue;    // undefined type, T-1 has it
      if(role == en.want)  continue;

      diags_.error(e.file, e.path + "/" + en.key, code::t4_port_role,
                   "edge " + msg->tq(m.label(e)) + " " + en.key +
                   " on node " + msg->tq(*en.node) + " is port type " +
                   msg->tq(*en.type) + " whose role is " + role +
                   ", the " + en.key + " end must be " + en.want);
    }
  }
}

// --------------------------------------------------------------------
// T-5. A finite graph fails to terminate exactly when it holds a
// cycle, so cycle detection is the whole check.
// --------------------------------------------------------------------
void Checker::visit(Model &m,
                    const std::string &n,
                    std::vector<std::string> &path,
                    std::map<std::string, int> &state,
                    std::set<std::string> &reported)
{
  state[n] = 1;                       // 1 on the current path
  path.push_back(n);

  for(const Model::Edge &e : m.edges) {
    if(!e.from_ok || !e.to_ok) continue;
    if(e.from != n)            continue;

    int s = state.count(e.to) ? state[e.to] : 0;

    if(s == 1) {
      auto it = std::find(path.begin(), path.end(), e.to);
      std::vector<std::string> loop(it, path.end());
      loop.push_back(e.to);

      std::set<std::string> key(loop.begin(), loop.end());
      std::string tag = join(std::vector<std::string>(key.begin(),
                                                      key.end()), ",");
      if(!reported.count(tag)) {
        reported.insert(tag);
        diags_.error(e.file, e.path, code::t5_cycle,
                     "graph does not terminate, cycle " +
                     join(loop, " -> "));
      }
      continue;
    }

    if(s == 0) visit(m, e.to, path, state, reported);
  }

  path.pop_back();
  state[n] = 2;                       // 2 done
}

// --------------------------------------------------------------------
void Checker::graph(Model &m)
{
  std::map<std::string, int> state;
  std::set<std::string>      reported;
  std::vector<std::string>   path;

  for(const Model::Node &n : m.nodes) {
    if(state.count(n.name) && state[n.name] != 0) continue;
    visit(m, n.name, path, state, reported);
  }
}

// --------------------------------------------------------------------
// T-6. A field group is wholly present or wholly absent, D-18. The
// schemas do not say what wholly present means, so the membership
// table below is tool policy.
//
//   policies       icache             read_miss, replacement
//                  dcache, unified    the above plus write_miss,
//                                     write_hit
//   miss_handling  mshrs, victim_buffer_entries, fill_buffer_entries
//                  mshr_targets is handled by the schema, it is
//                  required exactly when mshrs is at least one
//   fill           beat_order, bypass_to_upstream
//   timing         read_latency_cycles, emit_parameters, plus
//                  write_throughput_cycles unless icache, plus
//                  tag_compare_stage unless memory
//   storage        memory             data_array
//                  cache types        data_array_organization,
//                                     way_access, tag_array,
//                                     data_array, valid_bits,
//                                     replacement_bits, plus
//                                     dirty_bits when write_hit is
//                                     write_back
//   maintenance    invalidate_line, invalidate_all, flush_line,
//                  flush_all
// --------------------------------------------------------------------
void Checker::group_of(const SymbolTable::Entry &c,
                       const char *group,
                       const std::vector<std::string> &want)
{
  if(c.body == nullptr || !c.body->is_object()) return;
  if(!c.body->contains(group))                  return;   // absent, ok

  const json &g = (*c.body)[group];
  if(!g.is_object())                            return;

  std::vector<std::string> missing;
  for(const std::string &k : want) {
    if(!has_member(g, k)) missing.push_back(k);
  }
  if(missing.empty()) return;

  diags_.error(c.file, c.path + "/" + group, code::t6_group,
               "cache " + msg->tq(c.name) + " group " +
               msg->tq(group) + " is partly populated, missing " +
               join(missing, ", "));
}

// --------------------------------------------------------------------
void Checker::groups()
{
  for(const auto &kv : syms_.of(Kind::Cache)) {
    const SymbolTable::Entry &c = kv.second;
    if(c.body == nullptr || !c.body->is_object()) continue;

    const json       &b  = *c.body;
    const std::string ct = node_type_of(b);
    // an agent or an interconnect carries interfaces and nothing else
    if(ct == "agent" || ct == "interconnect") continue;

    bool is_i   = ct == "icache";
    bool is_mem = ct == "memory";

    std::vector<std::string> pol = { "read_miss", "replacement" };
    if(!is_i) {
      pol.push_back("write_miss");
      pol.push_back("write_hit");
    }
    group_of(c, "policies", pol);

    group_of(c, "miss_handling",
             { "mshrs", "victim_buffer_entries", "fill_buffer_entries" });

    group_of(c, "fill", { "beat_order", "bypass_to_upstream" });

    std::vector<std::string> tim = { "read_latency_cycles",
                                     "emit_parameters" };
    if(!is_i)   tim.push_back("write_throughput_cycles");
    if(!is_mem) tim.push_back("tag_compare_stage");
    group_of(c, "timing", tim);

    std::vector<std::string> sto;
    if(is_mem) {
      sto = { "data_array" };
    } else {
      sto = { "data_array_organization", "way_access", "tag_array",
              "data_array", "valid_bits", "replacement_bits" };
      // the VALUE of write_hit, not its presence: it decides
      // whether the storage group has to carry dirty_bits
      bool wb = b.contains("policies") && b["policies"].is_object() &&
                b["policies"].contains("write_hit") &&
                b["policies"]["write_hit"].is_string() &&
                b["policies"]["write_hit"].get<std::string>() ==
                  "write_back";
      if(b.contains("policies") && b["policies"].is_object() &&
         b["policies"].contains("write_hit")) {
        cfg_read(c.file, c.path + "/policies/write_hit");
      }
      if(wb) sto.push_back("dirty_bits");
    }
    group_of(c, "storage", sto);

    group_of(c, "maintenance",
             { "invalidate_line", "invalidate_all", "flush_line",
               "flush_all" });
  }
}

// --------------------------------------------------------------------
// T-7. D-5 settles Q-04 for the slave end: one slave port may host
// more than one edge, so the count is recorded and nothing is
// reported. The master end is left open, see the task file.
// --------------------------------------------------------------------
void Checker::occupancy(Model &m)
{
  m.occupancy.clear();
  for(const Model::Edge &e : m.edges) {
    if(e.from_ok && !e.from_iface.empty() && !e.from_port.empty()) {
      ++m.occupancy[e.from + "." + e.from_iface + "." + e.from_port];
    }
    if(e.to_ok && !e.to_iface.empty() && !e.to_port.empty()) {
      ++m.occupancy[e.to + "." + e.to_iface + "." + e.to_port];
    }
  }
}

// --------------------------------------------------------------------
// T-9. The interface at each end of an edge carries a link. Both ends
// have to name the same one. An end whose link never resolved is
// skipped, T-1.iface_link reported it.
// --------------------------------------------------------------------
void Checker::link_agree(Model &m)
{
  for(const Model::Edge &e : m.edges) {
    if(e.from_link.empty() || e.to_link.empty())          continue;
    if(e.from_link == e.to_link)                          continue;
    if(syms_.find(Kind::Link, e.from_link) == nullptr)    continue;
    if(syms_.find(Kind::Link, e.to_link)   == nullptr)    continue;

    diags_.error(e.file, e.path, code::t9_link_agree,
                 "edge " + msg->tq(m.label(e)) + " interface " +
                 msg->tq(e.from_iface) + " on node " +
                 msg->tq(e.from) + " carries link " +
                 msg->tq(e.from_link) + " but interface " +
                 msg->tq(e.to_iface) + " on node " + msg->tq(e.to) +
                 " carries link " + msg->tq(e.to_link) +
                 ", both ends of an edge must agree");
  }
}

// --------------------------------------------------------------------
// T-10. The address width a link declares. The field is named per
// protocol, so the lookup is per protocol too.
// --------------------------------------------------------------------
bool Checker::link_addr_bits(const std::string &link,
                             int &bits,
                             std::string &file,
                             std::string &ptr) const
{
  const SymbolTable::Entry *l = syms_.find(Kind::Link, link);
  if(l == nullptr || l->body == nullptr) return false;
  if(!l->body->is_object())              return false;

  const json &b = *l->body;
  if(!b.contains("protocol") || !b["protocol"].is_string()) return false;

  const std::string proto = b["protocol"].get<std::string>();

  const char *group = nullptr;
  const char *key   = nullptr;
  if(proto == "tilelink") { group = "tilelink"; key = "address_bits"; }
  else if(proto == "custom") { group = "custom";
                               key   = "address_width_bits"; }
  else return false;

  if(!b.contains(group) || !b[group].is_object()) return false;
  const json &g = b[group];
  if(!g.contains(key) || !g[key].is_number_integer()) return false;

  bits = g[key].get<int>();
  file = l->file;
  ptr  = l->path + "/" + group + "/" + key;
  cfg_read(file, ptr);
  return true;
}

// --------------------------------------------------------------------
// T-10. A node whose emitted address type is pa_bits wide. Every
// cache and the memory decompose an address; an agent and an
// interconnect do not, and neither does a node whose type never
// resolved.
// --------------------------------------------------------------------
bool Checker::is_addressed(const Model &m, const std::string &node) const
{
  const Model::Node *n = m.node(node);
  if(n == nullptr) return false;

  const std::string &t = n->node_type;
  return t == "icache" || t == "dcache" || t == "unified" ||
         t == "memory";
}

// --------------------------------------------------------------------
// T-10. A link address width and the system pa_bits are independent
// integers in two documents and nothing else compares them. The
// generated address type is pa_bits wide, so a link that is narrower
// zero extends on the way into a node and truncates on the way out,
// and a tree in that state builds and runs.
//
// The rule fires on an edge that touches a cache or a memory node,
// because that is where the address meets a pa_bits wide type. An
// agent to agent edge carries no such type and is left alone.
//
// One diagnostic per offending LINK, not per edge. The width is one
// value in one file, so one edit clears it, and a link carried by two
// edges would otherwise report the same field twice.
// --------------------------------------------------------------------
void Checker::addr_width(Model &m)
{
  if(!m.has_addressing) return;    // T-8.no_addressing has that case

  std::set<std::string> reported;

  for(const Model::Edge &e : m.edges) {
    if(!e.link_ok || e.link.empty())         continue;
    if(reported.count(e.link))               continue;
    if(!is_addressed(m, e.from) && !is_addressed(m, e.to)) continue;

    int         bits = 0;
    std::string file;
    std::string ptr;
    if(!link_addr_bits(e.link, bits, file, ptr)) continue;
    if(bits == m.pa_bits)                        continue;

    reported.insert(e.link);
    diags_.error(file, ptr, code::t10_addr_width,
                 "link " + msg->tq(e.link) + " declares an address "
                 "width of " + std::to_string(bits) + " bits and the "
                 "system pa_bits is " + std::to_string(m.pa_bits) +
                 ". Edge " + msg->tq(m.label(e)) + " carries it into "
                 "a node whose address type is pa_bits wide, so the "
                 "two must agree");
  }
}

// --------------------------------------------------------------------
void Checker::run(Model &m)
{
  link_agree(m);
  port_types(m);
  port_roles(m);
  addr_width(m);
  graph(m);
  groups();
  occupancy(m);
}

} // namespace cgen
