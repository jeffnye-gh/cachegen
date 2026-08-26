// --------------------------------------------------------------------
// FILE:    driver.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "driver.h"
#include "report.h"
#include "msg.h"

namespace cgen
{

// --------------------------------------------------------------------
Driver::Driver(const Args &args)
  : args_(args),
    loader_(diags_),
    schemas_(diags_),
    syms_(diags_)
{
  diags_.set_eoe(args_.eoe);
}

// --------------------------------------------------------------------
int Driver::run()
{
  bool halted = false;

  try {
    // ----------------------------------------------------------------
    // R-3 load, R-4 validate, R-5 enumerate, R-6 resolve
    // ----------------------------------------------------------------
    if(!loader_.load(args_.config)) {
      if(!args_.quiet) diags_.print();
      return 1;
    }

    if(schemas_.locate(loader_.root_dir())) {
      for(const Loader::File &f : loader_.files()) schemas_.validate(f);
    }

    syms_.build(loader_.files());

    Resolver r(diags_, syms_);
    r.resolve(loader_.files(), model_);

    // ----------------------------------------------------------------
    // R-8 derive, R-7 check
    // ----------------------------------------------------------------
    Geometry g(diags_);
    g.compute(model_);

    Checker c(diags_, syms_);
    c.run(model_);

  } catch(const DiagList::Halt &) {
    halted = true;
  }

  // ------------------------------------------------------------------
  // R-9 report
  // ------------------------------------------------------------------
  if(!args_.quiet) {
    if(halted) {
      msg->emsg("stopped on the first error, --eoe is set");
    } else {
      Report rep;
      rep.check(model_, diags_);
      if(!schemas_.dir().empty()) {
        msg->imsg("schemas read from " + schemas_.dir());
      }
    }
  }

  if(args_.cmd == "emit" && !args_.quiet) {
    msg->imsg("");
    msg->wmsg("--cmd emit is not implemented, the configuration was "
              "parsed and checked only");
  }

  return diags_.has_error() ? 1 : 0;
}

} // namespace cgen
