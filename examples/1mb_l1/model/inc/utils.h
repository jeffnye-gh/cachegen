#pragma once
#include "msg.h"
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
// --------------------------------------------------------------------
#ifndef ADDR
#define ADDR(T,I,W,B) \
    ((T & opts.l1_tagMask ) << opts.l1_tagShift) \
  | ((I & opts.l1_setMask ) << opts.l1_setShift) \
  | ((W & opts.l1_offMask ) << opts.l1_offShift) \
  | B
#endif
// --------------------------------------------------------------------
#ifndef HEX
#define HEX std::hex<<std::setw(8)<<std::setfill('0')
#endif
#ifndef HEXN
#define HEXN(N) std::hex<<std::setw(N)<<std::setfill('0')
#endif
// --------------------------------------------------------------------
#ifndef NDEBUG
  #ifndef ASSERT
    #define ASSERT(cond,msg) \
      if(!(cond)) { \
        std::cout<<"-E:"<<(std::string("ASSERT: ")+std::string(msg)) \
                 <<std::endl; \
        exit(1); \
      }
  #endif
#endif
// --------------------------------------------------------------------
struct Ram;
struct Tag;
struct BitArray;
// ====================================================================
// FIXME: Change to std::pair
// ====================================================================
typedef std::pair<uint64_t,std::string> ValueUnit;
//struct ValueUnit
//{
//  double value{0.};
//  std::string units{""};
//};
// ====================================================================
// ====================================================================
struct Utils
{
  Utils() { msg.setWho("utils"); }

  ValueUnit getValueAndUnits(uint64_t v);

  uint32_t makeMask(uint32_t,uint32_t);

  std::string tq(std::string s) { return "'"+s+"'"; }
  std::string getUpperHeader(uint32_t,uint32_t);
  std::string getLowerHeader(uint32_t,uint32_t);

  std::string tostr(uint32_t i)
    { std::stringstream ss; ss<<i; return ss.str(); }
  std::string tostr(uint64_t i)
    { std::stringstream ss; ss<<i; return ss.str(); }
  std::string tostr(double i)
    { std::stringstream ss; ss<<i; return ss.str(); }

  std::string tostr(std::vector<uint32_t>&);

  // ------------------------------------------------------------------
  //bool runFileChecks();
  // ------------------------------------------------------------------
  void req_msg(std::ostream &o,std::string m,uint32_t a,uint32_t be) {
    std::bitset<4>  _be(be);
    std::stringstream ss;
    ss<<m<<" a:0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<a<<" "
          <<"be:0b"<<_be;
    msg.imsg(o,"req:"+ss.str());
  }

  // ------------------------------------------------------------------
  void tag_msg(std::ostream &o,std::string m,uint32_t a,uint32_t tag) {
    std::stringstream ss;
    ss<<m<<" a:0x"<<HEX<<a<<" t:0x"<<HEX<<tag;
    msg.imsg(o,"tag:"+ss.str());
  }
  // ------------------------------------------------------------------
  bool loadRamFromVerilog(Ram*,std::string,bool=false);
  bool loadRamFromVerilog(Ram*,std::ifstream&,bool=false);
  bool loadRamFromVerilog(BitArray*,std::string,bool=false);
  bool loadRamFromVerilog(std::vector<Tag*>&,std::string,bool=false);

  bool loadCaptureFromVerilog(std::vector<uint32_t>&,std::string,bool=false);
  // ------------------------------------------------------------------
  void info(std::ostream &out,std::vector<uint32_t> &vec) {
    uint32_t idx = 0;
    for(auto v : vec) out<<std::dec<<idx++<<" "<<HEX<<v<<std::endl;
  }
  // ------------------------------------------------------------------
  bool sizeChecks(uint32_t &,size_t,size_t);

  bool compare(std::vector<uint32_t> &exp,std::vector<uint32_t> &act,
               uint32_t&,size_t start, size_t end,bool verbose=false);

  bool compare(std::vector<Ram*> &exp,std::vector<Ram*> &act,
               uint32_t&,size_t start, size_t end,bool verbose=false);

  bool compare(std::vector<Tag*> &exp,std::vector<Tag*> &act,
               uint32_t&,size_t start, size_t end,bool verbose=false);

  bool compare(BitArray &exp,BitArray &act,
               uint32_t&,size_t start, size_t end,bool verbose=false);

  bool compare(Ram *exp,Ram *act,uint32_t&,size_t start,size_t end,
               bool verbose=false,int32_t way=-1);

  // ------------------------------------------------------------------
  uint32_t hexStrToUint(std::string s,bool reportX=false);

  void to_upper(std::string &in) {
    std::transform(in.begin(),in.end(),
                   in.begin(),[](char s){ return std::toupper(s); });
  }
  // ------------------------------------------------------------------
  void fileLoadError(std::string fn,uint32_t &errs,bool die=false) {
    msg.emsg("Could not load file: "+fn);
    ++errs;
    if(die) exit(1);
  }
  // ------------------------------------------------------------------

  Msg msg;
  static const std::string vlgSep;

};
