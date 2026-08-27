// --------------------------------------------------------------------
// FILE:    field_use.h
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
//
// R-6b. THE UNCONSUMED FIELD REPORT.
//
// A field a user can edit, re-emit, and see no difference from is
// indistinguishable from a field that works. The report says which
// fields those are, and it is DERIVED FROM WHAT THE TOOL ACTUALLY
// READ rather than from a maintained list of known-inert fields. A
// hand written list is the defect this exists to remove: it agrees
// with the tool on the day it is written and silently stops agreeing
// afterwards.
//
// The mechanism has two halves.
//
//   enumerate()  walks every loaded document once and records every
//                leaf, by file and JSON pointer. That is every field
//                the configuration carries.
//
//   cfg_read()   is called at the point a stage EXTRACTS a value.
//                Marking a container marks everything below it.
//
// What is left is the report. A stage that starts consuming a field
// calls cfg_read at the same line it starts reading it, so the field
// leaves the report in the same change that makes it live. Nothing
// has to be remembered.
//
// PRESENCE IS NOT A READ. The checker's T-6 group completeness asks
// whether a field is there and never looks at its value, so it does
// not mark. A group whose members are all inert is exactly the case
// the report exists to surface, and a presence check must not hide
// it.
//
// The recorder is installed for the life of one Driver::run through
// FieldUse::Scope, so a suite running many drivers cannot let one
// run's reads reach another's report.
// --------------------------------------------------------------------
#pragma once
#include "loader.h"
#include <string>
#include <vector>

namespace cgen
{

class FieldUse
{
public:
  struct Leaf {
    std::string file;   // the configuration file, as diagnostics name it
    std::string ptr;    // JSON pointer, /caches/l1i/inclusion
  };

  // every leaf of every loaded document, once
  void enumerate(const std::vector<Loader::File> &files);

  // a stage read the value at this pointer. A container marks its
  // whole subtree.
  void read(const std::string &file, const std::string &ptr);

  bool was_read(const std::string &file, const std::string &ptr) const;

  const std::vector<Leaf> &leaves() const { return leaves_; }

  // every leaf no stage read, in file then pointer order
  std::vector<Leaf> unread() const;

  // ------------------------------------------------------------------
  // The active recorder, installed for one Driver::run. Follows the
  // singleton idiom the tool already uses for Opt and Msg, scoped so
  // it cannot outlive the run it belongs to.
  // ------------------------------------------------------------------
  class Scope
  {
  public:
    explicit Scope(FieldUse &u);
    ~Scope();
  private:
    FieldUse *prev_;
  };

  static FieldUse *active();

private:
  std::vector<Leaf>        leaves_;
  std::vector<std::string> marks_;   // "file\x1f" + pointer prefix
};

// --------------------------------------------------------------------
// One read. A no-op when no recorder is installed, so a stage can call
// it unconditionally and a unit test of that stage needs no recorder.
// --------------------------------------------------------------------
void cfg_read(const std::string &file, const std::string &ptr);

} // namespace cgen
