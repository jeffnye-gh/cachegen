#pragma once
#include <fstream>
#include <bitset>
#include <iomanip>
#include <string>

#ifndef HEX
#define HEX std::hex<<std::setw(8)<<std::setfill('0')
#endif
// ====================================================================
// ====================================================================
struct AddressPacket
{
  AddressPacket(uint32_t _a=0,uint32_t _be=0,uint32_t _tag=0,
                uint32_t _idx=0,uint32_t _off=0,uint32_t _mmAddr=0)
    : a(_a),
      be(_be),
      tag(_tag),
      idx(_idx),
      off(_off),
      mmAddr(_mmAddr),
      hit(false),
      val(0),
      mod(0),
      lru(0),
      wayActive(0)
  { }

  void info(std::ostream &out,std::string m="") {
    out<<" AddressPacket info"<<'\n';
    if(m.length() > 0) out<<"   msg:    "<<m<<'\n';
    out<<"   a:      0x"<<HEX<<(a)<<'\n';
    out<<"   be:     0x"<<HEX<<(be)<<'\n';
    out<<"   tag:    0x"<<HEX<<(tag)<<'\n';
    out<<"   idx:    0x"<<HEX<<(idx)<<'\n';
    out<<"   off:    0x"<<HEX<<(off)<<'\n';
    out<<"   mmAddr: 0x"<<HEX<<(mmAddr)<<'\n';
    out<<"   hit:    "  <<hit<<'\n';
    out<<"   val:    0b"<<val.to_string()<<'\n';
    out<<"   mod:    0b"<<mod.to_string()<<'\n';
    out<<"   lru:    0b"<<lru.to_string()<<'\n';
    out<<"   wayAct:   "<<wayActive<<'\n';
  }

  uint32_t a;       // the original request
  uint32_t be;      // the original byte enables
  uint32_t tag;     // extracted tag field
  uint32_t idx;     // extracted index field
  uint32_t off;     // extracted offset field
  uint32_t mmAddr;  // derived from 'a'

  bool hit{false};    // updated as part of the process
//  bool present{false};// updated as part of the process

  std::bitset<4> val; // valid bits updated later
  std::bitset<4> mod; // dirty bits updated later
  std::bitset<4> lru; // dirty bits updated later

  uint32_t wayActive;    // populated in taglookup()
};

