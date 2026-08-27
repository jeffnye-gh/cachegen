// --------------------------------------------------------------------
// FILE:    gen_log.h
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
//
// R-6. The generation logs, under <output>/logs. The tool writes what
// it did and what it noticed.
//
//   emission.log     every file written, every node emitted, every
//                    node skipped and why, and the tool variable set
//                    the emitted build was given
//   unconsumed.log   R-6b, every field the configuration carries that
//                    no stage read
//   geometry.log     the derived geometry per node, the same content
//                    the console report carries
//   features.log     R-8, every feature the configuration declares,
//                    with the test that covers it or the reason none
//                    can
//
// WHAT VARIES BETWEEN TWO RUNS, and nothing else does.
//
// One block of emission.log varies: the Vars.mk provenance. It names
// the master copy the run was given and the value every tool variable
// resolved to, and both are command line input. That is the same
// exemption Vars.mk itself has under R-4 and for the same reason.
//
// Everything else in all three logs is a function of the
// configuration alone. Configuration files are named by their BASE
// NAME, never by a path, because a path is relative to the working
// directory and would make two runs of one configuration from two
// directories differ. That is R-11's rule applied to a log.
// --------------------------------------------------------------------
#pragma once
#include "feature_table.h"
#include "field_use.h"
#include "model.h"
#include "node_ctx.h"
#include "sv_file.h"
#include "tool_vars.h"
#include <map>
#include <string>
#include <vector>

namespace cgen
{

class GenLog
{
public:
  // one node the run did not emit, and why
  struct Skipped {
    std::string node;
    std::string why;
  };

  static void emission(SvFile &f, const Model &m,
                       const std::map<std::string, NodeCtx> &nodes,
                       const std::vector<std::string> &written,
                       const std::vector<Skipped> &skipped,
                       const ToolVars &tv);

  static void unconsumed(SvFile &f, const Model &m, const FieldUse &u);

  static void geometry(SvFile &f, const Model &m);

  static void features(SvFile &f, const Features &ft);

  static const char *dir()            { return "logs"; }
  static const char *emission_name()  { return "emission.log"; }
  static const char *unconsumed_name(){ return "unconsumed.log"; }
  static const char *geometry_name()  { return "geometry.log"; }
  static const char *features_name()  { return "features.log"; }
};

} // namespace cgen
