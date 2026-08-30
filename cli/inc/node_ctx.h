// --------------------------------------------------------------------
// FILE:    node_ctx.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// Everything one node's emitters need, gathered once. The node is the
// topology INSTANCE, R-4, so two instances of one cache definition
// get two contexts and two output directories and cannot collide.
//
// Nothing here is read straight out of the input and passed through.
// The geometry comes from Model::Geom, which the tool derived, D-37.
// --------------------------------------------------------------------
#pragma once
#include "link_sig.h"
#include "model.h"
#include "replacement.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace cgen
{

class NodeCtx
{
public:
  // one interface of the node, with the bundle its link carries
  struct Iface {
    std::string name;        // the interface name, core, mem, up_i
    std::string link;        // the link definition it names
    std::string port;        // the one port name inside it
    bool        master{false};  // this node is the master end
    LinkSig     sig;
    std::string why;         // set when the bundle could not be built
    bool        ok{false};
  };

  // build from the resolved model, returns false with why set
  bool build(const Model &m, const Model::Node &n,
             const nlohmann::json *caches_root,
             const std::map<std::string, LinkRef> &link_defs,
             std::string &why);

  // --- identity -----------------------------------------------------
  const std::string &name() const { return name_; }
  const std::string &type() const { return type_; }
  bool is_cache()  const;
  bool is_memory() const { return type_ == "memory"; }
  bool is_agent()  const { return type_ == "agent"; }

  // --- geometry -----------------------------------------------------
  const Model::Geom &geom() const { return geom_; }
  int  pa_bits() const { return pa_bits_; }
  int  line_bits() const { return int(geom_.line_bytes) * 8; }
  int  way_bits() const;
  int  set_idx_bits() const;

  // ------------------------------------------------------------------
  // --- policy -------------------------------------------------------
  //
  // R-6b. EVERY ACCESSOR BELOW RECORDS THE READ, and the record is
  // made HERE rather than where the value was extracted in build().
  //
  // That is the whole mechanism and it is deliberate. Extracting a
  // value into a member is not consuming it: inclusion is extracted
  // and no emitter has ever asked for it, so the configuration says
  // inclusive and the emitted design is the same either way. Marking
  // at extraction would call that field consumed and hide exactly
  // what R-6b exists to surface.
  //
  // The moment an emitter calls one of these, the field leaves the
  // unconsumed report, in the same change that made it live. Nothing
  // has to be remembered and no list has to be maintained.
  // ------------------------------------------------------------------
  const std::string &indexing()    const;
  const std::string &read_miss()   const;
  const std::string &write_miss()  const;
  const std::string &write_hit()   const;
  const std::string &inclusion()   const;
  const std::string &beat_order()  const;
  bool has_writes() const { return type_ != "icache"; }
  bool has_dirty()  const;
  bool range_check() const;

  int  mshrs() const;
  int  mshr_targets() const;
  int  read_latency() const;
  const std::string &tag_stage() const;

  // ------------------------------------------------------------------
  // NON BLOCKING. The core link declares more than one outstanding
  // request and returns a response keyed by an identifier, so the
  // node needs a miss handling file rather than a single busy flag.
  // Both halves are required: outstanding requests with no identifier
  // could only return in order, and an identifier with one
  // outstanding request has nothing to disambiguate.
  // ------------------------------------------------------------------
  bool nonblocking() const;

  // the first slave interface, which is the core port. Null on a
  // node that has none
  const Iface *core_iface() const;

  // how many miss handling registers must stay free for a request
  // carrying the reserve qualifier, and the qualifier's wire name.
  // Zero and empty when the core link declares no such qualifier.
  int prefetch_reserve() const;
  std::string reserve_qual() const;

  const std::string &array_kind(const char *which) const;
  bool cleared_on_reset(const char *which) const;
  bool registered_read(const char *which) const;
  bool byte_enables(const char *which) const;

  // --- interfaces ---------------------------------------------------
  const std::vector<Iface> &ifaces() const { return ifaces_; }
  std::vector<const Iface *> slaves() const;
  std::vector<const Iface *> masters() const;

  // the widest slave data path, and the master data path
  int core_data_bits() const;
  int mem_data_bits()  const;
  int refill_beats()   const;

  const Replacement &repl() const;

  // where this node's cache definition sits, for the R-6b record
  const std::string &cfg_file() const { return cfg_file_; }
  const std::string &cfg_path() const { return cfg_path_; }

  // module names, all derived from the instance name
  std::string mod(const char *suffix = "") const;
  std::string pkg() const { return name_ + "_pkg"; }

private:
  void arrays(const nlohmann::json &st);

  // R-6b, one read against the cache definition this node came from
  void mark(const std::string &rel) const;

  // the storage JSON key one array name selects
  static const char *array_key(const char *which);

  std::string cfg_file_;      // the file the cache definition sits in
  std::string cfg_path_;      // its JSON pointer, /caches/l1i
  std::string name_;
  std::string type_;
  std::string indexing_;
  std::string read_miss_;
  std::string write_miss_;
  std::string write_hit_;
  std::string inclusion_;
  std::string beat_order_;
  std::string tag_stage_;
  std::string repl_policy_;

  struct Arr {
    std::string kind{"flop_file"};
    bool registered{false};
    bool byte_en{false};
    bool cleared{false};
    bool present{false};
  };
  Arr tag_, data_, valid_, dirty_, replb_;

  Model::Geom geom_;
  int  pa_bits_{32};
  int  mshrs_{0};
  int  mshr_targets_{0};
  int  read_latency_{1};
  bool range_check_{false};

  std::vector<Iface> ifaces_;
  std::shared_ptr<Replacement> repl_;
};

} // namespace cgen
