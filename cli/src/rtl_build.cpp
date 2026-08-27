// --------------------------------------------------------------------
// FILE:    rtl_build.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "rtl_build.h"

namespace cgen
{

namespace {

// The lint bar of R-8: every warning on, and nothing waived from the
// command line. A waiver that is needed lives in the source, at the
// lines that need it, where it can be read and counted.
void lint_vars(SvFile &f)
{
  f.ln("VERILATOR ?= verilator");
  f.ln("WARN      = -Wall");
  f.ln("STD       = --timing -sv");
  f.ln("TRACE     ?=");
  f.ln("PLUSARGS  ?=");
  f.ln();
  f.note("TRACE, PLUSARGS, the clock period and the cycle limit are");
  f.note("simulation control and live here, never in the "
         "configuration.");
  f.note("D-41. make trace=1 turns waveforms on.");
  f.ln("ifneq ($(TRACE),)");
  f.ln("  TRACEOPT = --trace");
  f.ln("else");
  f.ln("  TRACEOPT =");
  f.ln("endif");
  f.ln();
}

} // namespace

// --------------------------------------------------------------------
void RtlBuild::node_flist(SvFile &f, const NodeCtx &c,
                          const std::vector<std::string> &shared,
                          const std::vector<std::string> &rtl,
                          const std::vector<std::string> &tb)
{
  f.note("Filelist for node '" + c.name() + "'.");
  f.note("");
  f.note("Order matters: a package has to be read before the module");
  f.note("that imports it.");
  f.ln();
  f.note("the tests and the task set are included, not compiled, so");
  f.note("the testbench directory has to be on the include path");
  f.ln("+incdir+tb");
  f.ln();
  f.note("shared packages");
  for(const std::string &s : shared) f.ln(s);
  f.ln();
  f.note("the node");
  for(const std::string &s : rtl) f.ln(s);
  f.ln();
  f.note("the testbench");
  for(const std::string &s : tb) f.ln(s);
}

// --------------------------------------------------------------------
void RtlBuild::node_make(SvFile &f, const NodeCtx &c)
{
  const std::string tb = c.mod("tb");

  f.note("Build and run the unit testbench of node '" + c.name() +
         "'.");
  f.note("");
  f.note("  make lint    every file, warnings on, nothing waived");
  f.note("               from the command line");
  f.note("  make build   compile and elaborate the testbench");
  f.note("  make run     build then run it");
  f.note("  make clean");
  f.bar();
  f.ln(".PHONY: default all lint build run clean");
  f.ln();
  f.ln("TOP   = " + tb);
  f.ln("FLIST = " + c.name() + ".f");
  f.ln("OBJ   = obj_dir");
  f.ln();
  lint_vars(f);
  f.ln("default: run");
  f.ln("all: lint build");
  f.ln();
  f.note("R-8a. Every emitted file passes lint with warnings on and");
  f.note("no warning of its own. The waivers that exist are in the");
  f.note("source at the lines that need them, not here.");
  f.ln("lint:");
  f.ln("\t$(VERILATOR) --lint-only $(WARN) $(STD) -f $(FLIST) \\");
  f.ln("\t  --top-module $(TOP)");
  f.ln();
  f.note("R-8b. Compile and elaborate.");
  f.ln("build:");
  f.ln("\t$(VERILATOR) --binary $(WARN) $(STD) $(TRACEOPT) \\");
  f.ln("\t  -f $(FLIST) --top-module $(TOP) --Mdir $(OBJ) \\");
  f.ln("\t  -o $(TOP)");
  f.ln();
  f.ln("run: build");
  f.ln("\t./$(OBJ)/$(TOP) $(PLUSARGS)");
  f.ln();
  f.ln("clean:");
  f.ln("\t-rm -rf $(OBJ)");
}

// --------------------------------------------------------------------
void RtlBuild::sys_flist(SvFile &f, const std::string &sys,
                         const std::vector<std::string> &files)
{
  f.note("Filelist for system '" + sys + "'. Every package, every");
  f.note("node, the system top and the top level testbench.");
  f.ln();
  f.note("the tests and the task set are included, not compiled");
  f.ln("+incdir+tb");
  f.ln();
  for(const std::string &s : files) f.ln(s);
}

// --------------------------------------------------------------------
void RtlBuild::sys_make(SvFile &f, const std::string &sys,
                        const std::vector<std::string> &nodes)
{
  f.note("Build and run system '" + sys + "'.");
  f.note("");
  f.note("  make lint       every emitted file, system and units");
  f.note("  make build      the system testbench");
  f.note("  make run        the system testbench");
  f.note("  make units      every unit testbench, built and run");
  f.note("  make all        lint, then the system, then the units");
  f.note("  make clean");
  f.bar();
  f.ln(".PHONY: default all lint build run units unit-lint clean");
  f.ln();
  f.ln("TOP   = " + sys + "_tb");
  f.ln("FLIST = " + sys + ".f");
  f.ln("OBJ   = obj_dir");
  f.ln("NODES = " + [&]{
    std::string s;
    for(size_t k = 0; k < nodes.size(); ++k) {
      if(k) s += " ";
      s += nodes[k];
    }
    return s;
  }());
  f.ln();
  lint_vars(f);
  f.ln("default: run");
  f.ln("all: lint build units");
  f.ln();
  f.ln("lint: unit-lint");
  f.ln("\t$(VERILATOR) --lint-only $(WARN) $(STD) -f $(FLIST) \\");
  f.ln("\t  --top-module $(TOP)");
  f.ln();
  f.ln("unit-lint:");
  f.ln("\t@for n in $(NODES); do \\");
  f.ln("\t  echo \"lint $$n\"; \\");
  f.ln("\t  $(MAKE) -s -C ../$$n lint || exit 1; \\");
  f.ln("\tdone");
  f.ln();
  f.ln("build:");
  f.ln("\t$(VERILATOR) --binary $(WARN) $(STD) $(TRACEOPT) \\");
  f.ln("\t  -f $(FLIST) --top-module $(TOP) --Mdir $(OBJ) \\");
  f.ln("\t  -o $(TOP)");
  f.ln();
  f.ln("run: build");
  f.ln("\t./$(OBJ)/$(TOP) $(PLUSARGS)");
  f.ln();
  f.ln("units:");
  f.ln("\t@for n in $(NODES); do \\");
  f.ln("\t  echo \"=== $$n ===\"; \\");
  f.ln("\t  $(MAKE) -s -C ../$$n run || exit 1; \\");
  f.ln("\tdone");
  f.ln();
  f.ln("clean:");
  f.ln("\t-rm -rf $(OBJ)");
  f.ln("\t@for n in $(NODES); do $(MAKE) -s -C ../$$n clean; done");
}

} // namespace cgen
