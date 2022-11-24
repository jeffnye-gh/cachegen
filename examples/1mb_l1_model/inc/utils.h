#pragma once
#include "msg.h"
#include <string>
#include <fstream>
#include <bitset>
#include <cstdlib>

#define HEX std::hex<<std::setw(8)<<std::setfill('0')

#ifndef NDEBUG
  #ifndef ASSERT
    #define ASSERT(cond,msg) \
      if(!(cond)) { \
        std::cout<<(std::string("ASSERT: ")+std::string(msg))<<std::endl; \
        exit(1); \
      }
  #endif
#endif
// ====================================================================
// ====================================================================
struct ValueUnit
{
  double value{0.};
  std::string units{""};
};
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

  Msg msg;
};
#undef HEX
