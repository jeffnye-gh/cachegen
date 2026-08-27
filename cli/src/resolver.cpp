// --------------------------------------------------------------------
// FILE:    resolver.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "resolver.h"
#include "msg.h"

using nlohmann::json;
using Kind = cgen::SymbolTable::Kind;

namespace cgen
{

// --------------------------------------------------------------------
const json *Resolver::iface_of(const json *body,
                               const std::string &iface)
{
  if(body == nullptr || !body->is_object())      return nullptr;
  if(!body->contains("interfaces"))              return nullptr;
  const json &i = (*body)["interfaces"];
  if(!i.is_object() || !i.contains(iface))       return nullptr;
  if(!i[iface].is_object())                      return nullptr;
  return &i[iface];
}

// --------------------------------------------------------------------
std::string Resolver::port_type_of(const json *body,
                                   const std::string &iface,
                                   const std::string &port)
{
  const json *f = iface_of(body, iface);
  if(f == nullptr || !f->contains("ports"))      return "";
  const json &p = (*f)["ports"];
  if(!p.is_object() || !p.contains(port))        return "";
  if(!p[port].is_string())                       return "";
  return p[port].get<std::string>();
}

// --------------------------------------------------------------------
std::string Resolver::link_of(const json *body,
                              const std::string &iface)
{
  const json *f = iface_of(body, iface);
  if(f == nullptr || !f->contains("link"))       return "";
  if(!(*f)["link"].is_string())                  return "";
  return (*f)["link"].get<std::string>();
}

// --------------------------------------------------------------------
// Every topology node names a node definition, D-35.
// --------------------------------------------------------------------
void Resolver::bind_nodes(Model &m)
{
  const auto &nodes = syms_.of(Kind::Node);

  for(const auto &kv : nodes) {
    const SymbolTable::Entry &e = kv.second;

    Model::Node n;
    n.name = e.name;
    n.file = e.file;
    n.path = e.path;

    if(e.body && e.body->is_object() && e.body->contains("cache") &&
       (*e.body)["cache"].is_string()) {
      n.cache = (*e.body)["cache"].get<std::string>();
    }

    if(n.cache.empty()) {
      diags_.error(n.file, n.path + "/cache", "T-1.node_cache",
                   "node " + msg->tq(n.name) +
                   " does not name a cache definition");
      m.nodes.push_back(n);
      continue;
    }

    const SymbolTable::Entry *c = syms_.find(Kind::Cache, n.cache);
    if(c == nullptr) {
      diags_.error(n.file, n.path + "/cache", "T-1.node_cache",
                   "node " + msg->tq(n.name) +
                   " names cache definition " + msg->tq(n.cache) +
                   " which is not defined");
      m.nodes.push_back(n);
      continue;
    }

    n.resolved   = true;
    n.body       = c->body;
    n.cache_file = c->file;
    n.cache_path = c->path;

    if(c->body && c->body->is_object()) {
      if(c->body->contains("node_type") &&
         (*c->body)["node_type"].is_string()) {
        n.node_type = (*c->body)["node_type"].get<std::string>();
      }
      if(c->body->contains("indexing") &&
         (*c->body)["indexing"].is_string()) {
        n.indexing = (*c->body)["indexing"].get<std::string>();
      }
    }

    m.nodes.push_back(n);
  }
}

// --------------------------------------------------------------------
// Every interface names one link and a map of port instances to port
// types. Both sides of that are resolved here, once per definition,
// so an unused interface is checked as well as an attached one.
// --------------------------------------------------------------------
void Resolver::bind_interfaces()
{
  for(const auto &kv : syms_.of(Kind::Cache)) {
    const SymbolTable::Entry &c = kv.second;
    if(c.body == nullptr || !c.body->is_object())  continue;
    if(!c.body->contains("interfaces"))            continue;

    const json &ifaces = (*c.body)["interfaces"];
    if(!ifaces.is_object()) continue;

    for(auto fi = ifaces.begin(); fi != ifaces.end(); ++fi) {
      if(!fi.value().is_object()) continue;
      const std::string site = c.path + "/interfaces/" + fi.key();

      // the link the interface carries
      if(fi.value().contains("link") && fi.value()["link"].is_string()) {
        std::string ln = fi.value()["link"].get<std::string>();
        if(syms_.find(Kind::Link, ln) == nullptr) {
          diags_.error(c.file, site + "/link", "T-1.iface_link",
                       "node " + msg->tq(c.name) + " interface " +
                       msg->tq(fi.key()) + " names link " +
                       msg->tq(ln) + " which is not defined");
        }
      }

      // the port types the interface carries
      if(!fi.value().contains("ports"))         continue;
      const json &ports = fi.value()["ports"];
      if(!ports.is_object())                    continue;

      for(auto pi = ports.begin(); pi != ports.end(); ++pi) {
        if(!pi.value().is_string()) continue;
        std::string pt = pi.value().get<std::string>();
        if(syms_.find(Kind::PortType, pt) != nullptr) continue;

        diags_.error(c.file, site + "/ports/" + pi.key(),
                     "T-1.port_type",
                     "node " + msg->tq(c.name) + " interface " +
                     msg->tq(fi.key()) + " port " + msg->tq(pi.key()) +
                     " names port type " + msg->tq(pt) +
                     " which is not defined");
      }
    }
  }
}

// --------------------------------------------------------------------
// Both ends of every link definition name a port type.
// --------------------------------------------------------------------
void Resolver::bind_link_ports()
{
  const char *ends[2] = { "master_port_type", "slave_port_type" };

  for(const auto &kv : syms_.of(Kind::Link)) {
    const SymbolTable::Entry &l = kv.second;
    if(l.body == nullptr || !l.body->is_object()) continue;

    for(const char *end : ends) {
      if(!l.body->contains(end) || !(*l.body)[end].is_string()) continue;
      std::string pt = (*l.body)[end].get<std::string>();
      if(syms_.find(Kind::PortType, pt) != nullptr) continue;

      diags_.error(l.file, l.path + "/" + end, "T-1.link_port_type",
                   "link " + msg->tq(l.name) + " " + end + " names " +
                   msg->tq(pt) + " which is not defined");
    }
  }
}

// --------------------------------------------------------------------
void Resolver::collect_edges(const std::vector<Loader::File> &files,
                             Model &m)
{
  struct Take { const char *key; std::string Model::Edge::*field; };
  static const Take take[] = {
    { "name",           &Model::Edge::name       },
    { "from",           &Model::Edge::from       },
    { "to",             &Model::Edge::to         },
    { "from_interface", &Model::Edge::from_iface },
    { "to_interface",   &Model::Edge::to_iface   },
    { "from_port",      &Model::Edge::from_port  },
    { "to_port",        &Model::Edge::to_port    }
  };

  for(const Loader::File &f : files) {
    if(f.declared != "topology")                     continue;
    if(!f.doc.contains("edges") || !f.doc["edges"].is_array()) continue;

    const json &edges = f.doc["edges"];
    for(size_t i = 0; i < edges.size(); ++i) {
      const json &j = edges[i];
      if(!j.is_object()) continue;

      Model::Edge e;
      e.file = f.disp;
      e.path = "/edges/" + std::to_string(i);

      for(const Take &t : take) {
        if(j.contains(t.key) && j[t.key].is_string()) {
          e.*(t.field) = j[t.key].get<std::string>();
        }
      }

      m.edges.push_back(e);
    }
  }
}

// --------------------------------------------------------------------
// One end of one edge: the node, then the interface on the node
// definition behind it, then the port instance in that interface.
// --------------------------------------------------------------------
void Resolver::bind_edge_end(Model &m, Model::Edge &e, bool from_end)
{
  const char *key   = from_end ? "from" : "to";
  const std::string &node  = from_end ? e.from       : e.to;
  const std::string &iface = from_end ? e.from_iface : e.to_iface;
  const std::string &port  = from_end ? e.from_port  : e.to_port;
  std::string &type = from_end ? e.from_port_type : e.to_port_type;
  std::string &link = from_end ? e.from_link      : e.to_link;
  bool &ok = from_end ? e.from_ok : e.to_ok;

  ok = false;

  if(node.empty()) {
    diags_.error(e.file, e.path + "/" + key, "T-1.edge_endpoint",
                 "edge " + msg->tq(m.label(e)) + " has no " + key +
                 " endpoint");
    return;
  }
  if(syms_.find(Kind::Node, node) == nullptr) {
    diags_.error(e.file, e.path + "/" + key, "T-1.edge_endpoint",
                 "edge " + msg->tq(m.label(e)) + " " + key +
                 " endpoint " + msg->tq(node) +
                 " is not a topology node");
    return;
  }
  ok = true;

  const Model::Node *n = m.node(node);
  if(n == nullptr || !n->resolved) return;

  const std::string ikey = std::string(key) + "_interface";
  if(iface.empty()) {
    diags_.error(e.file, e.path + "/" + ikey, "T-1.edge_interface",
                 "edge " + msg->tq(m.label(e)) +
                 " does not name a " + ikey + " on node " +
                 msg->tq(node));
    return;
  }
  if(iface_of(n->body, iface) == nullptr) {
    diags_.error(e.file, e.path + "/" + ikey, "T-1.edge_interface",
                 "edge " + msg->tq(m.label(e)) + " names interface " +
                 msg->tq(iface) + " which node " + msg->tq(node) +
                 " does not have, its definition is " +
                 msg->tq(n->cache));
    return;
  }

  link = link_of(n->body, iface);

  const std::string pkey = std::string(key) + "_port";
  if(port.empty()) {
    diags_.error(e.file, e.path + "/" + pkey, "T-1.edge_port",
                 "edge " + msg->tq(m.label(e)) + " does not name a " +
                 pkey + " on node " + msg->tq(node));
    return;
  }

  std::string pt = port_type_of(n->body, iface, port);
  if(pt.empty()) {
    diags_.error(e.file, e.path + "/" + pkey, "T-1.edge_port",
                 "edge " + msg->tq(m.label(e)) + " names port " +
                 msg->tq(port) + " which interface " + msg->tq(iface) +
                 " on node " + msg->tq(node) + " does not have");
    return;
  }
  type = pt;
}

// --------------------------------------------------------------------
// The link is not on the edge, both ends carry one. They have to name
// the same definition. Disagreement is reported by the checker, T-9,
// so that this stage stays a binding stage.
// --------------------------------------------------------------------
void Resolver::bind_edge_link(Model &m, Model::Edge &e)
{
  (void)m;
  if(e.from_link.empty() || e.to_link.empty()) return;
  if(e.from_link != e.to_link)                 return;

  const SymbolTable::Entry *l = syms_.find(Kind::Link, e.from_link);
  if(l == nullptr || l->body == nullptr)       return;  // T-1 has it

  e.link    = e.from_link;
  e.link_ok = true;

  const json &b = *l->body;
  if(b.contains("master_port_type") && b["master_port_type"].is_string())
    e.link_master_type = b["master_port_type"].get<std::string>();
  if(b.contains("slave_port_type") && b["slave_port_type"].is_string())
    e.link_slave_type = b["slave_port_type"].get<std::string>();
  if(b.contains("protocol") && b["protocol"].is_string())
    e.protocol = b["protocol"].get<std::string>();

  if(e.protocol == "tilelink" && b.contains("tilelink")) {
    const json &t = b["tilelink"];
    if(t.contains("conformance") && t["conformance"].is_string())
      e.conformance = t["conformance"].get<std::string>();
    if(t.contains("data_bus_bytes") &&
       t["data_bus_bytes"].is_number_integer()) {
      e.width_bytes = t["data_bus_bytes"].get<int>();
      e.width_known = true;
    }
  } else if(e.protocol == "custom" && b.contains("custom")) {
    const json &c = b["custom"];
    if(c.contains("read_width_bits") &&
       c["read_width_bits"].is_number_integer()) {
      int bits = c["read_width_bits"].get<int>();
      if(bits % 8 == 0) {
        e.width_bytes = bits / 8;
        e.width_known = true;
      }
    }
  }
}

// --------------------------------------------------------------------
void Resolver::bind_edge(Model &m, Model::Edge &e)
{
  bind_edge_end(m, e, true);
  bind_edge_end(m, e, false);
  bind_edge_link(m, e);
}

// --------------------------------------------------------------------
void Resolver::resolve(const std::vector<Loader::File> &files, Model &m)
{
  m.has_addressing  = syms_.has_addressing();
  m.pa_bits         = syms_.pa_bits();
  m.has_va_bits     = syms_.has_va_bits();
  m.va_bits         = syms_.va_bits();
  m.has_page_bytes  = syms_.has_page_bytes();
  m.page_bytes      = syms_.page_bytes();

  for(const Loader::File &f : files) {
    if(f.declared == "system" && f.doc.contains("name") &&
       f.doc["name"].is_string()) {
      m.system_name = f.doc["name"].get<std::string>();
      break;
    }
  }

  bind_nodes(m);
  bind_interfaces();
  bind_link_ports();
  collect_edges(files, m);

  for(Model::Edge &e : m.edges) bind_edge(m, e);
}

} // namespace cgen
