// --------------------------------------------------------------------
// FILE:    checker.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// T-3 port type compatibility, T-4 port role against edge direction,
// T-5 graph termination, T-6 field group completeness, T-7 port
// occupancy, T-9 link agreement and T-10 address width agreement.
// T-8 lives in Geometry. See R-7.
//
// T-9 exists because the link is carried by the interface at each end
// of an edge rather than by the edge itself, so the two ends can name
// different links and that has to be caught.
//
// T-10 exists because a link address width and the system pa_bits are
// two independent integers in two different documents. A schema
// constraint cannot see across documents, so the agreement is a
// checker rule. What goes wrong without it is silent: the generated
// address type is pa_bits wide, so a narrower link zero extends into
// it on the way in and truncates out of it on the way out, and the
// tree still builds.
//
// The group membership table used by T-6 is stated in checker.cpp.
// The schemas do not enumerate which fields make a group complete,
// so that table is tool policy, recorded as a schema gap.
// --------------------------------------------------------------------
#pragma once
#include "diag_list.h"
#include "model.h"
#include "symbol_table.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace cgen
{

class Checker
{
public:
  Checker(DiagList &diags, const SymbolTable &syms)
    : diags_(diags), syms_(syms) {}

  void run(Model &m);

private:
  void port_types(Model &m);        // T-3
  void port_roles(Model &m);        // T-4
  void graph(Model &m);             // T-5
  void groups();                    // T-6
  void occupancy(Model &m);         // T-7
  void link_agree(Model &m);        // T-9
  void addr_width(Model &m);        // T-10

  void group_of(const SymbolTable::Entry &c,
                const char *group,
                const std::vector<std::string> &want);

  // role of a port type, empty when the type is not defined
  std::string role_of(const std::string &port_type) const;

  // T-10. The address width a link declares, and the JSON pointer
  // it was read from. Returns false when the link carries no width.
  bool link_addr_bits(const std::string &link,
                      int &bits,
                      std::string &file,
                      std::string &ptr) const;

  // T-10. True for a node whose address decomposition is pa_bits
  // wide, which is every cache and the memory. An agent and an
  // interconnect carry interfaces and no geometry.
  bool is_addressed(const Model &m, const std::string &node) const;

  void visit(Model &m,
             const std::string &n,
             std::vector<std::string> &path,
             std::map<std::string, int> &state,
             std::set<std::string> &reported);

  DiagList          &diags_;
  const SymbolTable &syms_;
};

} // namespace cgen
