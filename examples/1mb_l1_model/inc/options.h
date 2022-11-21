#pragma once
#include "msg.h"
#include <boost/program_options.hpp>

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

  void usage(po::options_description&);
  void relNotes();
  void version();

  Msg msg;
  po::variables_map vm;

  uint32_t verbose{2};

  bool dry_run;

  //std::vector<CacheParameters> cacheParams; Future

  uint64_t l1_capacity;
  uint64_t l1_line_size;
  uint64_t l1_associativity;
  bool     l1_critical_word_first;

  uint64_t mm_address_bits;
  uint64_t mm_capacity;
  uint64_t mm_fetch_size;

  std::string output_file_prefix;
  std::string output_dir;

  //derived parameters
  uint32_t l1_capacityBits;
  uint32_t l1_offsetBits;
  uint32_t l1_blocks;
  uint32_t l1_blockBits;
  uint32_t l1_setBits;
  uint32_t l1_assocBits;
  uint32_t l1_tagBits;
  uint32_t l1_tagMsb;
  uint32_t l1_tagLsb;
  uint32_t l1_setMsb;
  uint32_t l1_setLsb;
  uint32_t l1_offMsb;
  uint32_t l1_offLsb;

  uint32_t mm_capacityBits;


};
