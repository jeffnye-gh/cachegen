#include "cachegen.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <filesystem>
using namespace std;

const string CacheGen::vlgSep = 
"// ---------------------------------------------------------------\n";
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool CacheGen::generate()
{
  msg.imsg("Begin generate");

  filesystem::path dirPath = string(opts.output_dir);

  bool dirExists = filesystem::is_directory(dirPath);

  if(!dirExists) {
    if(!filesystem::create_directory(dirPath)) {
      msg.emsg("Can not create output directory: "+u.tq("./"+opts.output_dir));
      return false;
    }
  }

  string fn = opts.output_dir+"/"+opts.output_file_prefix+".datasheet";
  ofstream out(fn);
  if(!out.is_open()) {
    msg.emsg("Can not open file: "+u.tq(fn));
    return false;
  }

  createDataSheet(out);

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
  string fn = opts.output_dir+"/"+opts.output_file_prefix+".bits.memb";
  msg.imsg("Creating file "+fn);
  ofstream out(fn);
  
  if(!out.is_open()) {
    msg.emsg("Can not open file: "+u.tq(fn));
    return false;
  }

  uint32_t bitEntries = pow(2,opts.l1_setBits);

  out<<vlgSep;
  out<<"// Control bit input data file"<<'\n';
  out<<"//   index bits  : "<<opts.l1_setBits<<'\n';
  out<<"//   entries     : "<<bitEntries<<'\n';
  out<<"//   width       : 12\n";
  out<<"//\n";
  out<<"//   LRU sub-field is padded to 4 bit, 0+LRU\n";
  out<<"//   V=valid, D=dirty, L=LRU\n";
  out<<"//   VVVV DDDD 0LLL\n";
  out<<vlgSep;

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
  uint32_t ways = opts.l1_associativity;
  uint32_t entryNibbles  = opts.l1_word_size / 4; 
  uint32_t daryEntries   = pow(2,opts.l1_setBits);
  uint32_t addrNibbles   = opts.l1_setBits/4;
  uint32_t wordsPerEntry = opts.l1_line_size / 4;

  stringstream entrySS;
  entrySS<<setw(entryNibbles)<<setfill('x')<<"x";
  string entry = entrySS.str();

  for(size_t i=0;i<ways;++i) {
    string pfx = opts.output_file_prefix+".d"+::to_string(i)+".memh";
    string fn = opts.output_dir+"/"+pfx;

    msg.imsg("Creating file "+fn);
    ofstream out(fn);
    if(!out.is_open()) {
      msg.emsg("Can not open file: "+u.tq(fn));
      return false;
    }

    out<<vlgSep;
    out<<"// Dary input data file, way "<<i<<'\n';
    out<<"//   index bits  : "<<opts.l1_setBits<<'\n';
    out<<"//   entries     : "<<daryEntries<<'\n';
    out<<vlgSep;

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
  string fn = opts.output_dir+"/"+opts.output_file_prefix+".tags.memh";
  msg.imsg("Creating file "+fn);
  ofstream out(fn);
  
  if(!out.is_open()) {
    msg.emsg("Can not open file: "+u.tq(fn));
    return false;
  }

  uint32_t ways       = opts.l1_associativity;
  uint32_t tagWidth   = opts.l1_tagBits * ways;
  uint32_t tagEntries = pow(2,opts.l1_setBits);

  out<<vlgSep;
  out<<"// Tag input data file\n";
  out<<"//   tag bits    : "<<opts.l1_tagBits<<'\n';
  out<<"//   ways        : "<<ways<<'\n';
  out<<"//   total width : "<<tagWidth<<'\n';
  out<<"//   entries     : "<<tagEntries<<'\n';
  out<<vlgSep;

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
  string fn = opts.output_dir+"/"+opts.output_file_prefix+".mm.memh";
  msg.imsg("Creating file "+fn);
  ofstream out(fn);

  if(!out.is_open()) {
    msg.emsg("Can not open file: "+u.tq(fn));
    return false;
  }

  out<<vlgSep;
  out<<"// Main memory input data file\n";
  out<<"//   address bits: "<<opts.mm_address_bits<<'\n';
  out<<"//   width       : "<<(opts.mm_fetch_size*8)<<'\n';
  out<<"//   entries     : "<<opts.default_mm_entries<<'\n';
  out<<vlgSep;

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

