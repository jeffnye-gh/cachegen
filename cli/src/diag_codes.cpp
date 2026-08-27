// --------------------------------------------------------------------
// FILE:    diag_codes.cpp
// SOURCE:  CLI-003
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "diag_codes.h"

namespace cgen
{

// --------------------------------------------------------------------
// The table and the constants in the header come from one macro, so
// there is no second list to keep in step.
// --------------------------------------------------------------------
const std::vector<DiagCodes::Entry> &DiagCodes::all()
{
  using R = DiagCodes::Reach;

  static const std::vector<Entry> table = {
#define CGEN_DIAG_CODE_ROW(id, str, reach, note) { str, R::reach, note },
    CGEN_DIAG_CODES(CGEN_DIAG_CODE_ROW)
#undef CGEN_DIAG_CODE_ROW
  };

  return table;
}

// --------------------------------------------------------------------
const DiagCodes::Entry *DiagCodes::find(const std::string &c)
{
  for(const Entry &e : all()) {
    if(c == e.code) return &e;
  }
  return nullptr;
}

// --------------------------------------------------------------------
std::vector<std::string> DiagCodes::of_reach(Reach r)
{
  std::vector<std::string> out;
  for(const Entry &e : all()) {
    if(e.reach == r) out.push_back(e.code);
  }
  return out;
}

// --------------------------------------------------------------------
std::string DiagCodes::reach_text(Reach r)
{
  switch(r) {
    case Reach::Fixture: return "Fixture";
    case Reach::Guard:   return "Guard";
    case Reach::Env:     return "Env";
  }
  return "unknown";
}

} // namespace cgen
