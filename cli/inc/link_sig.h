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
  std::string              protocol_;
  std::string              conformance_;
  int  data_bits_{0};
  int  data_bytes_{0};
  int  addr_bits_{0};
  bool bce_{false};
};

} // namespace cgen
