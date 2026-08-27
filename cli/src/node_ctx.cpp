// --------------------------------------------------------------------
// FILE:    node_ctx.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "node_ctx.h"
#include <algorithm>

using nlohmann::json;

namespace cgen
{

namespace {
const std::string kEmpty;
}

// --------------------------------------------------------------------
bool NodeCtx::is_cache() const
{
  return type_ == "icache" || type_ == "dcache" || type_ == "unified";
}

// --------------------------------------------------------------------
int NodeCtx::way_bits() const
{
  int b = Replacement::log2i(geom_.associativity);
  return b < 1 ? 1 : b;
}

// --------------------------------------------------------------------
int NodeCtx::set_idx_bits() const
{
  if(geom_.bank_resolved && geom_.banks > 1) return geom_.set_index.bits;
  return geom_.index_bits;
}

// --------------------------------------------------------------------
std::string NodeCtx::mod(const char *suffix) const
{
  std::string s = name_;
  if(suffix && *suffix) { s += "_"; s += suffix; }
  return s;
}

// --------------------------------------------------------------------
const std::string &NodeCtx::array_kind(const char *which) const
{
  const std::string w = which;
  if(w == "tag")   return tag_.kind;
  if(w == "data")  return data_.kind;
  if(w == "valid") return valid_.kind;
  if(w == "dirty") return dirty_.kind;
  if(w == "repl")  return replb_.kind;
  return kEmpty;
}

// --------------------------------------------------------------------
bool NodeCtx::cleared_on_reset(const char *which) const
{
  const std::string w = which;
  if(w == "tag")   return tag_.cleared;
  if(w == "data")  return data_.cleared;
  if(w == "valid") return valid_.cleared;
  if(w == "dirty") return dirty_.cleared;
  if(w == "repl")  return replb_.cleared;
  return false;
}

// --------------------------------------------------------------------
bool NodeCtx::registered_read(const char *which) const
{
  const std::string w = which;
  if(w == "tag")   return tag_.registered;
  if(w == "data")  return data_.registered;
  if(w == "valid") return valid_.registered;
  if(w == "dirty") return dirty_.registered;
  if(w == "repl")  return replb_.registered;
  return false;
}

// --------------------------------------------------------------------
bool NodeCtx::byte_enables(const char *which) const
{
  const std::string w = which;
  if(w == "tag")   return tag_.byte_en;
  if(w == "data")  return data_.byte_en;
  if(w == "valid") return valid_.byte_en;
  if(w == "dirty") return dirty_.byte_en;
  if(w == "repl")  return replb_.byte_en;
  return false;
}

// --------------------------------------------------------------------
std::vector<const NodeCtx::Iface *> NodeCtx::slaves() const
{
  std::vector<const Iface *> v;
  for(const Iface &i : ifaces_) if(!i.master) v.push_back(&i);
  return v;
}

// --------------------------------------------------------------------
std::vector<const NodeCtx::Iface *> NodeCtx::masters() const
{
  std::vector<const Iface *> v;
  for(const Iface &i : ifaces_) if(i.master) v.push_back(&i);
  return v;
}

// --------------------------------------------------------------------
int NodeCtx::core_data_bits() const
{
  int w = 0;
  for(const Iface &i : ifaces_) {
    if(i.master || !i.ok) continue;
    w = std::max(w, i.sig.data_bits());
  }
  return w > 0 ? w : 32;
}

// --------------------------------------------------------------------
int NodeCtx::mem_data_bits() const
{
  for(const Iface &i : ifaces_) {
    if(i.master && i.ok) return i.sig.data_bits();
  }
  return 0;
}

// --------------------------------------------------------------------
int NodeCtx::refill_beats() const
{
  const int w = mem_data_bits();
  if(w <= 0) return 0;
  return (line_bits() + w - 1) / w;
}

// --------------------------------------------------------------------
void NodeCtx::arrays(const json &st)
{
  struct Pick { const char *key; Arr *slot; };
  const Pick picks[5] = {
    { "tag_array",        &tag_   },
    { "data_array",       &data_  },
    { "valid_bits",       &valid_ },
    { "dirty_bits",       &dirty_ },
    { "replacement_bits", &replb_ }
  };

  for(const Pick &p : picks) {
    if(!st.contains(p.key)) continue;
    const json &a  = st[p.key];
    p.slot->present    = true;
    p.slot->kind       = a.value("kind", std::string("flop_file"));
    p.slot->registered = a.value("read_port", std::string()) ==
                         "registered";
    p.slot->byte_en    = a.value("byte_enables", false);
    p.slot->cleared    = a.value("cleared_on_reset", false);
  }
}

// --------------------------------------------------------------------
bool NodeCtx::build(const Model &m, const Model::Node &n,
                    const json *body,
                    const std::vector<const json *> &link_defs,
                    std::string &why)
{
  name_    = n.name;
  type_    = n.node_type;
  indexing_ = n.indexing;
  geom_    = n.geom;
  pa_bits_ = m.pa_bits;

  if(body == nullptr || !body->is_object()) {
    why = "node '" + n.name + "' has no cache definition body";
    return false;
  }

  if(body->contains("policies")) {
    const json &p = (*body)["policies"];
    read_miss_   = p.value("read_miss",   std::string());
    write_miss_  = p.value("write_miss",  std::string());
    write_hit_   = p.value("write_hit",   std::string());
    repl_policy_ = p.value("replacement", std::string());
  }
  inclusion_ = body->value("inclusion", std::string());

  if(body->contains("fill")) {
    beat_order_ = (*body)["fill"].value("beat_order",
                                        std::string("linear"));
  }
  if(body->contains("timing")) {
    const json &t = (*body)["timing"];
    read_latency_ = t.value("read_latency_cycles", 1);
    tag_stage_    = t.value("tag_compare_stage",
                            std::string("same_cycle"));
  }
  if(body->contains("miss_handling")) {
    mshrs_ = (*body)["miss_handling"].value("mshrs", 0);
  }
  if(body->contains("storage")) arrays((*body)["storage"]);
  range_check_ = body->value("range_check", false);

  // ------------------------------------------------------------------
  // interfaces, in the order the definition declares them so that
  // two emissions of one configuration cannot differ, R-11. A JSON
  // object from nlohmann iterates in key order, which is stable.
  // ------------------------------------------------------------------
  if(!body->contains("interfaces")) {
    why = "node '" + n.name + "' declares no interfaces";
    return false;
  }

  for(auto it = (*body)["interfaces"].begin();
      it != (*body)["interfaces"].end(); ++it) {
    Iface f;
    f.name = it.key();
    f.link = it.value().value("link", std::string());

    // the one port. More than one port on an interface is a shared
    // bus and the emitter reports it rather than guessing an arbiter.
    const json &ports = it.value()["ports"];
    int count = 0;
    for(auto p = ports.begin(); p != ports.end(); ++p) {
      if(count == 0) {
        f.port = p.key();
        const std::string ty = p.value().get<std::string>();
        f.master = ty.size() >= 5 &&
                   ty.compare(ty.size() - 5, 5, "_mstr") == 0;
      }
      ++count;
    }
    if(count != 1) {
      why = "interface '" + f.name + "' on node '" + n.name +
            "' carries " + std::to_string(count) + " ports. The "
            "emitter builds one port per interface, an arbiter for "
            "several is Q-08 and nothing consumes arbitration yet";
      return false;
    }

    // the master flag from the port ROLE, not from the name spelling
    for(const json *ld : link_defs) {
      if(ld == nullptr || !ld->contains(f.link)) continue;
      const json &link = (*ld)[f.link];
      const std::string mt = link.value("master_port_type",
                                        std::string());
      const std::string pt = ports.begin().value().get<std::string>();
      f.master = pt == mt;
      f.ok     = f.sig.build(link, f.why);
      break;
    }

    if(!f.ok && f.why.empty()) {
      f.why = "link '" + f.link + "' was not found";
    }
    ifaces_.push_back(f);
  }

  for(const Iface &f : ifaces_) {
    if(!f.ok) { why = f.why; return false; }
  }

  // ------------------------------------------------------------------
  // A cache fetches and writes back a whole LINE, and the emitter
  // builds that over TileLink, where a size field says how many beats
  // a line is. An ad hoc port carries one word and declares no beat
  // structure, so there is nothing to serialise a line into. That is
  // reported rather than guessed at.
  // ------------------------------------------------------------------
  if(is_cache()) {
    for(const Iface &f : ifaces_) {
      if(!f.master || f.sig.is_tl()) continue;
      why = "interface '" + f.name + "' is the downstream master of "
            "a cache and carries link '" + f.link + "', protocol " +
            f.sig.protocol() + ". A line fill and a writeback need a "
            "beat structure and only TileLink declares one here";
      return false;
    }
  }

  repl_ = std::make_shared<Replacement>(
      repl_policy_.empty() ? std::string("tree_plru") : repl_policy_,
      geom_.associativity > 0 ? geom_.associativity : 1);

  return true;
}

} // namespace cgen
