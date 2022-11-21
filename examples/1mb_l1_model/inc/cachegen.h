#pragma once
#include "msg.h"
#include "options.h"
#include <string>

// ====================================================================
// ====================================================================
struct ValueUnit
{
  double value{0.};
  std::string units{""};
};

// ====================================================================
// ====================================================================
struct CacheGen
{
  CacheGen(int,char**);

  void process();

  ValueUnit getValueAndUnits(uint64_t);

  std::string getUpperHeader(uint32_t,uint32_t);
  std::string getLowerHeader(uint32_t,uint32_t);

  Msg msg;
  Options opts;
};
