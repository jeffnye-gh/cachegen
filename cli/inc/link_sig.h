// --------------------------------------------------------------------
// FILE:    link_sig.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The wire bundle of one link, derived once from the link definition
// and handed to every consumer: the module that drives it, the module
// that receives it, the system top that joins them, and the testbench
// that watches them. D-39, generated not transcribed. Four hand kept
// port lists that happen to agree is not one source.
//
// A bundle is always described from the MASTER's point of view and
// the slave end is the same list with every direction reversed, so
// the two ends of an edge cannot drift apart by construction.
// --------------------------------------------------------------------
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace cgen
{

// --------------------------------------------------------------------
// One link definition and where it sits, so a stage that reads a link
// field can record the read against the file and the JSON pointer it
// came from, R-6b.
// --------------------------------------------------------------------
struct LinkRef {
  const nlohmann::json *body{nullptr};
  std::string           file;   // the links file, as diagnostics name it
  std::string           ptr;    // /links/<name>
};

class LinkSig
{
public:
  // one signal of a bundle
  struct Sig {
    std::string local;      // name inside the bundle, no prefix
    int         bits{1};
    bool        m_drives{true};   // true when the master end drives it
    std::string comment;
  };

  // ------------------------------------------------------------------
  // A REQUESTER SUPPLIED QUALIFIER, one bit the master presents with
  // a request. The bundle carries the wire and the policy says what
  // reading it means, so the rule is emitted rather than reimplemented
  // per node.
  //
  //   mshr_reserve   the request is refused unless `reserve` miss
  //                  handling registers are free. It is the ONLY
  //                  place the bit is read.
  // ------------------------------------------------------------------
  struct Qual {
    std::string name;
    std::string policy;
    int         reserve{0};
  };

  // Build the bundle for one link definition. Returns false and
  // fills why when the definition names something not covered.
  // site is where the definition sits. An empty file means no read
  // is recorded, which is what a unit test of this class wants.
  bool build(const nlohmann::json &link, std::string &why,
             const LinkRef &site = LinkRef());

  const std::vector<Sig> &sigs() const { return sigs_; }

  const std::string &protocol()    const { return protocol_; }
  const std::string &conformance() const { return conformance_; }
  int  data_bits()  const { return data_bits_; }
  int  data_bytes() const { return data_bytes_; }
  int  addr_bits()  const { return addr_bits_; }
  bool is_tl()      const { return protocol_ == "tilelink"; }
  bool has_bce()    const { return bce_; }

  // ------------------------------------------------------------------
  // The custom link's declared shape, for the emitters that have to
  // build different hardware from it. Every one of these is zero or
  // empty on a TileLink link.
  // ------------------------------------------------------------------
  int  read_bits()  const { return read_bits_; }
  int  write_bits() const { return write_bits_; }
  bool read_only()  const { return protocol_ == "custom" &&
                                   write_bits_ == 0; }
  int  id_bits()    const { return id_bits_; }
  int  outstanding() const { return outstanding_; }
  bool err_ret()    const { return err_ret_; }
  bool rsp_ready()  const { return rsp_ready_; }
  const std::string &ret_kind() const { return ret_; }
  const std::vector<Qual> &quals() const { return quals_; }

  // the qualifier carrying one policy, null when the link has none
  const Qual *qual_of(const std::string &policy) const;

  // TL-C carries B, C and E, and this design exercises none of them.
  // The signalling is emitted and tied off, R-5, and this is the
  // list of what was tied off so the report can enumerate it.
  const std::vector<std::string> &tied_off() const { return tied_; }

  // <prefix>_<local>, the one place a bundle signal gets its name
  static std::string wire(const std::string &prefix,
                          const std::string &local);

  // "logic [n-1:0] " or "logic " , the declaration prefix
  static std::string decl(int bits);

  // the range text alone, "[n-1:0]" or empty at one bit
  static std::string range(int bits);

private:
  void tl(const nlohmann::json &tl, const LinkRef &site);
  void tl_channel(const char *ch, bool m_drives, bool mask,
                  bool address, bool sink, bool denied, bool data,
                  int z, int o, int i);
  bool custom(const nlohmann::json &cu, std::string &why,
              const LinkRef &site);

  void add(const std::string &local, int bits, bool m_drives,
           const std::string &comment);

  std::vector<Sig>         sigs_;
  std::vector<std::string> tied_;
  std::vector<Qual>        quals_;
  std::string              protocol_;
  std::string              conformance_;
  std::string              ret_;
  int  data_bits_{0};
  int  data_bytes_{0};
  int  addr_bits_{0};
  int  read_bits_{0};
  int  write_bits_{0};
  int  id_bits_{0};
  int  outstanding_{1};
  bool err_ret_{false};
  bool rsp_ready_{false};
  bool bce_{false};
};

} // namespace cgen
