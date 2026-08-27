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
             const std::vector<const nlohmann::json *> &link_defs,
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

  // --- policy -------------------------------------------------------
  const std::string &indexing()    const { return indexing_; }
  const std::string &read_miss()   const { return read_miss_; }
  const std::string &write_miss()  const { return write_miss_; }
  const std::string &write_hit()   const { return write_hit_; }
  const std::string &inclusion()   const { return inclusion_; }
  const std::string &beat_order()  const { return beat_order_; }
  bool has_writes() const { return type_ != "icache"; }
  bool has_dirty()  const { return write_hit_ == "write_back"; }
  bool range_check() const { return range_check_; }

  int  mshrs() const { return mshrs_; }
  int  read_latency() const { return read_latency_; }
  const std::string &tag_stage() const { return tag_stage_; }

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

  const Replacement &repl() const { return *repl_; }

  // module names, all derived from the instance name
  std::string mod(const char *suffix = "") const;
  std::string pkg() const { return name_ + "_pkg"; }

private:
  void arrays(const nlohmann::json &st);

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
  int  read_latency_{1};
  bool range_check_{false};

  std::vector<Iface> ifaces_;
  std::shared_ptr<Replacement> repl_;
};

} // namespace cgen
