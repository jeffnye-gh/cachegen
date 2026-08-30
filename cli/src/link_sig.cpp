// --------------------------------------------------------------------
// FILE:    link_sig.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "link_sig.h"
#include "field_use.h"

using nlohmann::json;

namespace cgen
{

namespace {

// ------------------------------------------------------------------
// Every read of a link field goes through one of these two, so R-6b
// records the field at the point the value is taken and nowhere else.
// ------------------------------------------------------------------
int as_int(const json &j, const char *key, int dflt,
           const LinkRef &site, const char *group)
{
  if(!j.contains(key)) return dflt;
  if(!site.file.empty()) {
    cfg_read(site.file,
             site.ptr + "/" + group + "/" + key);
  }
  return j[key].get<int>();
}

std::string as_str(const json &j, const char *key,
                   const std::string &dflt,
                   const LinkRef &site, const char *group)
{
  if(!j.contains(key)) return dflt;
  if(!site.file.empty()) {
    cfg_read(site.file, site.ptr + "/" + group + "/" + key);
  }
  return j[key].get<std::string>();
}

bool as_bool(const json &j, const char *key, bool dflt,
             const LinkRef &site, const char *group)
{
  if(!j.contains(key)) return dflt;
  if(!site.file.empty()) {
    cfg_read(site.file, site.ptr + "/" + group + "/" + key);
  }
  return j[key].get<bool>();
}

} // namespace

// --------------------------------------------------------------------
std::string LinkSig::wire(const std::string &prefix,
                          const std::string &local)
{
  return prefix + "_" + local;
}

// --------------------------------------------------------------------
std::string LinkSig::range(int bits)
{
  if(bits <= 1) return "";
  return "[" + std::to_string(bits - 1) + ":0]";
}

// --------------------------------------------------------------------
std::string LinkSig::decl(int bits)
{
  const std::string r = range(bits);
  return r.empty() ? "logic       " : "logic " + r + " ";
}

// --------------------------------------------------------------------
void LinkSig::add(const std::string &local, int bits, bool m_drives,
                  const std::string &comment)
{
  if(bits <= 0) return;      // a zero width field is not a signal
  Sig s;
  s.local    = local;
  s.bits     = bits;
  s.m_drives = m_drives;
  s.comment  = comment;
  sigs_.push_back(s);
}

// --------------------------------------------------------------------
const LinkSig::Qual *LinkSig::qual_of(const std::string &policy) const
{
  for(const Qual &q : quals_) {
    if(q.policy == policy) return &q;
  }
  return nullptr;
}

// --------------------------------------------------------------------
bool LinkSig::build(const json &link, std::string &why,
                    const LinkRef &site)
{
  sigs_.clear();
  tied_.clear();
  quals_.clear();

  protocol_ = link.value("protocol", std::string());
  if(!site.file.empty() && link.contains("protocol")) {
    cfg_read(site.file, site.ptr + "/protocol");
  }

  if(protocol_ == "tilelink") {
    if(!link.contains("tilelink")) {
      why = "the link declares protocol tilelink and carries no "
            "tilelink body";
      return false;
    }
    tl(link["tilelink"], site);
    return true;
  }

  if(protocol_ == "custom") {
    if(!link.contains("custom")) {
      why = "the link declares protocol custom and carries no custom "
            "body";
      return false;
    }
    return custom(link["custom"], why, site);
  }

  why = "protocol '" + protocol_ + "' is not one the emitter covers";
  return false;
}

// --------------------------------------------------------------------
// TileLink 1.9.3. TL-UL and TL-UH carry A and D. TL-C adds B, C and
// E. Channel A and C run master to slave, B and D run slave to
// master, E runs master to slave.
//
// The five per link parameters of Table 4 are w, a, z, o and i. A
// field whose parameter is zero is not a signal, which is why add()
// drops a zero width.
// --------------------------------------------------------------------
void LinkSig::tl(const json &t, const LinkRef &site)
{
  conformance_ = as_str(t, "conformance", "TL-UL", site, "tilelink");
  data_bytes_  = as_int(t, "data_bus_bytes", 4, site, "tilelink");
  data_bits_   = data_bytes_ * 8;
  addr_bits_   = as_int(t, "address_bits", 32, site, "tilelink");

  const int z = as_int(t, "size_bits",   3, site, "tilelink");
  const int o = as_int(t, "source_bits", 0, site, "tilelink");
  const int i = as_int(t, "sink_bits",   0, site, "tilelink");

  bce_ = conformance_ == "TL-C";

  //                ch   master drives  mask  addr  sink  denied data
  tl_channel("a", true,  true,  true,  false, false, true,  z, o, i);
  if(bce_) {
    tl_channel("b", false, true,  true,  false, false, true,  z, o, i);
    tl_channel("c", true,  false, true,  false, false, true,  z, o, i);
  }
  tl_channel("d", false, false, false, true,  true,  true,  z, o, i);
  if(bce_) {
    tl_channel("e", true,  false, false, true,  false, false, z, o, i);
  }

  if(bce_) {
    tied_.push_back("channel B, the slave's probe path");
    tied_.push_back("channel C, the master's probe response and "
                    "release path");
    tied_.push_back("channel E, the master's grant acknowledge");
  }
}

// --------------------------------------------------------------------
// One TileLink channel. valid runs with the payload, ready runs the
// other way. Channel E carries a sink and nothing else.
// --------------------------------------------------------------------
void LinkSig::tl_channel(const char *ch, bool m_drives, bool mask,
                         bool address, bool sink, bool denied,
                         bool data, int z, int o, int i)
{
  const std::string c = ch;
  const std::string C = std::string(1, char(ch[0] - 32));

  add(c + "_valid", 1, m_drives,  "ch " + C + " payload live");
  add(c + "_ready", 1, !m_drives, "ch " + C + " sink accepts");

  if(c == "e") {
    add(c + "_sink", i, m_drives, "the granted sink");
    return;
  }

  add(c + "_opcode", 3, m_drives, "ch " + C + " operation");
  add(c + "_param",  3, m_drives, "op qualifier");
  add(c + "_size",   z, m_drives, "log2 beat bytes");
  add(c + "_source", o, m_drives, "master txn id");
  if(sink)    add(c + "_sink", i, m_drives, "slave txn id");
  if(address) add(c + "_address", addr_bits_, m_drives,
                  "byte address, R-7");
  if(mask)    add(c + "_mask", data_bytes_, m_drives,
                  "one bit a data byte");
  if(denied)  add(c + "_denied", 1, m_drives, "the slave refused");
  if(data)    add(c + "_data", data_bits_, m_drives, "the beat");
  add(c + "_corrupt", 1, m_drives, "the beat is bad");
}

// --------------------------------------------------------------------
// The ad hoc processor port, D-25. The three handshake choices are
// independent and each adds or removes signals on its own.
// --------------------------------------------------------------------
bool LinkSig::custom(const json &cu, std::string &why,
                     const LinkRef &site)
{
  conformance_ = "custom";
  addr_bits_ = as_int(cu, "address_width_bits", 32, site, "custom");

  // ------------------------------------------------------------------
  // A READ ONLY LINK IS write_width_bits 0, or the field absent.
  // TD-L1I-7. The node knowing it is read only was never enough: the
  // LINK has to say so, or the adapter carries a write channel tied
  // off into an unused net and the two disagree about the port list.
  // ------------------------------------------------------------------
  const int rw_bits = as_int(cu, "read_width_bits",  32, site, "custom");
  const int ww_bits = as_int(cu, "write_width_bits",  0, site, "custom");
  read_bits_  = rw_bits;
  write_bits_ = ww_bits;
  const bool rd_only = (ww_bits == 0);
  data_bits_  = rw_bits > ww_bits ? rw_bits : ww_bits;
  data_bytes_ = data_bits_ / 8;

  const json &hs = cu.contains("handshake") ? cu["handshake"]
                                            : json::object();
  const json &ch = cu.contains("channels") ? cu["channels"]
                                           : json::object();

  const std::string strobes =
      as_str(hs, "request_strobes", "single_valid_with_rw", site,
             "custom/handshake");
  const std::string accept =
      as_str(hs, "accept", "ready", site, "custom/handshake");
  const std::string ret =
      as_str(hs, "read_data_return", "valid_flag", site,
             "custom/handshake");
  const std::string addr_ch =
      as_str(ch, "address", "shared", site, "custom/channels");
  const std::string data_ch =
      as_str(ch, "data", "split_read_write", site, "custom/channels");

  const std::string rsp_acc =
      as_str(hs, "response_accept", "none", site, "custom/handshake");

  const int id_bits = as_int(cu, "id_width_bits", 0, site, "custom");
  id_bits_     = id_bits;
  outstanding_ = as_int(cu, "outstanding_requests", 1, site, "custom");
  ret_         = ret;

  // ------------------------------------------------------------------
  // request strobes. A READ ONLY LINK CARRIES NO STROBE AT ALL: a
  // read/write bit would take one constant value, and a separate
  // write valid would name a channel the link does not have.
  // ------------------------------------------------------------------
  if(strobes == "single_valid_with_rw") {
    add("valid", 1, true, "a request is live");
    if(rd_only) {
      tied_.push_back("no read/write strobe, the link declares no "
                      "write channel and every request is a read");
    } else {
      add("rw", 1, true, "1 write, 0 read");
    }
  } else if(strobes == "separate_read_write") {
    add("rd_valid", 1, true, "a read is live");
    if(rd_only) {
      tied_.push_back("no write valid, the link declares no write "
                      "channel");
    } else {
      add("wr_valid", 1, true, "a write is live");
    }
  } else {
    why = "request_strobes '" + strobes + "' is not covered";
    return false;
  }

  // ------------------------------------------------------------------
  // address
  // ------------------------------------------------------------------
  if(addr_ch == "shared") {
    add("addr", addr_bits_, true, "byte address, R-7");
  } else if(addr_ch == "split_read_write") {
    add("raddr", addr_bits_, true, "read address, R-7");
    add("waddr", addr_bits_, true, "write address, R-7");
  } else {
    why = "address channel '" + addr_ch + "' is not covered";
    return false;
  }

  add("id", id_bits, true, "request id");

  // ------------------------------------------------------------------
  // THE REQUESTER SUPPLIED QUALIFIERS, TD-L1I-9. One bit each,
  // presented with the request. The policy travels with the wire so
  // that the rule reading it is emitted from the declaration rather
  // than written once per node.
  // ------------------------------------------------------------------
  if(cu.contains("request_qualifiers")) {
    if(!site.file.empty()) {
      cfg_read(site.file, site.ptr + "/custom/request_qualifiers");
    }
    for(const json &q : cu["request_qualifiers"]) {
      Qual k;
      k.name    = q.value("name", std::string());
      k.policy  = q.value("policy", std::string());
      k.reserve = q.value("reserve", 0);
      if(k.name.empty()) {
        why = "a request qualifier carries no name";
        return false;
      }
      if(k.policy != "mshr_reserve") {
        why = "request qualifier '" + k.name + "' policy '" +
              k.policy + "' is not covered";
        return false;
      }
      quals_.push_back(k);
      add(k.name, 1, true, "requester qualifier, " + k.policy);
    }
  }

  // ------------------------------------------------------------------
  // write data and its strobes
  // ------------------------------------------------------------------
  const int gran = rd_only ? 0
                 : as_int(cu, "write_granularity_bytes", 0, site,
                          "custom");
  const int wstrb = gran > 0 ? (ww_bits / 8) / gran : 0;

  if(data_ch == "split_read_write") {
    add("wdata", ww_bits, true, "write data");
    add("wstrb", wstrb, true, "per writeable unit");
    add("rdata", rw_bits, false, "read data");
  } else if(data_ch == "shared_bidir") {
    // one wire cannot be driven from both ends in synthesizable RTL,
    // so the shared case is emitted as two directed halves and the
    // fact that the configuration called them one is reported.
    add("wdata", ww_bits, true, "write data, master half");
    add("wstrb", wstrb, true, "per writeable unit");
    add("rdata", rw_bits, false, "read data, slave half");
    if(!rd_only) {
      tied_.push_back("the shared bidirectional data channel, emitted "
                      "as a directed write half and a directed read "
                      "half");
    }
  } else {
    why = "data channel '" + data_ch + "' is not covered";
    return false;
  }

  if(as_bool(cu, "read_byte_enables", false, site, "custom")) {
    add("rstrb", (rw_bits / 8), true, "read byte enables");
  }

  // ------------------------------------------------------------------
  // accept
  // ------------------------------------------------------------------
  if(accept == "ready") {
    add("ready", 1, false, "slave took it");
  } else if(accept == "stall") {
    add("stall", 1, false, "slave could not take");
  } else if(accept == "always") {
    tied_.push_back("no accept signal, the slave takes every request "
                    "in the cycle it is offered");
  } else {
    why = "accept '" + accept + "' is not covered";
    return false;
  }

  // ------------------------------------------------------------------
  // read data return
  // ------------------------------------------------------------------
  if(ret == "valid_flag") {
    add("rvalid", 1, false, "read data is live");
  } else if(ret == "valid_with_id") {
    add("rvalid", 1, false, "read data is live");
    add("rid", id_bits, false, "answers this request");
  } else if(ret == "same_cycle") {
    tied_.push_back("no read data valid, read data stands in the "
                    "cycle the request was accepted");
  } else {
    why = "read_data_return '" + ret + "' is not covered";
    return false;
  }

  // ------------------------------------------------------------------
  // THE RESPONSE SIDE HANDSHAKE, TD-IF-3. `accept` above governs the
  // REQUEST side only. Whether the responder can be held off on the
  // response is a separate declaration, and its ABSENCE is as much a
  // decision as its presence: without this field the emitted
  // behaviour was a tool policy nothing in the input could state.
  // ------------------------------------------------------------------
  if(rsp_acc == "ready") {
    rsp_ready_ = true;
    add("rready", 1, true, "the requester took the response");
  } else if(rsp_acc == "none") {
    rsp_ready_ = false;
    tied_.push_back("no response ready, the requester accepts every "
                    "response in the cycle it is presented");
  } else {
    why = "response_accept '" + rsp_acc + "' is not covered";
    return false;
  }

  // ------------------------------------------------------------------
  // THE ERROR RETURN, TD-IF-2. Without it a slave that knows its
  // answer is bad has no wire to say so on, and the adapter drops it.
  // ------------------------------------------------------------------
  err_ret_ = as_bool(cu, "error_response", false, site, "custom");
  if(err_ret_) {
    add("rerr", 1, false, "the response carries an error");
  }

  if(as_bool(cu, "write_response", false, site, "custom")) {
    add("bvalid", 1, false, "the write completed");
  }

  return true;
}

} // namespace cgen
