// --------------------------------------------------------------------
// FILE:    report.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// The check report: the node list with its derived geometry, the edge
// list with resolved link types, then every diagnostic. See R-9.
// --------------------------------------------------------------------
#pragma once
#include "diag_list.h"
#include "model.h"
#include <string>

namespace cgen
{

class Report
{
public:
  void nodes(const Model &m) const;
  void edges(const Model &m) const;
  void ports(const Model &m) const;
  void diagnostics(const DiagList &d) const;
  void summary(const Model &m, const DiagList &d) const;

  void check(const Model &m, const DiagList &d) const;

  static std::string hex(uint64_t v, int bits);
  static std::string field(const char *name,
                           const Model::Field &f,
                           int addr_bits);
};

} // namespace cgen
