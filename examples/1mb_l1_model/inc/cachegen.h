#pragma once
#include "msg.h"
#include "options.h"
#include "utils.h"
#include <string>
#include <ostream>

struct CacheModel;
// ====================================================================
// ====================================================================
struct CacheGen
{
  CacheGen(int,char**);

  void process();

  ValueUnit getValueAndUnits(uint64_t);

  bool execute();

  void createDataSheet(std::ostream&);

  bool generate();
  bool generateMainMemoryData();
  bool generateTags();
  bool generateDary();
  bool generateBits();

  bool run();
  bool runFileChecks();

  bool simulate();

  Msg msg;
  Utils u;
  Options opts;
  CacheModel *model;

  static const std::string vlgSep;
};
