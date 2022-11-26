#include "gen.h"
#include <cmath>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace std;
using namespace std;

// ----------------------------------------------------------------
// ----------------------------------------------------------------
CacheGen::CacheGen(int _ac,char **_av)
{
  msg.imsg("+CacheGen::CacheGen");
  msg.setWho("cgen ");
  opts.msg.setWho("cgen_opt");

  if(!opts.setupOptions(_ac,_av)) {
    throw std::invalid_argument("Option parsing failed");
  }

}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool CacheGen::run()
{
  msg.imsg("+CacheGen::run()");
  if(!opts.dry_run && opts.generate) {
    if(!generate()) return false;
  }

  if(opts.dry_run) msg.imsg("Dry run complete");
  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool CacheGen::generate()
{
  msg.imsg("+CacheGen::generate");
  msg.imsg("Begin generate");

  filesystem::path dirPath = string(opts.data_dir);

  bool dirExists = filesystem::is_directory(dirPath);

  if(!dirExists) {
    if(!filesystem::create_directory(dirPath)) {
      msg.emsg("Can not create data directory: "+u.tq("./"+opts.data_dir));
      return false;
    }
  }

  opts.datasheet = opts.data_dir+"/"+opts.tc_prefix+".datasheet";
  ofstream out(opts.datasheet);
  if(!out.is_open()) {
    msg.emsg("Can not open file: "+u.tq(opts.datasheet));
    return false;
  }

  createDataSheet(out);
  if(!createJsonFile(out)) return false;

  if(!generateMainMemoryData()) return false;
  if(!generateTags()) return false;
  if(!generateDary()) return false;
  if(!generateBits()) return false;
  msg.imsg("End generate");
  return true;
}
// ----------------------------------------------------------------
// Bit arrays are initialized, all entries are invalid, unmodified
// and LRU set to 0. The sub-fields are padded to 4 bits and parsed
// in verilog to extract only the meaningful bits
// ----------------------------------------------------------------
bool CacheGen::generateBits()
{
  msg.imsg("+CacheGen::generateBits");
  opts.bits_file = opts.data_dir+"/"+opts.tc_prefix+".bits.memb";
  msg.imsg("Creating file "+opts.bits_file);
  ofstream out(opts.bits_file);
  
  if(!out.is_open()) {
    msg.emsg("Can not open file: "+u.tq(opts.bits_file));
    return false;
  }

  uint32_t bitEntries = pow(2,opts.l1_setBits);

  out<<u.vlgSep;
  out<<"// Control bit input data file"<<'\n';
  out<<"//   index bits  : "<<opts.l1_setBits<<'\n';
  out<<"//   entries     : "<<bitEntries<<'\n';
  out<<"//   width       : 12\n";
  out<<"//\n";
  out<<"//   LRU sub-field is padded to 4 bit, 0+LRU\n";
  out<<"//   V=valid, D=dirty, L=LRU\n";
  out<<"//   VVVV DDDD 0LLL\n";
  out<<u.vlgSep;

  uint32_t ways = opts.l1_associativity;

  string field = "";
  for(size_t i=0;i<ways;++i) field += "0"; 

  //             valid        dirty         0+lru
  string entry = field +"_" + field + "_" + field;

  uint32_t addrNibbles   = opts.l1_setBits/4;
  for(size_t row=0;row<bitEntries;++row) {
    out<<"@"<<hex<<setfill('0')<<setw(addrNibbles)<<row<<" "<<entry<<'\n';
  }

  out.close();
  return true;
}
// ----------------------------------------------------------------
// Data arrays are not initialized, cache is cold at start of simulation 
// ----------------------------------------------------------------
bool CacheGen::generateDary()
{
  msg.imsg("+CacheGen::generateDary");
  uint32_t ways = opts.l1_associativity;
  uint32_t entryNibbles  = opts.l1_word_size / 4; 
  uint32_t daryEntries   = pow(2,opts.l1_setBits);
  uint32_t addrNibbles   = opts.l1_setBits/4;
  uint32_t wordsPerEntry = opts.l1_line_size / 4;

  stringstream entrySS;
  entrySS<<setw(entryNibbles)<<setfill('x')<<"x";
  string entry = entrySS.str();

  for(size_t i=0;i<ways;++i) {
    string pfx = opts.tc_prefix+".d"+::to_string(i)+".memh";
    string fn = opts.data_dir+"/"+pfx;

    opts.daryFiles.push_back(fn);

    msg.imsg("Creating file "+fn);
    ofstream out(fn);
    if(!out.is_open()) {
      msg.emsg("Can not open file: "+u.tq(fn));
      return false;
    }

    out<<u.vlgSep;
    out<<"// Dary input data file, way "<<i<<'\n';
    out<<"//   index bits  : "<<opts.l1_setBits<<'\n';
    out<<"//   entries     : "<<daryEntries<<'\n';
    out<<u.vlgSep;

    stringstream ss;
    for(size_t row=0;row<daryEntries;++row) {
      ss<<"@"<<hex<<setfill('0')<<setw(addrNibbles)<<row<<" ";

      for(size_t word=0;word<wordsPerEntry;++word) {
        string sep = word == 0 ? "" : "_";
        ss<<sep<<entry;
      }

      out<<ss.str()<<'\n';
      ss.str("");
    }

    out.close();
  }

  return true;
}
// ----------------------------------------------------------------
// Tags are not initialized, cache is cold at start of simulation 
// ----------------------------------------------------------------
bool CacheGen::generateTags()
{
  msg.imsg("+CacheGen::generateTags");
  opts.tags_file = opts.data_dir+"/"+opts.tc_prefix+".tags.memh";

  msg.imsg("Creating file "+opts.tags_file);
  ofstream out(opts.tags_file);
  
  if(!out.is_open()) {
    msg.emsg("Can not open file: "+u.tq(opts.tags_file));
    return false;
  }

  uint32_t ways       = opts.l1_associativity;
  uint32_t tagWidth   = opts.l1_tagBits * ways;
  uint32_t tagEntries = pow(2,opts.l1_setBits);

  out<<u.vlgSep;
  out<<"// Tag input data file\n";
  out<<"//   tag bits    : "<<opts.l1_tagBits<<'\n';
  out<<"//   ways        : "<<ways<<'\n';
  out<<"//   total width : "<<tagWidth<<'\n';
  out<<"//   entries     : "<<tagEntries<<'\n';
  out<<u.vlgSep;

  stringstream wayTagSS;
  uint32_t tagNibbles = (uint32_t) ceil(opts.l1_tagBits/4 + .001);
  wayTagSS<<setw(tagNibbles)<<setfill('x')<<"x";
  string wayTag = wayTagSS.str();

  stringstream ss;
  for(size_t row=0;row<tagEntries;++row) {
    ss<<"@"<<hex<<setfill('0')<<setw(tagNibbles)<<row<<" ";

    for(size_t way=0;way<ways;++way) {
      string sep = way == 0 ? "" : "_";
      ss<<sep<<wayTag;
    }

    out<<ss.str()<<'\n';
    ss.str("");
  }

  out.close();
  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool CacheGen::generateMainMemoryData()
{
  msg.imsg("+CacheGen::generateMainMemoryData");

  opts.mm_file = opts.data_dir+"/"+opts.tc_prefix+".mm.memh";

  msg.imsg("Creating file "+opts.mm_file);
  ofstream out(opts.mm_file);

  if(!out.is_open()) {
    msg.emsg("Can not open file: "+u.tq(opts.mm_file));
    return false;
  }

  out<<u.vlgSep;
  out<<"// Main memory input data file\n";
  out<<"//   address bits: "<<opts.mm_address_bits<<'\n';
  out<<"//   width       : "<<(opts.mm_fetch_size*8)<<'\n';
  out<<"//   entries     : "<<opts.default_mm_entries<<'\n';
  out<<u.vlgSep;

  size_t words = opts.mm_fetch_size/4;
  size_t w = (size_t) ceil(opts.mm_address_bits/4 + .001) - 1;

  stringstream ss;
  for(size_t row=0;row<opts.default_mm_entries;++row) { 

    ss<<"@"<<hex<<setfill('0')<<setw(w)<<row<<" ";

    for(size_t j=0;j<words;++j) {
      uint32_t word = words - 1 - j;
      string sep = j == 0 ? "" : "_";
      ss<<sep<<hex<<setfill('0')<<setw(5)<<row
             <<hex<<setfill('0')<<setw(3)<<word;
    } 

    out<<ss.str()<<'\n';
    ss.str("");
  } 

  out.close();
  return true;
}


// ----------------------------------------------------------------
// ----------------------------------------------------------------
#define DEC dec
#define FLT fixed<<setprecision(3)
void CacheGen::createDataSheet(ostream &out)
{
  msg.imsg("+CacheGen::process()");

  opts.json.clear();
  opts.json["json_format_version"] = opts.json_format_version;
  opts.json["json_file"]           = opts.json_file;
  opts.json["tc_prefix"]           = opts.tc_prefix;
  opts.json["data_dir"]            = opts.data_dir;
  opts.json["cmd_file"]            = opts.cmd_file;

  out<<'\n';

  out<<"L1 cache properties\n";
  ValueUnit l1_cap = u.getValueAndUnits(opts.l1_capacity);

  opts.l1_capacity_value = l1_cap.first;
  opts.l1_capacity_units = l1_cap.second;
 
  out<<"  Capacity      "<<FLT<<l1_cap.first<<" "<<l1_cap.second<<'\n';
  out<<"  Line size     "<<DEC<<opts.l1_line_size<<" bytes\n";
  out<<"  Associativity "<<DEC<<opts.l1_associativity<<" ways\n";

  opts.json["l1_capacity"]       = (Json::Value::UInt64)opts.l1_capacity;
  opts.json["l1_capacity_value"] = (Json::Value::UInt64)l1_cap.first;
  opts.json["l1_capacity_units"] = l1_cap.second;
  opts.json["l1_line_size"]      = opts.l1_line_size;
  opts.json["l1_associativity"]  = opts.l1_associativity;

  out<<'\n';

  stringstream ss;

  out<<"  Read miss          : "<<opts.l1_read_miss_policy<<'\n';
  out<<"  Write miss         : "<<opts.l1_write_miss_policy<<'\n';
  out<<"  Write hit          : "<<opts.l1_write_hit_policy<<'\n';
  out<<"  Replacement        : "<<opts.l1_replacement_policy<<'\n';
  out<<"  Coherency protocol : "<<opts.l1_coherency_protocol<<'\n';
  out<<"  Victim buffer size : "<<opts.l1_victim_buffer_size<<'\n';
  out<<"  Store buffer size  : "<<opts.l1_store_buffer_size<<'\n';
  out<<"  Tag type           : "<<opts.l1_tag_type<<'\n';
  out<<"  Critical word first: "<<opts.l1_critical_word_first<<'\n';
  out<<"  MMU                : "<<opts.l1_mmu_present<<'\n';
  out<<"  MPU                : "<<opts.l1_mpu_present<<'\n';

  opts.json["l1_read_miss_policy"]    = opts.l1_read_miss_policy;
  opts.json["l1_write_miss_policy"]   = opts.l1_write_miss_policy;
  opts.json["l1_write_hit_policy"]    = opts.l1_write_hit_policy;
  opts.json["l1_replacement_policy"]  = opts.l1_replacement_policy;
  opts.json["l1_coherency_protocol"]  = opts.l1_coherency_protocol;
  opts.json["l1_victim_buffer_size"]  = opts.l1_victim_buffer_size;
  opts.json["l1_store_buffer_size"]   = opts.l1_store_buffer_size;
  opts.json["l1_tag_type"]            = opts.l1_tag_type;
  opts.json["l1_critical_word_first"] = opts.l1_critical_word_first;
  opts.json["l1_mmu_present"]         = opts.l1_mmu_present;
  opts.json["l1_mpu_present"]         = opts.l1_mpu_present;

  out<<'\n';

  opts.l1_address_bits = log2(opts.l1_capacity);
  opts.l1_offBits   = log2(opts.l1_line_size);
  opts.l1_blocks       = opts.l1_capacity/opts.l1_line_size;
  opts.l1_blockBits    = log2(opts.l1_blocks);
  opts.l1_setBits      = log2(opts.l1_blocks/opts.l1_associativity);
  opts.l1_assocBits    = log2(opts.l1_associativity);

  opts.json["l1_address_bits"] = opts.l1_address_bits;
  opts.json["l1_offBits"]   = opts.l1_offBits;
  opts.json["l1_blocks"]       = opts.l1_blocks;
  opts.json["l1_blockBits"]    = opts.l1_blockBits;
  opts.json["l1_setBits"]      = opts.l1_setBits;
  opts.json["l1_assocBits"]    = opts.l1_assocBits;

  opts.mm_address_bits = log2(opts.mm_capacity);

  out<<"  Capacity      "<<opts.l1_address_bits<<" bits\n";
  out<<"  Word offset   "<<opts.l1_offBits  <<" bits\n";
  out<<"  Associativity "<<opts.l1_assocBits   <<" bits\n";

  out<<'\n';

  out<<"  #Blocks = Capacity / Block size = 2^"<<opts.l1_address_bits
     <<                                 " / 2^"<<opts.l1_offBits
     <<                                 " = 2^"<<opts.l1_blockBits<<'\n';

  out<<"  #Sets   = #Blocks  / #Ways      = 2^"<<opts.l1_blockBits
     <<                                 " / 2^"<<opts.l1_assocBits
     <<                                 " = 2^"<<opts.l1_setBits<<'\n';

  opts.l1_tagBits
    = opts.mm_address_bits - opts.l1_setBits - opts.l1_offBits;

  opts.l1_tagMsb  = opts.mm_address_bits - 1;
  opts.l1_tagLsb  = opts.l1_tagMsb - opts.l1_tagBits + 1;
  opts.l1_tagShift = opts.l1_tagLsb;
  opts.l1_tagMask  = u.makeMask(opts.l1_tagMsb,opts.l1_tagLsb);

  opts.json["l1_tagMsb"] = opts.l1_tagMsb;
  opts.json["l1_tagLsb"] = opts.l1_tagLsb;
  opts.json["l1_tagShift"] = opts.l1_tagShift;
  opts.json["l1_tagMask"] = opts.l1_tagMask;

  out<<'\n';

  out<<"  With "<<opts.mm_address_bits<<" main memory address bits\n";
  out<<"    tag width    = "<<opts.mm_address_bits
                   <<" - "<<opts.l1_setBits
                   <<" - "<<opts.l1_offBits
                   <<" = "<<opts.l1_tagBits
                   <<" -> ["<<opts.l1_tagMsb<<":"<<opts.l1_tagLsb<<"]\n";
  out<<"    (capacity bits - index bits - offset bits)\n";


  opts.l1_setMsb  = opts.l1_tagLsb - 1;
  opts.l1_setLsb  = opts.l1_setMsb - opts.l1_setBits + 1;
  opts.l1_setShift = opts.l1_setLsb;
  opts.l1_setMask  = u.makeMask(opts.l1_setMsb,opts.l1_setLsb);

  opts.json["l1_setMsb"] = opts.l1_setMsb;
  opts.json["l1_setLsb"] = opts.l1_setLsb;
  opts.json["l1_setShift"] = opts.l1_setShift;
  opts.json["l1_setMask"] = opts.l1_setMask;

  out<<"    set index    = "<<opts.l1_setBits<<" bits "
     <<"         -> ["<<opts.l1_setMsb<<":"<<opts.l1_setLsb<<"]\n";

  opts.l1_offMsb  = opts.l1_setLsb - 1;
  opts.l1_offLsb  = opts.l1_offMsb - opts.l1_offBits + 1;
  opts.l1_offShift = opts.l1_offLsb;
  opts.l1_offMask  = u.makeMask(opts.l1_offMsb,opts.l1_offLsb);

  opts.json["l1_offMsb"]   = opts.l1_offMsb;
  opts.json["l1_offLsb"]   = opts.l1_offLsb;
  opts.json["l1_offShift"] = opts.l1_offShift;
  opts.json["l1_offMask"]  = opts.l1_offMask;

  out<<"    offset width = "<<opts.l1_offBits<<" bits "
     <<"          -> ["<<opts.l1_offMsb<<":"<<opts.l1_offLsb<<"]\n";

  string upper = u.getUpperHeader(31,0);
  string lower = u.getLowerHeader(31,0);

  out<<'\n';

  out<<"    "<<upper<<'\n';
  out<<"    "<<lower<<'\n';

  string vx;
  for(size_t i=0;i<opts.l1_tagBits;++i)      vx += "t";
  for(size_t i=0;i<opts.l1_setBits;++i)      vx += "i";
  for(size_t i=0;i<opts.l1_offBits-2;++i) vx += "o";
  vx += "-- bbbb";

  out<<"    "<<vx<<'\n';

  out<<'\n';

  uint32_t numWays   = opts.l1_associativity;
  uint32_t indexBits = opts.l1_setBits;
  opts.l1_lru_bits = opts.l1_replacement_policy == "PLRU" 
                   ? numWays - 1 : 0;
  opts.l1_sets  = pow(2,opts.l1_setBits)/1024;
  uint32_t sets = opts.l1_sets;

  opts.json["l1_lru_bits"]  = opts.l1_lru_bits;
  opts.json["l1_sets"]  = opts.l1_sets;

  out<<"  Tags      :  "<<sets<<"K x "<<numWays<<" x "<<indexBits<<"b\n";
  out<<"  Valid bits:  "<<sets<<"K x "<<numWays<<" x 1b\n";
  out<<"  Dirty bits:  "<<sets<<"K x "<<numWays<<" x 1b\n";
  out<<"  LRU bits  :  "<<sets<<"K x "<<opts.l1_lru_bits<<"b\n";

  out<<dec<<'\n';

  out<<"  l1_tagBits        "<<opts.l1_tagBits<<'\n';
  out<<"  l1_setBits(index) "<<opts.l1_setBits<<'\n';
  out<<"  l1_offBits        "<<opts.l1_offBits<<'\n';
  out<<"  l1_lru_bits       "<<opts.l1_lru_bits<<'\n';
  out<<"  l1_line_size      "<<opts.l1_line_size<<'\n';

  opts.json["l1_tagBits"]    = opts.l1_tagBits;
  opts.json["l1_setBits"]    = opts.l1_setBits;
  opts.json["l1_offBits"]    = opts.l1_offBits;
  opts.json["l1_lru_bits"]   = opts.l1_lru_bits;
  opts.json["l1_line_size"]  = opts.l1_line_size;

  out<<'\n';

  out<<"  l1_address_bits   "<<opts.l1_address_bits<<'\n';
  out<<"  l1_assocBits      "<<opts.l1_assocBits<<'\n';
  out<<'\n';
  out<<"  l1_tagMsb         "<<opts.l1_tagMsb<<'\n';
  out<<"  l1_tagLsb         "<<opts.l1_tagLsb<<'\n';
  bitset<32> l1_tagMask(opts.l1_tagMask);
  out<<"  l1_tagMask        "<<l1_tagMask<<'\n';
  out<<"  l1_tagShift       "<<opts.l1_tagShift<<'\n';
  out<<'\n';
  out<<"  l1_setMsb         "<<opts.l1_setMsb<<'\n';
  out<<"  l1_setLsb         "<<opts.l1_setLsb<<'\n';
  bitset<32> l1_setMask(opts.l1_setMask);
  out<<"  l1_setMask        "<<l1_setMask<<'\n';
  out<<"  l1_setShift       "<<opts.l1_setShift<<'\n';
  out<<'\n';
  out<<"  l1_offMsb         "<<opts.l1_offMsb<<'\n';
  out<<"  l1_offLsb         "<<opts.l1_offLsb<<'\n';
  bitset<32> l1_offMask(opts.l1_offMask);
  out<<"  l1_offMask        "<<l1_offMask<<'\n';
  out<<"  l1_offShift       "<<opts.l1_offShift<<'\n';
  out<<'\n';
  out<<"  l1_sets           "<<opts.l1_sets<<'\n';
  //out<<"  l1_blocks         "<<opts.l1_blocks<<'\n'; not pertinent
  //out<<"  l1_blockBits      "<<opts.l1_blockBits<<'\n'; confusing
  out<<'\n';
  out<<"  L1 numWays        "<<numWays<<'\n';
  out<<"  L1 indexBits      "<<indexBits<<'\n';
  out<<"  L1 bytes per line "<<indexBits<<'\n';
  out<<'\n';

  opts.mm_lineMsb = opts.mm_address_bits -1;
  opts.mm_lineLsb = opts.l1_setLsb;

  opts.mm_lineShift = opts.l1_setShift;
  opts.mm_lineMask  = u.makeMask(opts.mm_lineMsb,opts.mm_lineLsb);

  bitset<32> mm_lineMask(opts.mm_lineMask);
  out<<"  mm_lineMask       "<<mm_lineMask<<'\n';
  out<<"  mm_lineShift      "<<opts.mm_lineShift<<'\n';

  opts.json["mm_lineMsb"]   = opts.mm_lineMsb;
  opts.json["mm_lineLsb"]   = opts.mm_lineLsb;
  opts.json["mm_lineMask"]  = opts.mm_lineMask;
  opts.json["mm_lineShift"] = opts.mm_lineShift;

  out<<'\n';

  out<<"Main memory properties\n";
  //FIXME add cmdline option
  opts.mm_entries = opts.default_mm_entries;

  ValueUnit mm_cap = u.getValueAndUnits(opts.mm_capacity);
  out<<"  Capacity          "<<FLT<<mm_cap.first<<" "<<mm_cap.second<<'\n';
  out<<"  Populated entries "<<opts.mm_entries<<'\n';
  out<<"  Fetch size        "<<DEC<<opts.mm_fetch_size<<'\n';

  opts.json["mm_entries"]        = opts.mm_entries;
  opts.json["mm_capacity"]       = (Json::Value::UInt64)opts.mm_capacity;
  opts.json["mm_capacity_value"] = (Json::Value::UInt64)mm_cap.first;
  opts.json["mm_capacity_units"] = mm_cap.second;
  opts.json["mm_fetch_size"]     = opts.mm_fetch_size;

  //
  opts.json["bits_file"] = opts.bits_file;
  opts.json["tags_file"] = opts.tags_file;
  opts.json["mm_file"]   = opts.mm_file;

  out<<'\n';
}
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
bool CacheGen::createJsonFile(ostream &out)
{
  opts.json_file = opts.data_dir+"/"+opts.tc_prefix+".json";
  ofstream jsonOut(opts.json_file);

  if(!jsonOut.is_open()) {
    msg.emsg("Could not open file "+u.tq(opts.json_file));
    return false;
  }

  jsonOut<<opts.json<<endl;
  jsonOut.close();
  return true;
}
#undef HEX
#undef FLT
