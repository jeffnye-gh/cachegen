#pragma once
#include "msg.h"
//#include <string>
//#include <fstream>
#include <sstream>
//#include <bitset>
//#include <cstdlib>
#include <algorithm>

// --------------------------------------------------------------------
#define HEX std::hex<<std::setw(8)<<std::setfill('0')
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

  std::string tostr(uint32_t i) { std::stringstream ss; ss<<i; return ss.str(); }
  std::string tostr(uint64_t i) { std::stringstream ss; ss<<i; return ss.str(); }
  std::string tostr(double i)   { std::stringstream ss; ss<<i; return ss.str(); }

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
  bool loadRamFromVerilog(Ram *,std::ifstream&);
  // ------------------------------------------------------------------
  uint32_t hexStrToUint(std::string s);

  void to_upper(std::string &in) {
    std::transform(in.begin(),in.end(),
                   in.begin(),[](char s){ return std::toupper(s); });
  }
  // ------------------------------------------------------------------

  Msg msg;
  static const std::string vlgSep;

};
#undef HEX
