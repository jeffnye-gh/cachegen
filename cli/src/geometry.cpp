// --------------------------------------------------------------------
// FILE:    geometry.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "geometry.h"
#include "diag_codes.h"
#include "msg.h"

using nlohmann::json;

namespace cgen
{

namespace {
std::string u64(uint64_t v) { return std::to_string(v); }
}

// --------------------------------------------------------------------
bool Geometry::is_pow2(uint64_t v)
{
  return v != 0 && (v & (v - 1)) == 0;
}

// --------------------------------------------------------------------
int Geometry::log2_exact(uint64_t v)
{
  if(!is_pow2(v)) return -1;
  int n = 0;
  while((v >> n) != 1) ++n;
  return n;
}

// --------------------------------------------------------------------
// lsb, msb, width, shift and mask for one contiguous address field
// --------------------------------------------------------------------
Model::Field Geometry::make_field(int lsb, int bits)
{
  Model::Field f;
  f.valid = bits > 0;
  f.lsb   = lsb;
  f.bits  = bits;
  f.msb   = bits > 0 ? lsb + bits - 1 : lsb - 1;
  f.shift = lsb;
  f.mask  = 0;
  if(bits > 0 && bits < 64) {
    f.mask = ((uint64_t(1) << bits) - 1) << lsb;
  } else if(bits >= 64) {
    f.mask = ~uint64_t(0);
  }
  return f;
}

// --------------------------------------------------------------------
void Geometry::compute(Model &m)
{
  for(Model::Node &n : m.nodes) {
    if(!n.resolved) continue;
    one(m, n);
  }
  for(Model::Node &n : m.nodes) {
    if(!n.geom.valid) continue;
    refill(m, n);
  }
}

// --------------------------------------------------------------------
void Geometry::one(Model &m, Model::Node &n)
{
  if(n.body == nullptr || !n.body->is_object())          return;
  if(!n.body->contains("geometry"))                      return;

  const json &g = (*n.body)["geometry"];
  if(!g.is_object())                                     return;

  const std::string site = n.cache_path + "/geometry";

  if(!g.contains("capacity_bytes") || !g.contains("line_bytes") ||
     !g.contains("associativity")  || !g.contains("banks")) {
    diags_.error(n.cache_file, site, code::t8_geometry_fields,
                 "cache " + msg->tq(n.cache) +
                 " geometry is missing a required field");
    return;
  }

  Model::Geom &q   = n.geom;
  q.capacity_bytes = g["capacity_bytes"].get<uint64_t>();
  q.line_bytes     = g["line_bytes"].get<uint64_t>();
  q.associativity  = g["associativity"].get<int>();
  q.banks          = g["banks"].get<int>();

  if(!m.has_addressing) {
    diags_.error(n.cache_file, site, code::t8_no_addressing,
                 "cache " + msg->tq(n.cache) +
                 " needs pa_bits, no topology addressing block was read");
    return;
  }

  // ------------------------------------------------------------------
  // capacity_bytes / (line_bytes * associativity) integer and >= 1
  // ------------------------------------------------------------------
  uint64_t per_set = q.line_bytes * uint64_t(q.associativity);
  if(per_set == 0 || q.capacity_bytes % per_set != 0 ||
     q.capacity_bytes / per_set < 1) {
    diags_.error(n.cache_file, site, code::t8_sets_integer,
                 "cache " + msg->tq(n.cache) + " capacity " +
                 u64(q.capacity_bytes) + " / (line " +
                 u64(q.line_bytes) + " * ways " +
                 std::to_string(q.associativity) +
                 ") is not an integer set count of at least one");
    return;
  }
  q.sets = q.capacity_bytes / per_set;

  // ------------------------------------------------------------------
  // that set count is a power of two
  // ------------------------------------------------------------------
  if(!is_pow2(q.sets)) {
    diags_.error(n.cache_file, site, code::t8_sets_pow2,
                 "cache " + msg->tq(n.cache) + " set count " +
                 u64(q.sets) + " is not a power of two");
    return;
  }

  // ------------------------------------------------------------------
  // capacity_bytes is a power of two
  // ------------------------------------------------------------------
  if(!is_pow2(q.capacity_bytes)) {
    diags_.error(n.cache_file, site, code::t8_capacity_pow2,
                 "cache " + msg->tq(n.cache) + " capacity_bytes " +
                 u64(q.capacity_bytes) + " is not a power of two");
  }

  q.offset_bits   = log2_exact(q.line_bytes);
  q.index_bits    = log2_exact(q.sets);
  q.bytes_per_way = q.capacity_bytes / uint64_t(q.associativity);
  q.bank_bits     = log2_exact(uint64_t(q.banks));

  if(q.offset_bits < 0) {
    diags_.error(n.cache_file, site, code::t8_line_pow2,
                 "cache " + msg->tq(n.cache) + " line_bytes " +
                 u64(q.line_bytes) + " is not a power of two");
    return;
  }

  // ------------------------------------------------------------------
  // offset_bits + index_bits + tag_bits == pa_bits, tag_bits >= 1
  // ------------------------------------------------------------------
  q.tag_bits = m.pa_bits - q.offset_bits - q.index_bits;
  if(q.tag_bits < 1) {
    diags_.error(n.cache_file, site, code::t8_tag_bits,
                 "cache " + msg->tq(n.cache) + " leaves tag_bits " +
                 std::to_string(q.tag_bits) + ", offset " +
                 std::to_string(q.offset_bits) + " + index " +
                 std::to_string(q.index_bits) + " against pa_bits " +
                 std::to_string(m.pa_bits));
    return;
  }

  if(q.offset_bits + q.index_bits + q.tag_bits != m.pa_bits) {
    diags_.error(n.cache_file, site, code::t8_field_sum,
                 "cache " + msg->tq(n.cache) +
                 " address fields do not sum to pa_bits");
    return;
  }

  q.offset = make_field(0, q.offset_bits);
  q.index  = make_field(q.offset_bits, q.index_bits);
  q.tag    = make_field(q.offset_bits + q.index_bits, q.tag_bits);

  // ------------------------------------------------------------------
  // for a VIPT cache, bytes per way <= page_bytes
  // ------------------------------------------------------------------
  if(n.indexing == "VIPT") {
    if(!m.has_page_bytes) {
      diags_.error(n.cache_file, site, code::t8_vipt_no_page,
                   "cache " + msg->tq(n.cache) +
                   " is VIPT but the addressing block carries no "
                   "page_bytes, the index budget cannot be checked");
    } else if(q.bytes_per_way > uint64_t(m.page_bytes)) {
      diags_.error(n.cache_file, site, code::t8_vipt_index,
                   "VIPT cache " + msg->tq(n.cache) + " has " +
                   u64(q.bytes_per_way) +
                   " bytes per way against a page of " +
                   std::to_string(m.page_bytes) +
                   " bytes, the index reaches above the page offset "
                   "and the configuration has synonyms");
    }
  }

  // ------------------------------------------------------------------
  // for banks > 1, banks divides the set count
  // ------------------------------------------------------------------
  if(q.banks > 1) {
    if(q.sets % uint64_t(q.banks) != 0) {
      diags_.error(n.cache_file, site, code::t8_bank_divide,
                   "cache " + msg->tq(n.cache) + " has " +
                   std::to_string(q.banks) +
                   " banks which does not divide the set count " +
                   u64(q.sets));
    } else {
      q.sets_per_bank = q.sets / uint64_t(q.banks);
    }
  } else {
    q.sets_per_bank = q.sets;
  }

  bank_field(n, g);

  q.valid = true;
}

// --------------------------------------------------------------------
// R-6, CLI-004. The bank select field.
//
// The index already spans the WHOLE set space, because the rule
// offset + index + tag == pa_bits forces it and leaves no bits over.
// A bank select is therefore a subfield of one of the three fields,
// never a fourth field beside them. bank_select_position says which
// end of the index it is taken from, and bank_interleave_granularity
// says whether it is taken from the index at all.
//
//   line, above_index   the top bank_bits of the index
//   line, below_index   the bottom bank_bits of the index
//   word                the select lies inside the line, below the
//                       line boundary, and the index is untouched
//
// The line cases are checked against sets_per_bank, which is derived
// independently above. A disagreement means the reading is wrong.
// --------------------------------------------------------------------
void Geometry::bank_field(Model::Node &n, const json &g)
{
  Model::Geom &q = n.geom;

  q.set_index      = q.index;
  q.bank           = make_field(0, 0);
  q.bank_resolved  = false;

  if(g.contains("bank_interleave_granularity")) {
    q.bank_granularity =
        g["bank_interleave_granularity"].get<std::string>();
  }
  if(g.contains("bank_select_position")) {
    q.bank_position = g["bank_select_position"].get<std::string>();
  }

  // one bank, there is no select and nothing is taken from the index
  if(q.banks <= 1) {
    q.bank_resolved = true;
    q.bank_note     = "one bank, no bank select field";
    return;
  }

  if(q.bank_bits <= 0) {
    q.bank_note = "bank count " + std::to_string(q.banks) +
                  " is not a power of two, the select has no width";
    return;
  }

  if(q.sets % uint64_t(q.banks) != 0) {
    q.bank_note = "the bank count does not divide the set count, "
                  "see T-8.bank_divide";
    return;
  }

  // ------------------------------------------------------------------
  // word interleaving puts the select below the line boundary, so it
  // is inside the offset and bank_select_position has no bearing on
  // it. sets_per_bank, derived as sets / banks above, is then wrong
  // as well: every bank holds every set and a fraction of each line.
  // That is a derivation this task is not permitted to change, so the
  // field is reported unresolved rather than guessed.
  // ------------------------------------------------------------------
  if(q.bank_granularity == "word") {
    q.bank_note = "bank_interleave_granularity is word, which puts "
                  "the select inside the line offset where "
                  "bank_select_position does not reach, and leaves "
                  "sets_per_bank derived as sets / banks disagreeing "
                  "with every bank holding every set";
    return;
  }

  if(q.bank_granularity != "line") {
    q.bank_note = "bank_interleave_granularity " +
                  (q.bank_granularity.empty()
                       ? std::string("is absent")
                       : msg->tq(q.bank_granularity)) +
                  " is not one this derivation covers";
    return;
  }

  if(q.bank_bits > q.index_bits) {
    q.bank_note = "the bank select needs " +
                  std::to_string(q.bank_bits) +
                  " bits and the index carries only " +
                  std::to_string(q.index_bits);
    return;
  }

  if(q.bank_position == "above_index") {
    q.bank      = make_field(q.index.msb - q.bank_bits + 1, q.bank_bits);
    q.set_index = make_field(q.index.lsb, q.index_bits - q.bank_bits);
  } else if(q.bank_position == "below_index") {
    q.bank      = make_field(q.index.lsb, q.bank_bits);
    q.set_index = make_field(q.index.lsb + q.bank_bits,
                             q.index_bits - q.bank_bits);
  } else {
    q.bank_note = "bank_select_position " +
                  (q.bank_position.empty()
                       ? std::string("is absent")
                       : msg->tq(q.bank_position)) +
                  ", neither above_index nor below_index";
    return;
  }

  // ------------------------------------------------------------------
  // the corroboration. sets_per_bank came from sets / banks and the
  // set index width came from the field arithmetic. They are two
  // routes to one number and they have to agree.
  // ------------------------------------------------------------------
  const int want = log2_exact(q.sets_per_bank);
  if(want != q.set_index.bits) {
    q.bank_note = "the set index the field arithmetic leaves is " +
                  std::to_string(q.set_index.bits) +
                  " bits and sets_per_bank " + u64(q.sets_per_bank) +
                  " wants " + std::to_string(want);
    q.set_index = q.index;
    q.bank      = make_field(0, 0);
    return;
  }

  q.bank_resolved = true;
  q.bank_note     = "the index spans the whole set space, so the " +
                    q.bank_position +
                    " select is taken out of it, leaving " +
                    u64(q.sets_per_bank) + " sets in each of the " +
                    std::to_string(q.banks) + " banks";
}

// --------------------------------------------------------------------
// Refill beat count comes from the line size and the width of the one
// downstream link, R-8. A terminal node has no downstream link.
// --------------------------------------------------------------------
void Geometry::refill(Model &m, Model::Node &n)
{
  const Model::Edge *down = nullptr;
  int count = 0;

  for(const Model::Edge &e : m.edges) {
    if(e.from != n.name) continue;
    ++count;
    if(down == nullptr) down = &e;
  }

  if(count == 0) {
    n.geom.refill_note = "no downstream link, terminal node";
    return;
  }
  if(count > 1) {
    n.geom.refill_note = "several downstream links, beat count is "
                         "ambiguous";
    return;
  }
  if(!down->width_known || down->width_bytes <= 0) {
    n.geom.refill_note = "downstream link width is not known";
    return;
  }

  uint64_t w = uint64_t(down->width_bytes);
  n.geom.refill_beats = int((n.geom.line_bytes + w - 1) / w);
  if(n.geom.line_bytes % w != 0) {
    n.geom.refill_note = "line does not divide evenly by the link width";
  }
}

} // namespace cgen
