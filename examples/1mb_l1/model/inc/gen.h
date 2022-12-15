#pragma once
#include "msg.h"
#include "options.h"
#include "utils.h"
#include <string>
#include <ostream>

// ====================================================================
// ====================================================================
struct CacheGen
{
  CacheGen(int,char**);

  void process();

  ValueUnit getValueAndUnits(uint64_t);

  bool run();
  bool generate();
  bool generateMainMemoryData();
  bool generateTags();
  bool generateDary();
  bool generateBits();

  void createDataSheet(std::ostream&);
  bool createJsonFile(std::ostream&);

  Msg msg;
  Utils u;
  Options opts;
};
