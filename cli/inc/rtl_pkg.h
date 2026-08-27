// --------------------------------------------------------------------
// FILE:    rtl_pkg.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The emitted packages. Every derived number and every encoding the
// design uses is written here once and read by every consumer, which
// is what D-39 asks for.
//
// Three kinds:
//   cgen_tl_pkg      the TileLink opcodes and parameters of spec
//                    1.9.3, the same for every link
//   <link>_pkg       one link's widths, from its definition
//   <node>_pkg       one node's geometry, address decomposition and
//                    replacement encoding
// --------------------------------------------------------------------
#pragma once
#include "link_sig.h"
#include "node_ctx.h"
#include "sv_file.h"
#include <string>

namespace cgen
{

class RtlPkg
{
public:
  // the protocol constants, shared by every TileLink link
  static void tilelink(SvFile &f);

  // one link definition's widths
  static void link(SvFile &f, const std::string &name,
                   const LinkSig &s);

  // one node's geometry, fields and replacement
  static void node(SvFile &f, const NodeCtx &c);

  // the file scope import of one package, placed and wrapped the
  // way both the project style and Verilator can live with. See the
  // note in the body.
  static void import_of(SvFile &f,
                        const std::vector<std::string> &pkgs);

  static std::string tl_pkg_name()  { return "cgen_tl_pkg"; }
  static std::string link_pkg(const std::string &n) { return n + "_pkg"; }
};

} // namespace cgen
