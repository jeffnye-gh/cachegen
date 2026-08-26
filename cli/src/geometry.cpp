// --------------------------------------------------------------------
// FILE:    geometry.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "geometry.h"
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
    diags_.error(n.cache_file, site, "T-8.geometry_fields",
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
    diags_.error(n.cache_file, site, "T-8.no_addressing",
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
    diags_.error(n.cache_file, site, "T-8.sets_integer",
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
    diags_.error(n.cache_file, site, "T-8.sets_pow2",
                 "cache " + msg->tq(n.cache) + " set count " +
                 u64(q.sets) + " is not a power of two");
    return;
  }

  // ------------------------------------------------------------------
  // capacity_bytes is a power of two
  // ------------------------------------------------------------------
  if(!is_pow2(q.capacity_bytes)) {
    diags_.error(n.cache_file, site, "T-8.capacity_pow2",
                 "cache " + msg->tq(n.cache) + " capacity_bytes " +
                 u64(q.capacity_bytes) + " is not a power of two");
  }

  q.offset_bits   = log2_exact(q.line_bytes);
  q.index_bits    = log2_exact(q.sets);
  q.bytes_per_way = q.capacity_bytes / uint64_t(q.associativity);
  q.bank_bits     = log2_exact(uint64_t(q.banks));

  if(q.offset_bits < 0) {
    diags_.error(n.cache_file, site, "T-8.line_pow2",
                 "cache " + msg->tq(n.cache) + " line_bytes " +
                 u64(q.line_bytes) + " is not a power of two");
    return;
  }

  // ------------------------------------------------------------------
  // offset_bits + index_bits + tag_bits == pa_bits, tag_bits >= 1
  // ------------------------------------------------------------------
  q.tag_bits = m.pa_bits - q.offset_bits - q.index_bits;
  if(q.tag_bits < 1) {
    diags_.error(n.cache_file, site, "T-8.tag_bits",
                 "cache " + msg->tq(n.cache) + " leaves tag_bits " +
                 std::to_string(q.tag_bits) + ", offset " +
                 std::to_string(q.offset_bits) + " + index " +
                 std::to_string(q.index_bits) + " against pa_bits " +
                 std::to_string(m.pa_bits));
    return;
  }

  if(q.offset_bits + q.index_bits + q.tag_bits != m.pa_bits) {
    diags_.error(n.cache_file, site, "T-8.field_sum",
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
      diags_.error(n.cache_file, site, "T-8.vipt_no_page",
                   "cache " + msg->tq(n.cache) +
                   " is VIPT but the addressing block carries no "
                   "page_bytes, the index budget cannot be checked");
    } else if(q.bytes_per_way > uint64_t(m.page_bytes)) {
      diags_.error(n.cache_file, site, "T-8.vipt_index",
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
      diags_.error(n.cache_file, site, "T-8.bank_divide",
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

  q.valid = true;
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
