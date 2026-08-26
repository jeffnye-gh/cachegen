// --------------------------------------------------------------------
// FILE:    checker.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// T-3 port type compatibility, T-4 port role against edge direction,
// T-5 graph termination, T-6 field group completeness and T-7 port
// occupancy. T-8 lives in Geometry. See R-7.
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

  void group_of(const SymbolTable::Entry &c,
                const char *group,
                const std::vector<std::string> &want);

  // role of a port type, empty when the type is not defined
  std::string role_of(const std::string &port_type) const;

  void visit(Model &m,
             const std::string &n,
             std::vector<std::string> &path,
             std::map<std::string, int> &state,
             std::set<std::string> &reported);

  DiagList          &diags_;
  const SymbolTable &syms_;
};

} // namespace cgen
