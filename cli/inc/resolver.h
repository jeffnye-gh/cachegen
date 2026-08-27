// --------------------------------------------------------------------
// FILE:    resolver.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// Binds every reference against the symbol table and builds the
// Model. Every name that does not bind is a diagnostic naming the
// referencing site, T-1, see R-6.
//
// A node definition carries interfaces, an interface carries one link
// and a port map, so an edge end resolves in three steps: node, then
// interface, then port.
// --------------------------------------------------------------------
#pragma once
#include "diag_list.h"
#include "loader.h"
#include "model.h"
#include "symbol_table.h"

namespace cgen
{

class Resolver
{
public:
  Resolver(DiagList &diags, const SymbolTable &syms)
    : diags_(diags), syms_(syms) {}

  void resolve(const std::vector<Loader::File> &files, Model &m);

  // one interface on a node definition, null when it is not there
  static const nlohmann::json *iface_of(const nlohmann::json *body,
                                        const std::string &iface);

  // port type carried by one port of one interface, empty when the
  // node, the interface or the port is not there
  static std::string port_type_of(const nlohmann::json *body,
                                  const std::string &iface,
                                  const std::string &port);

  // link named by one interface, empty when it is not there
  static std::string link_of(const nlohmann::json *body,
                             const std::string &iface);

private:
  void bind_nodes(Model &m);
  void bind_interfaces();
  void bind_link_ports();
  void collect_edges(const std::vector<Loader::File> &files, Model &m);
  void bind_edge(Model &m, Model::Edge &e);
  void bind_edge_end(Model &m, Model::Edge &e, bool from_end);
  void bind_edge_link(Model &m, Model::Edge &e);

  DiagList          &diags_;
  const SymbolTable &syms_;
};

} // namespace cgen
