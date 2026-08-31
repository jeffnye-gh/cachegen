// --------------------------------------------------------------------
// FILE:    diag_codes.h
// SOURCE:  CLI-003
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The one list of every diagnostic code the tool can emit, across
// every stage. An emission site draws its code string from the
// constants below, so a code cannot reach a diagnostic without
// appearing here first.
//
// Each entry carries how the code is reached. That is what lets a
// test relate the codes the tool can emit to the codes a fixture
// exercises, see Diagnostics.EveryCodeHasAFixture.
//
//   Fixture  a configuration or schema directory under tb/fixtures
//            produces it. The suite requires one.
//   Guard    no schema valid configuration reaches it. The schema
//            or the loader rejects the input first, or the
//            arithmetic makes the condition impossible. The note
//            says which.
//   Env      reachable only from a filesystem or permission failure
//            the suite cannot commit.
//
// A Guard or an Env code has no fixture on purpose. Moving one into
// Fixture without adding a fixture fails the suite, and a Guard that
// a fixture does reach fails it too, so neither side can drift
// unnoticed.
// --------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>

namespace cgen
{

// --------------------------------------------------------------------
// identifier, code string, reach, note. The note is empty unless the
// reach is Guard or Env, where it says why there is no fixture.
// --------------------------------------------------------------------
#define CGEN_DIAG_CODES(X)                                            \
  X(load_open,           "load.open",           Fixture, "")          \
  X(load_parse,          "load.parse",          Fixture, "")          \
  X(load_type,           "load.type",           Fixture, "")          \
  X(load_include_type,   "load.include_type",   Fixture, "")          \
  X(load_include_cycle,  "load.include_cycle",  Fixture, "")          \
  X(load_include_shape,  "load.include_shape",  Guard,                \
    "every include schema requires an object carrying a string "      \
    "file and a string type, so a shape failure is a schema "         \
    "violation before the loader can see it")                         \
  X(schema_dir,          "schema.dir",          Fixture, "")          \
  X(schema_parse,        "schema.parse",        Fixture, "")          \
  X(schema_build,        "schema.build",        Fixture, "")          \
  X(schema_violation,    "schema.violation",    Fixture, "")          \
  X(schema_open,         "schema.open",         Env,                  \
    "a schema file that exists and cannot be read. That is a file "   \
    "mode, and a file mode is not something a fixture can carry "     \
    "into the tree")                                                  \
  X(schema_unknown_type, "schema.unknown_type", Guard,                \
    "a file_type outside the five fails the loader's claimed "        \
    "against declared check first, so this never fires alone")        \
  X(topology_addressing, "topology.addressing", Fixture, "")          \
  X(t1_node_cache,       "T-1.node_cache",      Fixture, "")          \
  X(t1_iface_link,       "T-1.iface_link",      Fixture, "")          \
  X(t1_port_type,        "T-1.port_type",       Fixture, "")          \
  X(t1_link_port_type,   "T-1.link_port_type",  Fixture, "")          \
  X(t1_edge_endpoint,    "T-1.edge_endpoint",   Fixture, "")          \
  X(t1_edge_interface,   "T-1.edge_interface",  Fixture, "")          \
  X(t1_edge_port,        "T-1.edge_port",       Fixture, "")          \
  X(t2_duplicate,        "T-2.duplicate",       Fixture, "")          \
  X(t3_port_type,        "T-3.port_type",       Fixture, "")          \
  X(t4_port_role,        "T-4.port_role",       Fixture, "")          \
  X(t5_cycle,            "T-5.cycle",           Fixture, "")          \
  X(t6_group,            "T-6.group",           Fixture, "")          \
  X(t9_link_agree,       "T-9.link_agree",      Fixture, "")          \
  X(t10_addr_width,      "T-10.addr_width",     Fixture, "")          \
  X(t8_sets_integer,     "T-8.sets_integer",    Fixture, "")          \
  X(t8_sets_pow2,        "T-8.sets_pow2",       Fixture, "")          \
  X(t8_capacity_pow2,    "T-8.capacity_pow2",   Fixture, "")          \
  X(t8_tag_bits,         "T-8.tag_bits",        Fixture, "")          \
  X(t8_vipt_index,       "T-8.vipt_index",      Fixture, "")          \
  X(t8_bank_divide,      "T-8.bank_divide",     Fixture, "")          \
  X(t8_geometry_fields,  "T-8.geometry_fields", Guard,                \
    "caches.schema.json requires capacity_bytes, line_bytes, "        \
    "associativity and banks unconditionally inside geometry")        \
  X(t8_no_addressing,    "T-8.no_addressing",   Guard,                \
    "topology.schema.json requires an addressing block, and a "       \
    "configuration with no topology file has no node to derive")      \
  X(t8_line_pow2,        "T-8.line_pow2",       Guard,                \
    "the line_bytes enum carries powers of two only, and a line "     \
    "with an odd factor fails the set count checks first")            \
  X(t8_field_sum,        "T-8.field_sum",       Guard,                \
    "tag_bits is assigned pa_bits minus the other two fields one "    \
    "line above the test, so the sum holds by construction")          \
  X(t8_vipt_no_page,     "T-8.vipt_no_page",    Guard,                \
    "topology.schema.json requires page_bytes inside addressing")     \
  X(emit_refused,        "emit.refused",        Fixture, "")          \
  X(emit_unsupported,    "emit.unsupported",    Fixture, "")          \
  X(t11_read_latency,    "T-11.read_latency",   Fixture, "")          \
  X(t11_tag_stage,       "T-11.tag_stage",      Fixture, "")          \
  X(emit_mkdir,          "emit.mkdir",          Env,                  \
    "a directory under the output tree that cannot be created. "      \
    "That is a permission or a full filesystem, and neither is "      \
    "something a fixture can carry into the tree")                    \
  X(emit_write,          "emit.write",          Env,                  \
    "an output file that cannot be opened for writing, for the "      \
    "same reason emit.mkdir cannot be fixtured")                      \
  X(emit_vars,           "emit.vars",           Fixture, "")

class DiagCodes
{
public:
  enum class Reach { Fixture, Guard, Env };

  struct Entry {
    const char *code;
    Reach       reach;
    const char *note;    // empty unless the reach is Guard or Env
  };

  // every code the tool can emit, in the order declared above
  static const std::vector<Entry> &all();

  // the entry for one code string, null when the code is not listed
  static const Entry *find(const std::string &code);

  // every code carrying one reach
  static std::vector<std::string> of_reach(Reach r);

  static std::string reach_text(Reach r);
};

// --------------------------------------------------------------------
// The code strings, one constant per code. Emission sites use these
// and never a literal.
// --------------------------------------------------------------------
namespace code
{
#define CGEN_DIAG_CODE_CONST(id, str, reach, note)                    \
  inline constexpr const char *id = str;
CGEN_DIAG_CODES(CGEN_DIAG_CODE_CONST)
#undef CGEN_DIAG_CODE_CONST
} // namespace code

} // namespace cgen
