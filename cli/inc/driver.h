// --------------------------------------------------------------------
// FILE:    driver.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// Runs the front half in order: load, validate, enumerate, resolve,
// derive, check, report. Owns everything the Model points into, so a
// Driver must outlive any use of its model().
// --------------------------------------------------------------------
#pragma once
#include "checker.h"
#include "diag_list.h"
#include "geometry.h"
#include "loader.h"
#include "model.h"
#include "resolver.h"
#include "schema_set.h"
#include "symbol_table.h"
#include <string>
#include <vector>

namespace cgen
{

class Driver
{
public:
  struct Args {
    std::string cmd{"check"};
    std::string config;
    std::string output{"./output"};
    bool        eoe{false};
    bool        quiet{false};    // suppress the report, used by tests
  };

  explicit Driver(const Args &args);

  // process exit status, non zero when any diagnostic is an error
  int run();

  DiagList       &diags()       { return diags_; }
  const DiagList &diags() const { return diags_; }
  const Model    &model() const { return model_; }
  const SchemaSet &schemas() const { return schemas_; }

  // every path --cmd=emit wrote, relative to the output directory
  const std::vector<std::string> &emitted() const { return emitted_; }

private:
  Args        args_;
  DiagList    diags_;
  Loader      loader_;
  SchemaSet   schemas_;
  SymbolTable syms_;
  Model       model_;
  std::vector<std::string> emitted_;
};

} // namespace cgen
