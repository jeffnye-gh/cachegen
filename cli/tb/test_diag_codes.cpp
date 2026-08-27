// --------------------------------------------------------------------
// FILE:    test_diag_codes.cpp
// SOURCE:  CLI-003
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// Relates the codes the tool can emit to the codes a fixture actually
// exercises. Without this the suite can be green while a check is
// dead, which is what happened to T-3.port_type between CLI-001 and
// CLI-002.
//
// The expected set is read from DiagCodes at run time. A list of
// codes written out here would be the same defect one level up: it
// would go stale the moment a code was added, and nothing would say
// so.
//
// Three of the codes are about the schema directory rather than about
// a configuration, so they are reached by pointing CGEN_SCHEMA_DIR at
// a prepared directory under tb/fixtures instead of by running a
// system file:
//
//   tb/fixtures                    holds no schemas       -> schema.dir
//   tb/fixtures/schemas_bad_parse  one schema is not JSON -> schema.parse
//   tb/fixtures/schemas_bad_build  one schema has an
//                                  unresolvable $ref      -> schema.build
//
// The emit codes need the other command rather than another
// configuration, so every configuration is swept twice, once through
// check and once through emit. See codes_every_fixture_produces.
// --------------------------------------------------------------------
#include "diag_codes.h"
#include "fixture.h"
#include <gtest/gtest.h>
#include <set>
#include <string>

using cgen::DiagCodes;
using cgen::Fixture;

namespace {

// ------------------------------------------------------------------
// Every code any fixture in the tree produces, from one pass over
// every configuration plus the three schema directory runs.
// ------------------------------------------------------------------
std::set<std::string> codes_every_fixture_produces()
{
  std::set<std::string> seen;

  for(const std::string &cfg : Fixture::configs()) {
    auto drv = Fixture::run(cfg);
    for(const cgen::Diag &d : drv->diags().all()) seen.insert(d.code());
  }

  // ----------------------------------------------------------------
  // CLI-004. The emit codes are reached by running the same
  // configurations through --cmd=emit. A code the emitter can
  // produce is invisible to a sweep that only ever runs the check
  // path, which is the CLI-003 defect one command down.
  //
  // The output goes to a scratch directory. A clean configuration
  // does write a tree there and it is thrown away; an erroring one
  // writes nothing, which is R-3.
  // ----------------------------------------------------------------
  const std::string out = Fixture::scratch("sweep");
  for(const std::string &cfg : Fixture::configs()) {
    auto drv = Fixture::run_emit(cfg, out);
    for(const cgen::Diag &d : drv->diags().all()) seen.insert(d.code());
  }

  // ----------------------------------------------------------------
  // CLI-005. emit.vars needs neither another configuration nor
  // another command, it needs a master Vars.mk that is not there.
  // Every configuration in the sweep takes the default master copy
  // and finds it, so the code is invisible to the sweep above. This
  // is the same extension CLI-004 made for the emit codes, one input
  // further out.
  // ----------------------------------------------------------------
  {
    auto drv = Fixture::run_emit_vars(
        Fixture::pacino(), Fixture::scratch("sweep_vars"),
        Fixture::fixture_dir() + "/no_such_Vars.mk");
    for(const cgen::Diag &d : drv->diags().all()) seen.insert(d.code());
  }

  const std::string base = Fixture::fixture_dir() +
                           "/base/base_system.json";
  const std::string dirs[3] = {
    Fixture::fixture_dir(),
    Fixture::fixture_dir() + "/schemas_bad_parse",
    Fixture::fixture_dir() + "/schemas_bad_build"
  };

  for(const std::string &d : dirs) {
    auto drv = Fixture::run_with_schema_dir(base, d);
    for(const cgen::Diag &x : drv->diags().all()) seen.insert(x.code());
  }

  return seen;
}

} // namespace

// --------------------------------------------------------------------
// R-4. Every code the list marks Fixture is produced by at least one
// fixture, and no code the list marks Guard or Env is produced by
// any of them. The second half matters as much as the first: a Guard
// that a fixture reaches is a misclassification, not a pass.
// --------------------------------------------------------------------
TEST(Diagnostics, EveryCodeHasAFixture)
{
  const std::set<std::string> seen = codes_every_fixture_produces();

  ASSERT_FALSE(DiagCodes::all().empty())
    << "the diagnostic code list is empty";

  for(const DiagCodes::Entry &e : DiagCodes::all()) {
    const bool produced = seen.count(e.code) != 0;

    switch(e.reach) {
      case DiagCodes::Reach::Fixture:
        EXPECT_TRUE(produced)
          << e.code << " is marked Fixture and no fixture produces "
          << "it. Add a negative fixture for it, or move it to Guard "
          << "or Env with the reason.";
        break;

      case DiagCodes::Reach::Guard:
      case DiagCodes::Reach::Env:
        EXPECT_FALSE(produced)
          << e.code << " is marked "
          << DiagCodes::reach_text(e.reach)
          << " on the grounds that " << e.note
          << ", but a fixture produced it. The classification is "
          << "wrong, or the tool changed.";
        break;
    }
  }
}

// --------------------------------------------------------------------
// The other direction. A code that reaches a diagnostic without
// appearing in the list would leave the assertion above measuring
// less than it claims to.
// --------------------------------------------------------------------
TEST(Diagnostics, NoCodeIsEmittedOffTheList)
{
  for(const std::string &c : codes_every_fixture_produces()) {
    EXPECT_NE(nullptr, DiagCodes::find(c))
      << c << " was emitted and is not in the diagnostic code list";
  }
}

// --------------------------------------------------------------------
// The sweep is only worth what it covers. If the fixture directory
// stops being found, every code above would look uncovered for a
// reason that has nothing to do with the tool.
// --------------------------------------------------------------------
TEST(Diagnostics, TheFixtureSweepFindsItsConfigurations)
{
  const std::vector<std::string> cfg = Fixture::configs();
  EXPECT_LE(size_t(20), cfg.size()) << "only " << cfg.size() << " found";
}
