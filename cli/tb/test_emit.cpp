// --------------------------------------------------------------------
// FILE:    test_emit.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The emitter, R-12. Three things at minimum: the expected file set
// for pacino, R-11 determinism, and R-3's refusal to emit from a
// configuration that produced an error.
//
// The expected file set is DERIVED FROM THE MODEL at run time, not
// written out here. A list of 78 paths in a test file is the same
// defect CLI-003 removed one level down: it goes stale the moment the
// emitter gains a file, and nothing says so. What is asserted instead
// is the invariant R-4 states, which is what the list was standing in
// for: one directory per topology INSTANCE, one per system, and
// nothing outside them.
// --------------------------------------------------------------------
#include "diag_codes.h"
#include "fixture.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

using cgen::Fixture;

namespace {

// ------------------------------------------------------------------
// True when path starts with prefix.
// ------------------------------------------------------------------
bool starts(const std::string &p, const std::string &prefix)
{
  return p.size() >= prefix.size() &&
         p.compare(0, prefix.size(), prefix) == 0;
}

// ------------------------------------------------------------------
// The first path element, the directory an emitted file landed in.
// ------------------------------------------------------------------
std::string head_dir(const std::string &p)
{
  const size_t k = p.find('/');
  return k == std::string::npos ? std::string() : p.substr(0, k);
}

bool has(const std::vector<std::string> &v, const std::string &s)
{
  return std::find(v.begin(), v.end(), s) != v.end();
}

} // namespace

// --------------------------------------------------------------------
// R-3. The check path runs first and an error stops emission dead.
// Nothing at all is written, not a partial tree.
// --------------------------------------------------------------------
TEST(Emit, RefusesToEmitFromAnErroringConfiguration)
{
  const std::string out = Fixture::scratch("refuse");
  const std::string cfg = Fixture::fixture_dir() +
                          "/neg_dangling_cache/system.json";

  auto drv = Fixture::run_emit(cfg, out);

  EXPECT_TRUE(drv->diags().has_error())
    << "the fixture is meant to produce an error";
  EXPECT_EQ(size_t(1), drv->diags().count_code(cgen::code::emit_refused))
    << "codes: " << Fixture::codes(drv->diags());
  EXPECT_TRUE(drv->emitted().empty())
    << drv->emitted().size() << " files were written anyway";
  EXPECT_TRUE(Fixture::tree(out).empty())
    << "the output directory is not empty";
}

// --------------------------------------------------------------------
// Every negative fixture, not only the one above. A refusal that
// depended on which check failed would be worth knowing about.
// --------------------------------------------------------------------
TEST(Emit, EveryErroringConfigurationIsRefused)
{
  const std::string out = Fixture::scratch("refuse_all");
  int seen = 0;

  for(const std::string &cfg : Fixture::configs()) {
    auto drv = Fixture::run_emit(cfg, out);
    if(!drv->diags().has_error()) continue;
    ++seen;
    EXPECT_TRUE(drv->emitted().empty())
      << cfg << " produced errors and emitted "
      << drv->emitted().size() << " files";
  }

  EXPECT_LE(10, seen) << "only " << seen
                      << " erroring configurations were found";
}

// --------------------------------------------------------------------
// R-12 and R-4. The file set pacino produces.
// --------------------------------------------------------------------
TEST(Emit, ProducesTheExpectedFileSetForPacino)
{
  const std::string out = Fixture::scratch("pacino");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);

  ASSERT_FALSE(drv->diags().has_error())
    << "pacino is clean and must emit: "
    << Fixture::codes(drv->diags());

  const std::vector<std::string> files = Fixture::tree(out);
  ASSERT_FALSE(files.empty()) << "nothing was written";

  // what the driver says it wrote is what is on the disk
  std::vector<std::string> said = drv->emitted();
  std::sort(said.begin(), said.end());
  EXPECT_EQ(said, files)
    << "the emitted list and the output tree disagree";

  // the expected set, derived from the model rather than listed
  const cgen::Model &m = drv->model();
  const std::string sys = m.system_name;
  ASSERT_FALSE(sys.empty());

  std::set<std::string> dirs;
  for(const std::string &f : files) {
    const std::string d = head_dir(f);
    EXPECT_FALSE(d.empty())
      << f << " is not inside a node or a system directory";
    dirs.insert(d);
  }

  // R-4. One directory per topology INSTANCE name, plus the system.
  std::set<std::string> want;
  for(const cgen::Model::Node &n : m.nodes) want.insert(n.name);
  want.insert(sys);
  EXPECT_EQ(want, dirs)
    << "the directories do not match the topology instances";

  // every node carries its module, its package and its build
  for(const cgen::Model::Node &n : m.nodes) {
    EXPECT_TRUE(has(files, n.name + "/rtl/" + n.name + ".sv"))
      << n.name << " has no top module";
    EXPECT_TRUE(has(files, n.name + "/rtl/" + n.name + "_pkg.sv"))
      << n.name << " has no package";
    EXPECT_TRUE(has(files, n.name + "/" + n.name + ".f"))
      << n.name << " has no filelist";
    EXPECT_TRUE(has(files, n.name + "/Makefile"))
      << n.name << " has no Makefile";
    EXPECT_TRUE(has(files, n.name + "/tb/" + n.name + "_tb.sv"))
      << n.name << " has no unit testbench";
    EXPECT_TRUE(has(files, n.name + "/tb/cgen_tb_tasks.svh"))
      << n.name << " has no task set beside its testbench";
  }

  // the system carries its top, its testbench and its build
  EXPECT_TRUE(has(files, sys + "/rtl/" + sys + "_top.sv"));
  EXPECT_TRUE(has(files, sys + "/tb/" + sys + "_tb.sv"));
  EXPECT_TRUE(has(files, sys + "/tb/" + sys + "_tests.svh"));
  EXPECT_TRUE(has(files, sys + "/" + sys + ".f"));
  EXPECT_TRUE(has(files, sys + "/Makefile"));

  // R-5. A node's RTL is one module per file, so no node directory
  // is allowed to hold only a package.
  for(const cgen::Model::Node &n : m.nodes) {
    int rtl = 0;
    for(const std::string &f : files) {
      if(starts(f, n.name + "/rtl/")) ++rtl;
    }
    EXPECT_LE(2, rtl) << n.name << " emitted " << rtl
                      << " rtl files";
  }
}

// --------------------------------------------------------------------
// R-11. Two runs of one configuration produce byte identical files.
//
// The second run is given the config through a DIFFERENT path spelling
// from the first, because an absolute path reaching an emitted file is
// the most likely way for this to break.
// --------------------------------------------------------------------
TEST(Emit, TwoEmissionsAreIdentical)
{
  const std::string a = Fixture::scratch("det_a");
  const std::string b = Fixture::scratch("det_b");

  const std::string cfg  = Fixture::pacino();
  const std::string cfg2 =
      std::filesystem::absolute(cfg).generic_string();

  auto d1 = Fixture::run_emit(cfg,  a);
  auto d2 = Fixture::run_emit(cfg2, b);

  ASSERT_FALSE(d1->diags().has_error());
  ASSERT_FALSE(d2->diags().has_error());

  const std::vector<std::string> fa = Fixture::tree(a);
  const std::vector<std::string> fb = Fixture::tree(b);

  ASSERT_FALSE(fa.empty());
  ASSERT_EQ(fa, fb) << "the two runs wrote different file sets";

  for(const std::string &f : fa) {
    const std::string x = Fixture::slurp(a + "/" + f);
    const std::string y = Fixture::slurp(b + "/" + f);
    EXPECT_EQ(x, y) << f << " differs between two emissions";
  }
}

// --------------------------------------------------------------------
// R-11, the other half. Nothing that varies between two machines or
// two days reaches an emitted file.
// --------------------------------------------------------------------
TEST(Emit, NothingEmittedCarriesAVaryingValue)
{
  const std::string out = Fixture::scratch("vary");
  auto drv = Fixture::run_emit(
      std::filesystem::absolute(Fixture::pacino()).generic_string(),
      out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::vector<std::string> files = Fixture::tree(out);
  ASSERT_FALSE(files.empty());

  for(const std::string &f : files) {
    const std::string body = Fixture::slurp(out + "/" + f);

    // the absolute path the configuration was given by
    EXPECT_EQ(std::string::npos, body.find(out))
      << f << " carries the output directory path";
    EXPECT_EQ(std::string::npos,
              body.find(std::filesystem::absolute(
                  Fixture::pacino()).parent_path().generic_string()))
      << f << " carries the configuration's absolute path";

    // a date. The generated header names a tool and a source and
    // nothing that moves.
    EXPECT_EQ(std::string::npos, body.find("__DATE__"))
      << f << " carries a date macro";
    EXPECT_EQ(std::string::npos, body.find("__TIME__"))
      << f << " carries a time macro";
  }
}

// --------------------------------------------------------------------
// R-11. Every emitted file carries the generated header: the SPDX
// block, the do not edit statement, the tool and the source.
// --------------------------------------------------------------------
TEST(Emit, EveryEmittedFileCarriesTheGeneratedHeader)
{
  const std::string out = Fixture::scratch("header");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::vector<std::string> files = Fixture::tree(out);
  ASSERT_FALSE(files.empty());

  for(const std::string &f : files) {
    const std::string body = Fixture::slurp(out + "/" + f);
    EXPECT_NE(std::string::npos,
              body.find("SPDX-License-Identifier: Apache-2.0"))
      << f << " has no SPDX line";
    EXPECT_NE(std::string::npos, body.find("DO NOT HAND"))
      << f << " does not say it is generated";
    EXPECT_NE(std::string::npos, body.find("TOOL:      cgen"))
      << f << " does not name the tool";
    EXPECT_NE(std::string::npos, body.find("system 'pacino'"))
      << f << " does not name the configuration it came from";
  }
}

// --------------------------------------------------------------------
// A configuration the emitter cannot build is a diagnostic, and it
// stops emission the same way an error from the check path does.
// --------------------------------------------------------------------
TEST(Emit, AnUnbuildableNodeIsReportedAndNothingIsWritten)
{
  const std::string out = Fixture::scratch("unsupported");
  const std::string cfg = Fixture::fixture_dir() +
                          "/base/base_system.json";

  auto drv = Fixture::run_emit(cfg, out);

  EXPECT_EQ(size_t(1),
            drv->diags().count_code(cgen::code::emit_unsupported))
    << "codes: " << Fixture::codes(drv->diags());
  EXPECT_TRUE(drv->emitted().empty());
  EXPECT_TRUE(Fixture::tree(out).empty());
}

// --------------------------------------------------------------------
// R-4. What happens to a file already in the output tree that this
// run did not write: it is left alone. The emitter overwrites what it
// writes and removes nothing.
// --------------------------------------------------------------------
TEST(Emit, AFileTheRunDidNotWriteIsLeftAlone)
{
  const std::string out = Fixture::scratch("stale");

  std::filesystem::create_directories(out + "/stale_node/rtl");
  {
    std::ofstream os(out + "/stale_node/rtl/stale.sv");
    os << "// left by an earlier configuration\n";
  }

  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::vector<std::string> files = Fixture::tree(out);
  EXPECT_TRUE(has(files, "stale_node/rtl/stale.sv"))
    << "the emitter removed a file it did not write";

  // and it is not in what the run says it wrote, so a filelist
  // built from that list cannot pick it up
  EXPECT_FALSE(has(drv->emitted(), "stale_node/rtl/stale.sv"));
}

// --------------------------------------------------------------------
// The style rules give a width of 80 columns and the emitted tree is
// held to it. verilog_style.md sets no width of its own, so this is
// the project rule applied to generated text as well as to hand
// written text, and it is a rule a generator can break silently: the
// node name prefix is added when a file is written, after the line
// that carries it was built.
// --------------------------------------------------------------------
TEST(Emit, NoEmittedLineIsWiderThanEightyColumns)
{
  const std::string out = Fixture::scratch("width");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  const std::vector<std::string> files = Fixture::tree(out);
  ASSERT_FALSE(files.empty());

  int over = 0;
  for(const std::string &f : files) {
    const std::string body = Fixture::slurp(out + "/" + f);
    size_t at = 0;
    int    ln = 1;
    while(at <= body.size()) {
      const size_t nl = body.find('\n', at);
      const size_t end = nl == std::string::npos ? body.size() : nl;
      const size_t len = end - at;
      if(len > 80) {
        ++over;
        if(over <= 5) {
          ADD_FAILURE() << f << ":" << ln << " is " << len
                        << " columns: " << body.substr(at, len);
        }
      }
      if(nl == std::string::npos) break;
      at = nl + 1;
      ++ln;
    }
  }
  EXPECT_EQ(0, over) << over << " lines are wider than 80 columns";
}

// --------------------------------------------------------------------
// A tab in emitted text would defeat the width rule and the indent
// rule at once. A Makefile is exempt: make requires a tab.
// --------------------------------------------------------------------
TEST(Emit, NoEmittedSourceFileCarriesATab)
{
  const std::string out = Fixture::scratch("tabs");
  auto drv = Fixture::run_emit(Fixture::pacino(), out);
  ASSERT_FALSE(drv->diags().has_error());

  for(const std::string &f : Fixture::tree(out)) {
    if(f.find("Makefile") != std::string::npos) continue;
    const std::string body = Fixture::slurp(out + "/" + f);
    EXPECT_EQ(std::string::npos, body.find('\t')) << f << " has a tab";
  }
}
