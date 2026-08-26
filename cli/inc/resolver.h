// --------------------------------------------------------------------
// FILE:    resolver.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// Binds every reference against the symbol table and builds the
// Model. Every name that does not bind is a diagnostic naming the
// referencing site, T-1, see R-6.
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

  // port type carried by one port instance on a cache definition,
  // empty when the cache or the port is not there
  static std::string port_type_of(const nlohmann::json *cache_body,
                                  const std::string &port);

private:
  void bind_nodes(Model &m);
  void bind_cache_ports();
  void bind_link_ports();
  void collect_edges(const std::vector<Loader::File> &files, Model &m);
  void bind_edge(Model &m, Model::Edge &e);

  DiagList          &diags_;
  const SymbolTable &syms_;
};

} // namespace cgen
