#pragma once
#include "msg.h"
#include "utils.h"
#include "json/json.h"
#include <boost/program_options.hpp>

#include <fstream>
#include <string>
#include <vector>
namespace po = boost::program_options;
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Future not used atm
//struct CacheParameters
//{
//  uint32_t level;        //1 is next to PE, 100 is main memory
//};
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
struct Options
{
  Options() {}
 ~Options() {}

  bool setupOptions(int,char**);

  void buildOpts(po::options_description&,
                 po::options_description&,
                 po::positional_options_description&);

  bool loadFromJson();
  void info();

  bool checkOpts(po::variables_map&);
  bool reportBadOption(std::string,uint32_t);
  bool reportBadOption(std::string,std::string);
  //void to_upper(std::string&);

  void usage(po::options_description&);
  void relNotes();
  void version();

  Msg msg;
  Utils u;

  po::variables_map vm;

  uint32_t verbose{2};

  bool dry_run;

  //std::vector<CacheParameters> cacheParams; Future supports multi-level

  //Commands
  bool generate{false};
  bool run{false};
  bool load_json{false};

  //file io
  std::ofstream transactions;
  std::string json_format_version{"0.0.1"};
  //
  //params
  uint32_t    l1_capacity;
  uint32_t    l1_capacity_value;
  std::string l1_capacity_units;
  uint32_t    l1_line_size;     //bytes
  uint32_t    l1_associativity;

  //FIXME: add enums
  std::string l1_read_miss_policy;
  std::string l1_write_miss_policy;
  std::string l1_write_hit_policy;
  std::string l1_replacement_policy;
  std::string l1_coherency_protocol;
  uint32_t    l1_victim_buffer_size;
  uint32_t    l1_store_buffer_size;
  std::string l1_tag_type;

  bool        l1_critical_word_first;
  bool        l1_mmu_present;
  bool        l1_mpu_present;
  bool        interactive;    //model input only
  bool        preload_mm;     //model input only
  bool        preload_tags;   //model input only
  bool        preload_bits;   //model input only
  bool        preload_dary;   //model input only

  uint32_t    mm_address_bits;
  uint64_t    mm_capacity;
  uint64_t    mm_capacity_value;
  std::string mm_capacity_units;
  uint32_t mm_fetch_size;
  uint32_t mm_entries;
  uint32_t mm_lineMsb;
  uint32_t mm_lineLsb;
  uint32_t mm_lineShift;
  uint32_t mm_lineMask;

  std::string tc_prefix{""}; //gen output and model input
  std::string data_dir{""};  //gen output and model input
  std::string json_file{""}; //gen output and model input
  std::string cmd_file{""};  //model input

  //default parameters
  //FIXME add cmd line option 
  uint32_t default_mm_entries{1048576>>2}; //1048576>>2}; //256K
  uint32_t l1_word_size{32}; //bits

  //derived parameters
  uint32_t l1_address_bits;

  uint32_t l1_blocks;
  uint32_t l1_blockBits;

  uint32_t l1_assocBits;

  uint32_t l1_tagBits;
  uint32_t l1_tagMsb;
  uint32_t l1_tagLsb;
  uint32_t l1_tagMask;
  uint32_t l1_tagShift;

  uint32_t l1_sets;
  uint32_t l1_setBits;
  uint32_t l1_setMsb;
  uint32_t l1_setLsb;
  uint32_t l1_setMask;
  uint32_t l1_setShift;

  uint32_t l1_offBits;
  uint32_t l1_offMsb;
  uint32_t l1_offLsb;
  uint32_t l1_offMask;
  uint32_t l1_offShift;

  uint32_t l1_wrdShift{2};
  uint32_t l1_wrdMask{7};

  uint32_t l1_lru_bits;
  double   l1_tagKB;

  std::string datasheet;
  std::string bits_file;
  std::string tags_file;
  std::string mm_file;
  std::vector<std::string> daryFiles;

  //test switches
  bool basicTests{false};
  bool basicLruTest{false};
  bool basicRdHitTest{false};
  bool basicWrHitTest{false};
  bool basicRdAllocTest{false};
  bool basicWrAllocTest{false};
  bool basicRdEvictTest{false};
  bool basicWrEvictTest{false};

  Json::Value json;

};
