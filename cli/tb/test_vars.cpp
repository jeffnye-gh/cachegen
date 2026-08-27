// --------------------------------------------------------------------
// FILE:    test_vars.cpp
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
//
// R-3 and R-4. The tool variable set of the emitted build.
//
// The assertion that matters is NoEmittedMakefileCarriesABareToolName.
// It ENUMERATES ToolVars::all(), which is the same table the emitted
// Vars.mk is generated from, so a tool added to that table is covered
// here without an edit in this file. That is the CLI-003 rule applied
// one level out: a list of tool names written here would agree with
// the tool on the day it was written and stop agreeing silently.
//
// A bare name is looked for in RECIPE lines only. A comment that says
// "make lint" is documentation and is not an invocation.
// --------------------------------------------------------------------
#include "diag_codes.h"
#include "fixture.h"
#include "gen_log.h"
#include "tool_vars.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <string>
#include <vector>

using cgen::Fixture;
using cgen::ToolVars;

namespace {

bool has(const std::vector<std::string> &v, const std::string &s)
{
  return std::find(v.begin(), v.end(), s) != v.end();
}

// ------------------------------------------------------------------
// Every command word of one Make recipe line. A recipe line begins
// with a tab. The leading @ - and + of a recipe, the shell separators
// and the line continuation are stripped, so 'rm' is found in
// "\t-rm -rf x" and "$(RM)" is not found anywhere.
// ------------------------------------------------------------------
std::vector<std::string> recipe_words(const std::string &line)
{
  std::vector<std::string> out;
  if(line.empty() || line[0] != '\t') return out;

  std::string tok;
  for(size_t i = 1; i <= line.size(); ++i) {
    const char c = i < line.size() ? line[i] : ' ';
    if(c == ' ' || c == '\t' || c == ';' || c == '|' || c == '&') {
      if(!tok.empty()) out.push_back(tok);
      tok.clear();
      continue;
    }
    if(c == '#') break;                 // the rest is a comment
    tok += c;
  }

  for(std::string &w : out) {
    size_t b = 0;
    while(b < w.size() && (w[b] == '@' || w[b] == '-' || w[b] == '+')) {
      ++b;
    }
    w = w.substr(b);
    while(!w.empty() && w.back() == '\\') w.pop_back();
  }
  return out;
}

std::vector<std::string> lines_of(const std::string &body)
{
  std::vector<std::string> out;
  size_t at = 0;
  while(at <= body.size()) {
    const size_t nl = body.find('\n', at);
    const size_t end = nl == std::string::npos ? body.size() : nl;
    out.push_back(body.substr(at, end - at));
    if(nl == std::string::npos) break;
    at = nl + 1;
  }
  return out;
}

bool is_makefile(const std::string &path)
{
  return path.find("Makefile") != std::string::npos;
}

} // namespace

// --------------------------------------------------------------------
// R-4. No emitted Makefile invokes a tool by its bare name.
// --------------------------------------------------------------------
TEST(Vars, NoEmittedMakefileCarriesABareToolName)
{
  const std::string out = Fixture::scratch("vars_bare");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error())
    << Fixture::codes(drv->diags());

  const std::vector<std::string> files = Fixture::tree(out);
  ASSERT_FALSE(files.empty());

  int makefiles = 0;
  for(const std::string &f : files) {
    if(!is_makefile(f)) continue;
    ++makefiles;

    for(const std::string &line : lines_of(Fixture::slurp(out + "/" + f))) {
      for(const std::string &w : recipe_words(line)) {
        for(const ToolVars::Tool &t : ToolVars::all()) {
          EXPECT_NE(std::string(t.name), w)
            << f << " invokes " << t.name << " by its bare name: "
            << line << "\nUse $(" << t.var << "), which " << f
            << " gets from " << ToolVars::file_name()
            << ". A bare name runs whatever is on PATH.";
        }
      }
    }
  }

  EXPECT_LE(7, makefiles) << "only " << makefiles
                          << " Makefiles were emitted";
}

// --------------------------------------------------------------------
// R-4, the other half. A Makefile that never says 'verilator' and
// never includes Vars.mk would pass the assertion above and build
// nothing. Every emitted Makefile has to include the one copy.
// --------------------------------------------------------------------
TEST(Vars, EveryEmittedMakefileIncludesVarsMk)
{
  const std::string out = Fixture::scratch("vars_include");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::vector<std::string> files = Fixture::tree(out);
  ASSERT_TRUE(has(files, ToolVars::file_name()))
    << "no " << ToolVars::file_name() << " at the output root";

  for(const std::string &f : files) {
    if(!is_makefile(f)) continue;
    EXPECT_NE(std::string::npos,
              Fixture::slurp(out + "/" + f).find(
                  ToolVars::include_line()))
      << f << " does not include " << ToolVars::file_name();
  }
}

// --------------------------------------------------------------------
// R-3. Every tool the emitted build uses gets a variable, even where
// the value resolves to the system copy. The master copy assigns some
// of them and the generated block covers the rest, so the test asks
// the emitted file rather than either half.
//
// MAKE is the one exception and it is deliberate: make defines it and
// its own value is better than one cgen could write. The Tool entry
// says so with assign false, so the exception is in the table and not
// in this test.
// --------------------------------------------------------------------
TEST(Vars, EveryToolVariableIsDefinedInTheEmittedVarsMk)
{
  const std::string out = Fixture::scratch("vars_defined");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::string body =
      Fixture::slurp(out + "/" + ToolVars::file_name());
  ASSERT_FALSE(body.empty());

  for(const ToolVars::Tool &t : ToolVars::all()) {
    if(!t.assign) continue;

    bool found = false;
    for(const std::string &line : lines_of(body)) {
      if(line.empty() || line[0] == '#') continue;
      const size_t eq = line.find('=');
      if(eq == std::string::npos) continue;

      std::string name = line.substr(0, eq);
      while(!name.empty() &&
            (name.back() == ' ' || name.back() == '\t' ||
             name.back() == '?' || name.back() == ':' ||
             name.back() == '+')) {
        name.pop_back();
      }
      size_t b = 0;
      while(b < name.size() && (name[b] == ' ' || name[b] == '\t')) ++b;
      if(name.substr(b) == t.var) { found = true; break; }
    }

    EXPECT_TRUE(found) << t.var << " is a tool the emitted build uses "
                       << "and " << ToolVars::file_name()
                       << " assigns it nothing";
  }
}

// --------------------------------------------------------------------
// R-3. The master copy reaches the emitted file verbatim. It is read
// and copied, never rewritten, so its CGEN_ROOT guard is still there.
// --------------------------------------------------------------------
TEST(Vars, TheMasterCopyIsCopiedVerbatim)
{
  const std::string out = Fixture::scratch("vars_master");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::string master = Fixture::slurp(Fixture::vars_master());
  ASSERT_FALSE(master.empty()) << "the master copy could not be read";

  const std::string body =
      Fixture::slurp(out + "/" + ToolVars::file_name());

  // every non blank line of the master, in the emitted file
  size_t at = 0;
  int checked = 0;
  while(at < master.size()) {
    const size_t nl = master.find('\n', at);
    const size_t end = nl == std::string::npos ? master.size() : nl;
    const std::string line = master.substr(at, end - at);
    if(!line.empty()) {
      EXPECT_NE(std::string::npos, body.find(line))
        << "the master copy line is not in the emitted Vars.mk: "
        << line;
      ++checked;
    }
    if(nl == std::string::npos) break;
    at = nl + 1;
  }
  EXPECT_LE(3, checked) << "the master copy looks empty";
}

// --------------------------------------------------------------------
// R-3. CGEN_ROOT IS SPECIAL. A tool path inside the tree is written
// back in the $(CGEN_ROOT)/... form so the emitted tree stays
// portable. A path outside it is written verbatim, and the tree is
// machine specific by the user's choice.
// --------------------------------------------------------------------
TEST(Vars, APathInsideCgenRootIsWrittenRelativeToIt)
{
  const std::string root = ToolVars::cgen_root();
  if(root.empty()) {
    GTEST_SKIP() << "CGEN_ROOT is not set, the rewrite has no anchor";
  }

  const std::string out = Fixture::scratch("vars_inside");
  auto drv = Fixture::run_emit_vars(
      Fixture::pacino(), out, "",
      { "VERILATOR=" + root + "/tools/bin/verilator" });
  ASSERT_FALSE(drv->diags().has_error())
    << Fixture::codes(drv->diags());

  const std::string body =
      Fixture::slurp(out + "/" + ToolVars::file_name());

  EXPECT_NE(std::string::npos,
            body.find("VERILATOR = $(CGEN_ROOT)/tools/bin/verilator"))
    << "the path was not rewritten into the $(CGEN_ROOT) form";
  EXPECT_EQ(std::string::npos, body.find(root + "/tools"))
    << "the expansion of CGEN_ROOT reached the emitted file";
}

// --------------------------------------------------------------------
TEST(Vars, APathOutsideCgenRootIsWrittenVerbatim)
{
  const std::string out = Fixture::scratch("vars_outside");
  const std::string path = "/opt/verilator/bin/verilator";

  auto drv = Fixture::run_emit_vars(Fixture::pacino(), out, "",
                                    { "VERILATOR=" + path });
  ASSERT_FALSE(drv->diags().has_error())
    << Fixture::codes(drv->diags());

  const std::string body =
      Fixture::slurp(out + "/" + ToolVars::file_name());
  EXPECT_NE(std::string::npos, body.find("VERILATOR = " + path))
    << "the path outside the tree was not written as given";
  EXPECT_NE(std::string::npos, body.find("MACHINE SPECIFIC"))
    << "the emitted file does not say the tree is machine specific";
}

// --------------------------------------------------------------------
// R-4. Vars.mk is the ONE emitted file whose contents may vary with
// the command line, and R-11 stands for every other one. Two runs
// that differ only in --tool therefore differ in Vars.mk and in the
// emission log's tool block, and NOWHERE ELSE.
//
// That is the adjustment R-4 asks about: the determinism test used to
// assert that two runs of one configuration agree everywhere, which
// is still asserted by Emit.TwoEmissionsAreIdentical for one command
// line. This holds the second half, that the command line reaches
// exactly the two files that are allowed to see it.
// --------------------------------------------------------------------
TEST(Vars, ToolPathsReachVarsMkAndTheLogAndNothingElse)
{
  const std::string a = Fixture::scratch("tool_a");
  const std::string b = Fixture::scratch("tool_b");

  auto d1 = Fixture::run_emit_vars(Fixture::pacino(), a, "", {});
  auto d2 = Fixture::run_emit_vars(Fixture::pacino(), b, "",
                                   { "VERILATOR=/opt/vl/bin/verilator",
                                     "RM=/bin/rm" });
  ASSERT_FALSE(d1->diags().has_error());
  ASSERT_FALSE(d2->diags().has_error());

  const std::vector<std::string> fa = Fixture::tree(a);
  ASSERT_EQ(fa, Fixture::tree(b)) << "the file sets differ";

  const std::string log = std::string(cgen::GenLog::dir()) + "/" +
                          cgen::GenLog::emission_name();

  int differ = 0;
  for(const std::string &f : fa) {
    const std::string x = Fixture::slurp(a + "/" + f);
    const std::string y = Fixture::slurp(b + "/" + f);

    if(f == ToolVars::file_name() || f == log) {
      if(x != y) ++differ;
      continue;
    }
    EXPECT_EQ(x, y) << f << " changed with the tool paths, and only "
                    << ToolVars::file_name() << " and " << log
                    << " are allowed to";
  }

  EXPECT_EQ(2, differ)
    << ToolVars::file_name() << " and " << log
    << " did not both record the tool paths they were given";
}

// --------------------------------------------------------------------
// R-3. A master Vars.mk that cannot be read stops emission with a
// diagnostic rather than emitting a tree whose every Makefile
// includes a file that is not there.
// --------------------------------------------------------------------
TEST(Vars, AnUnreadableMasterCopyIsReportedAndNothingIsWritten)
{
  const std::string out = Fixture::scratch("vars_missing");

  auto drv = Fixture::run_emit_vars(
      Fixture::pacino(), out,
      Fixture::fixture_dir() + "/no_such_Vars.mk");

  EXPECT_EQ(size_t(1), drv->diags().count_code(cgen::code::emit_vars))
    << "codes: " << Fixture::codes(drv->diags());
  EXPECT_TRUE(drv->emitted().empty())
    << drv->emitted().size() << " files were written anyway";
}

// --------------------------------------------------------------------
// A --tool naming something that is not a tool of the emitted build
// is a mistake worth reporting. Accepting it would write a variable
// no recipe reads.
// --------------------------------------------------------------------
TEST(Vars, AnUnknownToolVariableIsRefused)
{
  const std::string out = Fixture::scratch("vars_unknown");

  auto drv = Fixture::run_emit_vars(Fixture::pacino(), out, "",
                                    { "GCC=/usr/bin/gcc" });

  EXPECT_EQ(size_t(1), drv->diags().count_code(cgen::code::emit_vars))
    << "codes: " << Fixture::codes(drv->diags());
  EXPECT_TRUE(drv->emitted().empty());
}
