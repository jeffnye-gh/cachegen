// --------------------------------------------------------------------
// FILE:    rtl_build.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The Make based build of the emitted tree, R-5. One filelist and one
// Makefile per node, and one of each for the system.
//
// Everything the Makefile carries that is not a file name is
// simulation control: the lint switches, the trace enable, the cycle
// limit, the clock period. D-41 puts all of it here and none of it in
// the configuration.
// --------------------------------------------------------------------
#pragma once
#include "node_ctx.h"
#include "sv_file.h"
#include <map>
#include <string>
#include <vector>

namespace cgen
{

class RtlBuild
{
public:
  // a node's filelist: the shared packages, its own rtl, its tb
  static void node_flist(SvFile &f, const NodeCtx &c,
                         const std::vector<std::string> &shared,
                         const std::vector<std::string> &rtl,
                         const std::vector<std::string> &tb);

  static void node_make(SvFile &f, const NodeCtx &c);

  static void sys_flist(SvFile &f, const std::string &sys,
                        const std::vector<std::string> &files);

  static void sys_make(SvFile &f, const std::string &sys,
                       const std::vector<std::string> &nodes);
};

} // namespace cgen
