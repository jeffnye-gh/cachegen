// --------------------------------------------------------------------
// FILE:    link_sig.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "link_sig.h"

using nlohmann::json;

namespace cgen
{

namespace {
int as_int(const json &j, const char *key, int dflt)
{
  if(!j.contains(key)) return dflt;
  return j[key].get<int>();
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
bool LinkSig::build(const json &link, std::string &why)
{
  sigs_.clear();
  tied_.clear();

  protocol_ = link.value("protocol", std::string());

  if(protocol_ == "tilelink") {
    if(!link.contains("tilelink")) {
      why = "the link declares protocol tilelink and carries no "
            "tilelink body";
      return false;
    }
    tl(link["tilelink"]);
    return true;
  }

  if(protocol_ == "custom") {
    if(!link.contains("custom")) {
      why = "the link declares protocol custom and carries no custom "
            "body";
      return false;
    }
    return custom(link["custom"], why);
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
void LinkSig::tl(const json &t)
{
  conformance_ = t.value("conformance", std::string("TL-UL"));
  data_bytes_  = as_int(t, "data_bus_bytes", 4);
  data_bits_   = data_bytes_ * 8;
  addr_bits_   = as_int(t, "address_bits", 32);

  const int z = as_int(t, "size_bits",   3);
  const int o = as_int(t, "source_bits", 0);
  const int i = as_int(t, "sink_bits",   0);

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
bool LinkSig::custom(const json &cu, std::string &why)
{
  conformance_ = "custom";
  addr_bits_   = as_int(cu, "address_width_bits", 32);

  const int rw_bits = as_int(cu, "read_width_bits",  32);
  const int ww_bits = as_int(cu, "write_width_bits", 32);
  data_bits_  = rw_bits > ww_bits ? rw_bits : ww_bits;
  data_bytes_ = data_bits_ / 8;

  const json &hs = cu.contains("handshake") ? cu["handshake"]
                                            : json::object();
  const json &ch = cu.contains("channels") ? cu["channels"]
                                           : json::object();

  const std::string strobes = hs.value("request_strobes",
                                       std::string("single_valid_with_rw"));
  const std::string accept  = hs.value("accept", std::string("ready"));
  const std::string ret     = hs.value("read_data_return",
                                       std::string("valid_flag"));
  const std::string addr_ch = ch.value("address", std::string("shared"));
  const std::string data_ch = ch.value("data",
                                       std::string("split_read_write"));

  const int id_bits = as_int(cu, "id_width_bits", 0);

  // ------------------------------------------------------------------
  // request strobes
  // ------------------------------------------------------------------
  if(strobes == "single_valid_with_rw") {
    add("valid", 1, true, "a request is live");
    add("rw",    1, true, "1 write, 0 read");
  } else if(strobes == "separate_read_write") {
    add("rd_valid", 1, true, "a read is live");
    add("wr_valid", 1, true, "a write is live");
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
  // write data and its strobes
  // ------------------------------------------------------------------
  const int gran = as_int(cu, "write_granularity_bytes", 0);
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
    tied_.push_back("the shared bidirectional data channel, emitted "
                    "as a directed write half and a directed read "
                    "half");
  } else {
    why = "data channel '" + data_ch + "' is not covered";
    return false;
  }

  if(cu.value("read_byte_enables", false)) {
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

  if(cu.value("write_response", false)) {
    add("bvalid", 1, false, "the write completed");
  }

  return true;
}

} // namespace cgen
