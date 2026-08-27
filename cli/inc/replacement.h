// --------------------------------------------------------------------
// FILE:    replacement.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The replacement policy, computed here and emitted as package text.
// D-39 names this case directly: the PLRU update and victim tables
// are built once by the tool and emitted to every consumer, rather
// than written out four times and checked for agreement.
//
// Every consumer, the cache control, the testbench and the self
// checking tests, calls the package function. There is one encoding
// and nothing to keep in step.
// --------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>

namespace cgen
{

class Replacement
{
public:
  // ways must be a power of two and at least one
  Replacement(const std::string &policy, int ways);

  const std::string &policy() const { return policy_; }

  // width in bits of the per set replacement state
  int state_bits() const { return state_bits_; }

  // the state a set holds out of reset, as an SV literal
  std::string reset_value() const;

  // package body: the constants, the tables and the two functions
  std::vector<std::string> package_text() const;

  // true when the policy is one the emitter can build
  bool ok() const { return ok_; }
  const std::string &why() const { return why_; }

  // true when the tree PLRU tables were emitted rather than the
  // expression walk. See the note in package_text.
  bool tabled() const { return tabled_; }

  // the victim for one PLRU state, the walk the tables come from
  int  plru_victim(unsigned state) const;
  int  plru_update(unsigned state, int way) const;

  static int  log2i(int v);

  // above this many states the tables are not emitted, because the
  // update table is states * ways entries and that stops being a
  // readable artifact
  static const int MaxTableStates = 256;

private:
  void tree_plru(std::vector<std::string> &o) const;
  void lru(std::vector<std::string> &o) const;
  void counter(std::vector<std::string> &o) const;

  std::string policy_;
  int  ways_{1};
  int  way_bits_{0};
  int  state_bits_{0};
  bool ok_{true};
  bool tabled_{false};
  std::string why_;
};

} // namespace cgen
