// --------------------------------------------------------------------
// FILE:    node_ctx.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "node_ctx.h"
#include "field_use.h"
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
// R-6b. One read, recorded against the field it came from. See the
// note on the accessor block in node_ctx.h for why the record is made
// here rather than in build().
// --------------------------------------------------------------------
void NodeCtx::mark(const std::string &rel) const
{
  if(cfg_file_.empty()) return;
  cfg_read(cfg_file_, cfg_path_ + rel);
}

// --------------------------------------------------------------------
const std::string &NodeCtx::indexing() const
{
  mark("/indexing");
  return indexing_;
}

// --------------------------------------------------------------------
const std::string &NodeCtx::read_miss() const
{
  mark("/policies/read_miss");
  return read_miss_;
}

// --------------------------------------------------------------------
const std::string &NodeCtx::write_miss() const
{
  mark("/policies/write_miss");
  return write_miss_;
}

// --------------------------------------------------------------------
const std::string &NodeCtx::write_hit() const
{
  mark("/policies/write_hit");
  return write_hit_;
}

// --------------------------------------------------------------------
const std::string &NodeCtx::inclusion() const
{
  mark("/inclusion");
  return inclusion_;
}

// --------------------------------------------------------------------
const std::string &NodeCtx::beat_order() const
{
  mark("/fill/beat_order");
  return beat_order_;
}

// --------------------------------------------------------------------
bool NodeCtx::has_dirty() const
{
  mark("/policies/write_hit");
  return write_hit_ == "write_back";
}

// --------------------------------------------------------------------
bool NodeCtx::range_check() const
{
  mark("/range_check");
  return range_check_;
}

// --------------------------------------------------------------------
int NodeCtx::mshrs() const
{
  mark("/miss_handling/mshrs");
  return mshrs_;
}

// --------------------------------------------------------------------
int NodeCtx::mshr_targets() const
{
  mark("/miss_handling/mshr_targets");
  return mshr_targets_;
}

// --------------------------------------------------------------------
const NodeCtx::Iface *NodeCtx::core_iface() const
{
  for(const Iface &i : ifaces_) {
    if(!i.master && i.ok) return &i;
  }
  return nullptr;
}

// --------------------------------------------------------------------
// The core link has to declare BOTH halves. Reading only one of them
// would build a miss handling file for a link that cannot address it
// or an identifier path with nothing to keep in flight.
// --------------------------------------------------------------------
bool NodeCtx::nonblocking() const
{
  if(!is_cache()) return false;
  const Iface *i = core_iface();
  if(i == nullptr || i->sig.is_tl()) return false;
  return i->sig.outstanding() > 1 &&
         i->sig.ret_kind() == "valid_with_id" &&
         i->sig.id_bits() > 0;
}

// --------------------------------------------------------------------
// A writing node keeps the blocking control, see the note in the
// header. has_writes and has_dirty are asked in that order so a read
// only node does not mark write_hit for a reason it never had.
// --------------------------------------------------------------------
bool NodeCtx::pipelined() const
{
  if(!nonblocking()) return false;
  if(slaves().size() != 1) return false;
  if(has_writes()) return false;
  return !has_dirty();
}

// --------------------------------------------------------------------
// The compare is in the cycle after the array read was ISSUED when
// tag_compare_stage is next_cycle, and in the same cycle when it is
// same_cycle. Stage 0 is the acceptance.
// --------------------------------------------------------------------
int NodeCtx::cmp_stage() const
{
  return tag_stage() == "next_cycle" ? 1 : 0;
}

// --------------------------------------------------------------------
// The tag array is the one the compare reads. A registered read port
// puts its output one cycle after the set was presented.
// --------------------------------------------------------------------
int NodeCtx::array_stage() const
{
  return registered_read("tag") ? 1 : 0;
}

// --------------------------------------------------------------------
int NodeCtx::min_latency() const
{
  return cmp_stage() + 1;
}

// --------------------------------------------------------------------
int NodeCtx::pipe_pad() const
{
  const int p = read_latency() - min_latency();
  return p > 0 ? p : 0;
}

// --------------------------------------------------------------------
// THE RANGE THE EMITTER SUPPORTS, stated as the three ways a
// declaration falls outside it. Nothing here reads a default: every
// value named comes from the configuration.
// --------------------------------------------------------------------
std::string NodeCtx::timing_why(std::string &field) const
{
  field.clear();

  if(array_stage() > cmp_stage()) {
    field = "tag_compare_stage";
    return "tag_compare_stage '" + tag_stage() + "' puts the compare "
           "in stage " + std::to_string(cmp_stage()) +
           ", and the tag array declares a " + std::string(
             registered_read("tag") ? "registered" : "combinational") +
           " read port whose output is not there until stage " +
           std::to_string(array_stage()) +
           ". A registered read needs tag_compare_stage next_cycle";
  }

  if(read_latency() < min_latency()) {
    field = "read_latency_cycles";
    return "read_latency_cycles " + std::to_string(read_latency()) +
           " is shorter than the array read plus the compare. The "
           "compare is in stage " + std::to_string(cmp_stage()) +
           " and the answer is registered after it, so the shortest "
           "hit this geometry can answer in is " +
           std::to_string(min_latency()) + " cycles";
  }

  if(read_latency() > MaxLatency) {
    field = "read_latency_cycles";
    return "read_latency_cycles " + std::to_string(read_latency()) +
           " is longer than the " + std::to_string(MaxLatency) +
           " the emitter builds. Every cycle beyond " +
           std::to_string(min_latency()) + " is a stage that carries "
           "the answer and does nothing else, and the emitter stops "
           "adding them at " + std::to_string(MaxLatency);
  }

  return std::string();
}

// --------------------------------------------------------------------
int NodeCtx::prefetch_reserve() const
{
  const Iface *i = core_iface();
  if(i == nullptr) return 0;
  const LinkSig::Qual *q = i->sig.qual_of("mshr_reserve");
  return q == nullptr ? 0 : q->reserve;
}

// --------------------------------------------------------------------
std::string NodeCtx::reserve_qual() const
{
  const Iface *i = core_iface();
  if(i == nullptr) return std::string();
  const LinkSig::Qual *q = i->sig.qual_of("mshr_reserve");
  return q == nullptr ? std::string() : q->name;
}

// --------------------------------------------------------------------
int NodeCtx::read_latency() const
{
  mark("/timing/read_latency_cycles");
  return read_latency_;
}

// --------------------------------------------------------------------
const std::string &NodeCtx::tag_stage() const
{
  mark("/timing/tag_compare_stage");
  return tag_stage_;
}

// --------------------------------------------------------------------
const Replacement &NodeCtx::repl() const
{
  mark("/policies/replacement");
  return *repl_;
}

// --------------------------------------------------------------------
// The storage array a name selects, and the JSON key it came from.
// One table, so a new array kind cannot be added to one of the four
// accessors and forgotten in the others.
// --------------------------------------------------------------------
const char *NodeCtx::array_key(const char *which)
{
  const std::string w = which;
  if(w == "tag")   return "tag_array";
  if(w == "data")  return "data_array";
  if(w == "valid") return "valid_bits";
  if(w == "dirty") return "dirty_bits";
  if(w == "repl")  return "replacement_bits";
  return "";
}

// --------------------------------------------------------------------
const std::string &NodeCtx::array_kind(const char *which) const
{
  const std::string w = which;
  const char *k = array_key(which);
  if(*k) mark(std::string("/storage/") + k + "/kind");
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
  const char *k = array_key(which);
  if(*k) mark(std::string("/storage/") + k + "/cleared_on_reset");
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
  const char *k = array_key(which);
  if(*k) mark(std::string("/storage/") + k + "/read_port");
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
  const char *k = array_key(which);
  if(*k) mark(std::string("/storage/") + k + "/byte_enables");
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
                    const std::map<std::string, LinkRef> &link_defs,
                    std::string &why)
{
  cfg_file_ = n.cache_file;
  cfg_path_ = n.cache_path;
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
    mshrs_        = (*body)["miss_handling"].value("mshrs", 0);
    mshr_targets_ = (*body)["miss_handling"].value("mshr_targets", 0);
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
    auto ld = link_defs.find(f.link);
    if(ld != link_defs.end() && ld->second.body != nullptr) {
      const json &link = *ld->second.body;
      const std::string mt = link.value("master_port_type",
                                        std::string());
      const std::string pt = ports.begin().value().get<std::string>();
      f.master = pt == mt;
      f.ok     = f.sig.build(link, f.why, ld->second);
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
