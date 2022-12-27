#pragma once
#include "addresspacket.h"
#include "utils.h"
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>
#include <map>
// ===========================================================================
typedef uint32_t word_t;
typedef std::vector<word_t> line_t;
#ifndef HEX
#define HEX std::hex<<std::setw(8)<<std::setfill('0')
#endif
// ---------------------------------------------------------------------------
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
  typename std::vector<T>::const_reverse_iterator q;
  std::string sep;
  os << "[";
  for (q = v.rbegin(); q != v.rend(); ++q ) {
    sep = q == v.rbegin() ? "" : ", ";
    os << sep << "0x" << HEX << *q;
  }
  os  << "]\n";
  return  os;
}
// ===========================================================================
struct Ram
{
  Ram(std::string _name,uint32_t _entries,uint32_t _width)
    : name(_name),
      entries(_entries),
      width(_width)
  {}

  size_t size() { return mem.size(); }

  void info(std::ostream&,uint32_t begin=0,uint32_t end=16);

  uint32_t ld(uint32_t idx,uint32_t off);
  line_t ld_line(uint32_t);

  void st(uint32_t idx,uint32_t off,uint32_t be,uint32_t d);
  void st_line(uint32_t idx,line_t &d);

  std::map<uint32_t,line_t>::iterator q;
  std::map<uint32_t,line_t> mem;

  std::string name;
  uint32_t entries;
  uint32_t width;
};
// ===========================================================================
struct Tag
{
  Tag(std::string _name,uint32_t _entries,uint32_t _width)
    : name(_name),
      entries(_entries),
      width(_width)
  {}

  size_t size() { return mem.size(); }

  void info(std::ostream&,uint32_t begin=0,uint32_t end=16);

  uint32_t ld(uint32_t a,uint32_t be);
  void     st(uint32_t a,uint32_t be,uint32_t d);

  std::map<uint32_t,word_t>::iterator q;
  std::map<uint32_t,word_t> mem;

  std::string name;
  uint32_t entries;
  uint32_t width;
};
// ===========================================================================
struct BitArray 
{
  typedef std::map<uint32_t,word_t>::iterator mem_itr_t;
  BitArray(std::string _name,uint32_t _entries,uint32_t _width)
    : name(_name), entries(_entries), width(_width)
  { }

 ~BitArray() {}

  size_t size() { return mem.size(); }

  void info(std::ostream&,std::string msg="",uint32_t begin=0,uint32_t end=16);

  void updateLru(uint32_t idx,uint32_t targetWay);

  void updateMod(AddressPacket&,uint32_t v);
  void updateMod(uint32_t targetWay,uint32_t v);

  void updateVal(AddressPacket&,uint32_t v);
  void updateVal(uint32_t targetWay,uint32_t v);

  //FIXME some hard coded magic numbers in this set of methods
  uint32_t getLru()             { return getLru(q); }
  uint32_t getLru(mem_itr_t &p) { return p->second & 0x7; }
  uint32_t getLru(uint32_t idx) { return mem[idx] & 0x7; } //no checks

  uint32_t getMod()             { return getMod(q); }
  uint32_t getMod(mem_itr_t &p) { return (p->second >> 4) & 0xF; }

  uint32_t getVal()             { return getVal(q); }
  uint32_t getVal(mem_itr_t &p) { return (p->second >> 8) & 0xF; }

  uint32_t getVal(uint32_t idx,uint32_t way);

  uint32_t ld(uint32_t a);
  void     st(uint32_t a,uint32_t d);

  void     setBit(uint32_t a,size_t pos,uint32_t value);
  uint32_t getBit(uint32_t a,size_t pos);

  mem_itr_t q;
  std::map<uint32_t,word_t> mem;

  std::string name;
  uint32_t entries;
  uint32_t width;
};
