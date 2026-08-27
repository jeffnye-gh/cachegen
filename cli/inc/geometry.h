// --------------------------------------------------------------------
// FILE:    geometry.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// The cross field arithmetic, T-8, and the derivation of R-8. Every
// value here is computed from the input, never read from it, D-37.
// --------------------------------------------------------------------
#pragma once
#include "diag_list.h"
#include "model.h"

namespace cgen
{

class Geometry
{
public:
  explicit Geometry(DiagList &diags) : diags_(diags) {}

  // derive and check every node that carries a geometry block
  void compute(Model &m);

  static bool is_pow2(uint64_t v);
  static int  log2_exact(uint64_t v);      // -1 when v is not a power of 2

  static Model::Field make_field(int lsb, int bits);

private:
  void one(Model &m, Model::Node &n);
  void refill(Model &m, Model::Node &n);
  void bank_field(Model::Node &n, const nlohmann::json &g);

  DiagList &diags_;
};

} // namespace cgen
