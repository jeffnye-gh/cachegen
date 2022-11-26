#include "mdl.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <filesystem>
using namespace std;
// ----------------------------------------------------------------
// ----------------------------------------------------------------
//bool CacheModel::simulate()
//{
// 
//  model = new CacheModel(opts,u);
//
//  if(!model->initializeMM()) {
//    msg.emsg("Model initialization failed");
//    return false;
//  }
//
//  if(!simulate()) {
//    msg.emsg("Model simulation failed");
//    return false;
//  }
// 
//  return true;
//}
// ----------------------------------------------------------------
// FIXME: need wrdMask and wrdShift in options
#define l1_wrdMask  0x3
#define l1_wrdShift 0x2
// ----------------------------------------------------------------
#define ADDR(T,I,W,B) \
    ((T & opts.l1_tagMask ) << opts.l1_tagShift) \
  | ((I & opts.l1_setMask ) << opts.l1_setShift) \
  | ((W &      l1_wrdMask ) <<      l1_wrdShift) \
  | B
// ----------------------------------------------------------------
bool CacheModel::simulate()
{
  msg.imsg("Begin simulation");

  if(!runFileChecks()) {
    msg.emsg("Simulation run aborted, missing files");
    return false;
  }

  //This should return 
//  bitset<14> tag;
//  bitset<13> idx;
//  bitset< 5> wrd;
//  bitset< 2> byt;
//  tag=0x000; idx=0x000; wrd = 0x3; byt=0x0;
  //mm addr:0000009               tag   idx   wrd byt
  uint32_t act = ld(ADDR(0x000,0x009,0x1,0x0),0xF);
  uint32_t exp = 0x00009001;
  if(act != exp) {
    msg.emsg("Actual value / expect value mismatch");
    return false;
  }
  msg.imsg("End simulation");
  return true;
}
// ----------------------------------------------------------------
// Needed files:
// 
// <output dir>/<testcase name>".bits.memb
// <output dir>/<testcase name>".dN.memh    # dN files == #ways
// <output dir>/<testcase name>".mm.memh
// <output dir>/<testcase name>".tags.memh
// ----------------------------------------------------------------
bool CacheModel::runFileChecks()
{
  msg.imsg("Running file checks ");
  string outDir = opts.data_dir;
  string prefix = opts.tc_prefix;

  filesystem::path dirPath = string(outDir);
  bool dirExists = filesystem::is_directory(dirPath);

  if(!dirExists) {
    if(!filesystem::create_directory(dirPath)) {
      msg.emsg("Output directory not found: "+u.tq("./"+outDir));
      return false;
    }
  }

  bool ok = true;

  //bit array
  string fn  = outDir+"/"+prefix+".bits.memb";
  filesystem::path filePath = fn;
  bool fileExists = filesystem::exists(filePath);
  if(!fileExists) {
    ok = false;
    msg.emsg("File not found: "+u.tq(fn));
  }

  opts.bits_file = fn;

  //data arrays
  for(size_t way=0;way<opts.l1_associativity;++way) {
    fn = outDir+"/"+prefix+".d"+::to_string(way)+".memh";

    filesystem::path filePath = fn;
    bool fileExists = filesystem::exists(filePath);
    if(!fileExists) {
      ok = false;
      msg.emsg("File not found: "+u.tq(fn));
    }

    opts.daryFiles.push_back(fn);
  } 

  //tag array
  fn  = outDir+"/"+prefix+".tags.memh";
  filePath = fn;
  fileExists = filesystem::exists(filePath);
  if(!fileExists) {
    ok = false;
    msg.emsg("File not found: "+u.tq(fn));
  }

  opts.tags_file = fn;

  //main memory array
  fn  = outDir+"/"+prefix+".mm.memh";
  filePath = fn;
  fileExists = filesystem::exists(filePath);
  if(!fileExists) {
    ok = false;
    msg.emsg("File not found: "+u.tq(fn));
  }

  opts.mm_file = fn;

  fn = outDir+"/"+prefix+".transactions.v";

  opts.transactions = ofstream(fn);
  if(!opts.transactions.is_open()) {
    msg.emsg("Could not open file "+u.tq(fn));
    ok = false;
  }

  if(ok) msg.imsg("Running file checks complete.");
  else   msg.emsg("Running file checks failed.");
  return ok;
}
