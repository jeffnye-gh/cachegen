// --------------------------------------------------------------------
// FILE:    feature_table.h
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
//
// R-8. THE FEATURE TABLE.
//
// "Seven checks is not coverage." The list of features to test is
// DERIVED FROM THE CONFIGURATION, node by node and field by field,
// not from a general idea of what a cache does. Every leaf a node's
// definition carries is a feature, and every one of them gets either
// a test or a statement of why it cannot have one.
//
// The three columns are all derived and none is maintained:
//
//   the feature   every leaf under a node's cache definition and
//                 under the link definitions its interfaces name,
//                 enumerated by FieldUse
//   the test      registered by the emitter AT THE POINT IT EMITS
//                 THE TEST, through cov_test below, so a test that
//                 stops being emitted stops claiming coverage
//   the reason    when no test covers it. The first reason comes
//                 straight from R-6b: A FEATURE NO STAGE CONSUMES
//                 CANNOT BE TESTED, because there is nothing in the
//                 emitted design that changing it would move.
//
// A hand written table would agree with the emitter on the day it
// was written. This one cannot disagree with it at all.
// --------------------------------------------------------------------
#pragma once
#include "field_use.h"
#include "loader.h"
#include "model.h"
#include <string>
#include <vector>

namespace cgen
{

class Features
{
public:
  // where a covering test lives
  enum class Level { Unit, Top };

  struct Cover {
    Level       level;
    std::string bench;    // l1d_tb, pacino_tb
    std::string test;     // the check's own name
  };

  struct Feature {
    std::string node;     // the topology instance, or the link name
    std::string ptr;      // JSON pointer of the field
    std::string value;    // what the configuration declares, rendered
    bool        consumed{false};
    std::vector<Cover> covers;
    std::string why;      // set when covers is empty
  };

  // Build the table. Call after emission, so every test that was
  // emitted has already registered what it covers.
  void build(const Model &m, const FieldUse &u,
             const std::vector<Loader::File> &files);

  const std::vector<Feature> &all() const { return feats_; }

  int tested()   const;
  int untested() const;

  // ------------------------------------------------------------------
  // The recorder, installed for the life of one emission. Same shape
  // as FieldUse::Scope and for the same reason: a suite running many
  // emitters must not let one run's registrations reach another's
  // table.
  // ------------------------------------------------------------------
  class Scope
  {
  public:
    explicit Scope(Features &f);
    ~Scope();
  private:
    Features *prev_;
  };

  static Features *active();

  void record(const std::string &node, const std::string &rel,
              Level level, const std::string &bench,
              const std::string &test);

private:
  struct Reg {
    std::string node;
    std::string rel;      // relative to the node's cache definition
    Cover       cover;
  };

  std::vector<Reg>     regs_;
  std::vector<Feature> feats_;
};

// --------------------------------------------------------------------
// One registration. node is the topology instance the test drives,
// rel is the field it exercises relative to that node's cache
// definition, "/policies/write_hit". A no-op when no recorder is
// installed.
// --------------------------------------------------------------------
void cov_test(const std::string &node, const std::string &rel,
              Features::Level level, const std::string &bench,
              const std::string &test);

} // namespace cgen
