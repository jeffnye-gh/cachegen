// --------------------------------------------------------------------
// FILE:    rtl_mem.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The behavioural memory model. NOT SYNTHESIZABLE and not meant to
// be: R-5 asks for a model sufficient to elaborate and respond.
//
// The store is an associative array keyed by beat address. A memory
// the size of the one in this configuration cannot be a declared
// array, and a sparse store is the standard behavioural answer.
// --------------------------------------------------------------------
#pragma once
#include "node_ctx.h"
#include "sv_file.h"

namespace cgen
{

class RtlMem
{
public:
  static void slave(SvFile &f, const NodeCtx &c,
                    const NodeCtx::Iface &i);
  static void top(SvFile &f, const NodeCtx &c);
};

} // namespace cgen
