#pragma once
#include "msg.h"
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

  bool checkOpts(po::variables_map&);
  bool reportBadOption(std::string,uint64_t);
  bool reportBadOption(std::string,std::string);
  void to_upper(std::string&);

  void usage(po::options_description&);
  void relNotes();
  void version();

  Msg msg;
  po::variables_map vm;

  uint32_t verbose{2};

  bool dry_run;

  //std::vector<CacheParameters> cacheParams; Future supports multi-level

  //Commands
  bool generate{false};
  bool run{false};

  //file io
  std::ofstream transactions;

  //params
  uint64_t l1_capacity;
  uint64_t l1_line_size;
  uint64_t l1_associativity;

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

  uint64_t mm_address_bits;
  uint64_t mm_capacity;
  uint64_t mm_fetch_size;
  uint64_t mm_entries;

  std::string output_file_prefix;
  std::string output_dir;

  //default parameters
  //FIXME add cmd line option 
  uint32_t default_mm_entries{1048576>>2}; //256K
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

  uint32_t l1_offsetBits;
  uint32_t l1_offMsb;
  uint32_t l1_offLsb;
  uint32_t l1_offMask;
  uint32_t l1_offShift;

  uint32_t l1_lru_bits;
  double   l1_tagKB;

  uint32_t mm_lineMask;
  uint32_t mm_lineShift;

  std::string bitsFile;
  std::string tagsFile;
  std::string mmFile;
  std::vector<std::string> daryFiles;


};
