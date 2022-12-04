#include "ram.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cmath>
using namespace std;

#define BIT(A,B) (B >> A) & 0x1
// ------------------------------------------------------------------------
// LRU truth table
//
// access to way0    b2=0  b1=b1  b0=0
// access to way1    b2=0  b1=b1  b0=1  
// access to way2    b2=1  b1=0   b0=b0    
// access to way3    b2=1  b1=1   b0=b0
// ------------------------------------------------------------------------
void BitArray::updateLru(uint32_t targetWay)
{
  uint32_t lru = getLru(q);
  uint32_t newLru=0;
  switch(targetWay) {
    case 0: newLru = (0 << 2) | (BIT(1,lru) << 1) | 0; break;
    case 1: newLru = (0 << 2) | (BIT(1,lru) << 1) | 1; break;
    case 2: newLru = (1 << 2) | (0          << 1) | (BIT(0,lru)); break;
    case 3: newLru = (1 << 2) | (1          << 1) | (BIT(0,lru)); break;
    default: newLru = 0;
  } 
 
  q->second = (getVal() << 8) | (getMod() << 4) | newLru; 
}
// ------------------------------------------------------------------------
void BitArray::updateLru(AddressPacket &pckt)
{
  q = mem.find(pckt.idx);
  updateLru(pckt.wayHit);
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
uint32_t BitArray::getVal(uint32_t idx,uint32_t way)
{
  q = mem.find(idx);
  if(q == mem.end()) return 0;
  //uint32_t v = (q->second >> 8) & 0xF;
  bitset<4> bs((q->second >> 8) & 0xF);
  return bs[way];
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void BitArray::info(ostream &out,string msg,uint32_t first,uint32_t last)
{
  out << "// ----------------------------------------"<<endl;
  out << "// Bits, name : "      << name    <<endl;
  out << "//    entries : " <<dec<< entries <<endl;
  out << "//    width   : " <<dec<< width   <<endl;
  out << "//    msg     : "      << msg     <<endl;
  out << "// ----------------------------------------"<<endl;

  q = mem.find(first);

//  bool scanEntries = false;
  if(q == mem.end()) {
    out<<"// -W: Could not find first address"<<endl;
    //scanEntries = true;
  }

  uint32_t cnt=0,MAX = 4;
  for(const auto &[key,_val] : mem) {
    bitset<4> lru((_val>>0)&0x7);
    bitset<4> mod((_val>>4)&0xF);
    bitset<4> val((_val>>8)&0xF);
    out<<"@"<<hex<<setw(8)<<setfill('0')<<key;
    out<<" "<<val<<"_"<<mod<<"_"<<lru<<endl;
    ++cnt;
    if(cnt >= MAX) break;
  }
}

// ========================================================================
// ========================================================================
void Tag::info(ostream &out,uint32_t first,uint32_t last)
{
  out << "// ----------------------------------------"<<endl;
  out << "// Tag, name : " << name    <<endl;
  out << "//   entries : " <<dec<< entries <<endl;
  out << "//   width   : " <<dec<< width   <<endl;
  out << "// ----------------------------------------"<<endl;
  
//  q = mem.find(first);
//  
//  bool scanEntries = false;
//  if(q == mem.end()) {
//    out<<"// -W: Could not find first address"<<endl;
//    scanEntries = true;
//  } 
//  
//  if(scanEntries) {
    uint32_t cnt = 0;
    uint32_t max = (uint32_t) std::abs((double)(last - first));
    for(const auto &[key,val] : mem) {
      stringstream ss;
      ss<<"@"<<hex<<setw(8)<<setfill('0')<<key;
      ss<<" "<<hex<<setw(8)<<setfill('0')<<val;
      out<<ss.str()<<endl;
      ++cnt;
      if(cnt > max) break;
    } 
    
//  } else {
//  
//    for(q == mem.find(first);q != mem.end();++q) {
//    
//      if(q->first > last) break;
//      stringstream ss;
//      ss<<"@"<<hex<<setw(8)<<setfill('0')<<q->first;
//      
//      line_t v = q->second;
//      reverse(v.begin(),v.end());
//      
//      stringstream ss2;
//      for(size_t i=0;i<v.size();++i) {
//        string sep = i == 0 ? "" : "_";
//        ss2<<sep<<hex<<setw(8)<<setfill('0')<<v[i];
//      } 
//      out<<ss.str()<<" "<<ss2.str()<<endl;
//      
//    }
//  } 
} 
// ========================================================================
// FIXME: add be, byte alignment (right justification)
// ========================================================================
uint32_t Ram::ld(AddressPacket &pckt)
{
  line_t line = ld_line(pckt);
  uint32_t w = line[pckt.off];
  return w;
}
// ------------------------------------------------------------------------
// FIXME: add error checking ?
// ------------------------------------------------------------------------
line_t Ram::ld_line(AddressPacket &pckt)
{
  q = mem.find(pckt.idx);
  return q->second;
}
// ------------------------------------------------------------------------
// FIXME: add error checking ?
// ------------------------------------------------------------------------
void Ram::st(AddressPacket &pckt,uint32_t d)
{
//  line_t line = ld_line(pckt);
//  uint32_t w = line[pckt.off];
//  bitset<32> bw(w);
//  bitset<4> be(pckt.be); 
//  uint32_t mask = 0xFFFFFFFF;
//
//  bitset<32> be0((mask >> (32-8)) <<  0);
//  bitset<32> be1((mask >> (32-8)) <<  8);
//  bitset<32> be2((mask >> (32-8)) << 16);
//  bitset<32> be3((mask >> (32-8)) << 24);
}
// ------------------------------------------------------------------------
// FIXME: add error checking ?
// ------------------------------------------------------------------------
void Ram::st_line(AddressPacket &pckt,line_t &line)
{
  q = mem.find(pckt.idx);
  q->second = line;
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void Ram::info(ostream &out,uint32_t first,uint32_t last)
{
  out << "// ----------------------------------------"<<endl;
  out << "// Ram, name : " << name    <<endl;
  out << "//   entries : " << entries <<endl;
  out << "//   width   : " << width   <<endl;
  out << "// ----------------------------------------"<<endl;

  q = mem.find(first);

  bool scanEntries = false;
  if(q == mem.end()) {
    out<<"// -W: Could not find first address"<<endl;
    scanEntries = true;
  }

  if(scanEntries) {
cout<<"HERE mem.size() "<<mem.size()<<endl;
//    uint32_t diff = last > first ? (last-first) : (first-last);
//    uint32_t cnt = 0;
    for(const auto &[key,val] : mem) {
cout<<"HERE"<<endl;
//      ++cnt;
//      if(cnt > diff) break;
      stringstream ss;
      ss<<"@"<<hex<<setw(8)<<setfill('0')<<key;

      line_t v = val;
      reverse(v.begin(),v.end());

      stringstream ss2;
      for(size_t i=0;i<v.size();++i) {
        string sep = i == 0 ? "" : "_";
        ss2<<sep<<hex<<setw(8)<<setfill('0')<<v[i];
      }
      out<<ss.str()<<" "<<ss2.str()<<endl;
    }

  } else {

    for(q == mem.find(first);q != mem.end();++q) {

      if(q->first > last) break;
      stringstream ss;
      ss<<"@"<<hex<<setw(8)<<setfill('0')<<q->first;

      line_t v = q->second;
      reverse(v.begin(),v.end());

      stringstream ss2;
      for(size_t i=0;i<v.size();++i) {
        string sep = i == 0 ? "" : "_";
        ss2<<sep<<hex<<setw(8)<<setfill('0')<<v[i];
      }
      out<<ss.str()<<" "<<ss2.str()<<endl;

    }
  }
}
