// --------------------------------------------------------------------
// FILE:    driver.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "driver.h"
#include "emitter.h"
#include "report.h"
#include "diag_codes.h"
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

  // ------------------------------------------------------------------
  // R-6b. Every read of a configuration field is recorded against the
  // file and pointer it came from, for the life of this run. What no
  // stage read is the unconsumed field report.
  // ------------------------------------------------------------------
  FieldUse::Scope use_scope(use_);

  try {
    // ----------------------------------------------------------------
    // R-3 load, R-4 validate, R-5 enumerate, R-6 resolve
    // ----------------------------------------------------------------
    if(!loader_.load(args_.config)) {
      if(!args_.quiet) diags_.print();
      return 1;
    }

    use_.enumerate(loader_.files());

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

  // ------------------------------------------------------------------
  // R-3, CLI-004. emit runs the whole check path first and writes
  // NOTHING when any diagnostic was an error. A warning does not stop
  // it. The check path above has already run, so the decision is just
  // whether an error reached the list.
  // ------------------------------------------------------------------
  if(args_.cmd == "emit" && !halted) {
    Emitter em(diags_, args_.output);
    em.set_vars(args_.vars, args_.tool);
    em.set_field_use(&use_);
    const bool wrote = em.run(model_, loader_);

    if(!args_.quiet) {
      msg->imsg("");
      if(wrote) {
        msg->imsg("emitted " + std::to_string(em.written().size()) +
                  " files under " + args_.output);
        for(const std::string &w : em.written()) {
          msg->imsg("  " + w);
        }
      } else {
        msg->emsg("nothing was emitted");
      }
      // Every code the emitter can raise on its own. The check
      // path's report ran before this stage, so a diagnostic
      // raised here would otherwise reach the list and never
      // reach the console.
      for(const Diag &d : diags_.all()) {
        if(d.code() == std::string(code::emit_refused) ||
           d.code() == std::string(code::emit_unsupported) ||
           d.code() == std::string(code::t11_read_latency) ||
           d.code() == std::string(code::t11_tag_stage)) {
          msg->emsg("  " + d.message());
        }
      }
    }
    emitted_ = em.written();
    feats_   = em.features();
  } else if(args_.cmd == "emit" && !args_.quiet) {
    msg->emsg("nothing was emitted, the run stopped on the first "
              "error");
  }

  return diags_.has_error() ? 1 : 0;
}

} // namespace cgen
