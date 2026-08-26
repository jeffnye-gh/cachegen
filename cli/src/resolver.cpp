// --------------------------------------------------------------------
// FILE:    resolver.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "resolver.h"
#include "msg.h"

using nlohmann::json;
using Kind = cgen::SymbolTable::Kind;

namespace cgen
{

// --------------------------------------------------------------------
std::string Resolver::port_type_of(const json *body,
                                   const std::string &port)
{
  if(body == nullptr || !body->is_object())         return "";
  if(!body->contains("ports"))                      return "";
  const json &p = (*body)["ports"];
  if(!p.is_object() || !p.contains(port))           return "";
  if(!p[port].is_string())                          return "";
  return p[port].get<std::string>();
}

// --------------------------------------------------------------------
// Every topology node names a cache definition, D-35.
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
      if(c->body->contains("cache_type") &&
         (*c->body)["cache_type"].is_string()) {
        n.cache_type = (*c->body)["cache_type"].get<std::string>();
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
// Every port instance on every cache definition names a port type.
// --------------------------------------------------------------------
void Resolver::bind_cache_ports()
{
  for(const auto &kv : syms_.of(Kind::Cache)) {
    const SymbolTable::Entry &c = kv.second;
    if(c.body == nullptr || !c.body->is_object())  continue;
    if(!c.body->contains("ports"))                 continue;

    const json &ports = (*c.body)["ports"];
    if(!ports.is_object()) continue;

    for(auto it = ports.begin(); it != ports.end(); ++it) {
      if(!it.value().is_string()) continue;
      std::string pt = it.value().get<std::string>();
      if(syms_.find(Kind::PortType, pt) != nullptr) continue;

      diags_.error(c.file, c.path + "/ports/" + it.key(),
                   "T-1.port_type",
                   "cache " + msg->tq(c.name) + " port " +
                   msg->tq(it.key()) + " names port type " +
                   msg->tq(pt) + " which is not defined");
    }
  }
}

// --------------------------------------------------------------------
// Both ends of every link definition name a port type.
// --------------------------------------------------------------------
void Resolver::bind_link_ports()
{
  const char *ends[2] = { "from_port_type", "to_port_type" };

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

      if(j.contains("name")      && j["name"].is_string())
        e.name = j["name"].get<std::string>();
      if(j.contains("from")      && j["from"].is_string())
        e.from = j["from"].get<std::string>();
      if(j.contains("to")        && j["to"].is_string())
        e.to = j["to"].get<std::string>();
      if(j.contains("link")      && j["link"].is_string())
        e.link = j["link"].get<std::string>();
      if(j.contains("from_port") && j["from_port"].is_string())
        e.from_port = j["from_port"].get<std::string>();
      if(j.contains("to_port")   && j["to_port"].is_string())
        e.to_port = j["to_port"].get<std::string>();

      m.edges.push_back(e);
    }
  }
}

// --------------------------------------------------------------------
// Endpoints resolve against nodes, the type against links, and the
// named port instances against the cache definitions behind the nodes.
// --------------------------------------------------------------------
void Resolver::bind_edge(Model &m, Model::Edge &e)
{
  const char *ends[2]  = { "from", "to" };
  std::string names[2] = { e.from, e.to };

  for(int i = 0; i < 2; ++i) {
    bool ok = false;
    if(names[i].empty()) {
      diags_.error(e.file, e.path + "/" + ends[i], "T-1.edge_endpoint",
                   "edge " + msg->tq(m.label(e)) + " has no " +
                   ends[i] + " endpoint");
    } else if(syms_.find(Kind::Node, names[i]) == nullptr) {
      diags_.error(e.file, e.path + "/" + ends[i], "T-1.edge_endpoint",
                   "edge " + msg->tq(m.label(e)) + " " + ends[i] +
                   " endpoint " + msg->tq(names[i]) +
                   " is not a topology node");
    } else {
      ok = true;
    }
    if(i == 0) e.from_ok = ok; else e.to_ok = ok;
  }

  if(e.link.empty()) {
    diags_.error(e.file, e.path + "/link", "T-1.edge_link",
                 "edge " + msg->tq(m.label(e)) +
                 " does not name a link definition");
  } else {
    const SymbolTable::Entry *l = syms_.find(Kind::Link, e.link);
    if(l == nullptr) {
      diags_.error(e.file, e.path + "/link", "T-1.edge_link",
                   "edge " + msg->tq(m.label(e)) + " names link " +
                   msg->tq(e.link) + " which is not defined");
    } else {
      e.link_ok = true;
      const json &b = *l->body;
      if(b.contains("from_port_type") && b["from_port_type"].is_string())
        e.link_from_type = b["from_port_type"].get<std::string>();
      if(b.contains("to_port_type") && b["to_port_type"].is_string())
        e.link_to_type = b["to_port_type"].get<std::string>();
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
  }

  // the named port instance has to exist on the cache behind the node
  struct End { bool ok; const std::string *port; std::string *type;
               const std::string *node; const char *key; };
  End list[2] = {
    { e.from_ok, &e.from_port, &e.from_port_type, &e.from, "from_port" },
    { e.to_ok,   &e.to_port,   &e.to_port_type,   &e.to,   "to_port"   }
  };

  for(const End &en : list) {
    if(!en.ok) continue;

    if(en.port->empty()) {
      // D-30 says the edge names the port instance, the schema leaves
      // it optional. Treated as an error, recorded as a schema gap.
      diags_.error(e.file, e.path, "T-1.edge_port",
                   "edge " + msg->tq(m.label(e)) + " does not name a " +
                   en.key + " on node " + msg->tq(*en.node));
      continue;
    }

    const Model::Node *n = m.node(*en.node);
    if(n == nullptr || !n->resolved) continue;

    std::string pt = port_type_of(n->body, *en.port);
    if(pt.empty()) {
      diags_.error(e.file, e.path + "/" + en.key, "T-1.edge_port",
                   "edge " + msg->tq(m.label(e)) + " names port " +
                   msg->tq(*en.port) + " which node " +
                   msg->tq(*en.node) + " does not have, its cache is " +
                   msg->tq(n->cache));
      continue;
    }
    *en.type = pt;
  }
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
  bind_cache_ports();
  bind_link_ports();
  collect_edges(files, m);

  for(Model::Edge &e : m.edges) bind_edge(m, e);
}

} // namespace cgen
