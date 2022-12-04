#include "mdl.h"
#include "utils.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <filesystem>
using namespace std;
// ----------------------------------------------------------------
// ----------------------------------------------------------------
//
//         TTTT TTTT TTTT TTII IIII IIII IIIO OO--
//tag mask 1111 1111 1111 11
//tag mask 11 1111 1111 1111                          3FFF   shift 18
//idx mask                  11 1111 1111 111
//idx mask                  1 1111 1111 1111          1FFF   shift  5
//off mask                                  1 11  
//off mask                                   111      7      shift  2
//
// ----------------------------------------------------------------
void CacheModel::report(uint32_t &errs,uint32_t act,uint32_t exp,bool verbose)
{
  stringstream ss;
  ss<<"exp:0x"<<HEX<<exp<<" act:0x"<<HEX<<act;
  if(act != exp) {
    ++errs;
    ss<<" FAIL";
    msg.emsg(ss.str());
  } else if(verbose) {
    ss<<" PASS";
    msg.imsg(ss.str());
  }
}
// ----------------------------------------------------------------
bool CacheModel::runTests(bool verbose)
{
  if(!runFileChecks()) {
    msg.emsg("Test run aborted, missing files");
    return false;
  }

  msg.imsg("Begin test execution");

  uint32_t basicErrs = 0;
  if(opts.basicTests) {

    uint32_t lruErrs     = 0;
    uint32_t rdHitErrs   = 0;
    uint32_t wrHitErrs   = 0;
    uint32_t rdAllocErrs = 0;
    uint32_t wrAllocErrs = 0;
    uint32_t rdEvictErrs = 0;
    uint32_t wrEvictErrs = 0;

    if(opts.basicLruTest)     basicLruTest(lruErrs,verbose);
    else                      lruErrs += 1000;
    if(opts.basicRdHitTest)   basicRdHitTest(rdHitErrs,true);
    else                      rdHitErrs += 1000;
    if(opts.basicWrHitTest)   basicWrHitTest(wrHitErrs,verbose);
    else                      wrHitErrs += 1000;
    if(opts.basicRdAllocTest) basicRdAllocTest(rdAllocErrs,verbose);
    else                      rdAllocErrs += 1000;
    if(opts.basicWrAllocTest) basicWrAllocTest(wrAllocErrs,verbose);
    else                      wrAllocErrs += 1000;
    if(opts.basicRdEvictTest) basicRdEvictTest(rdEvictErrs,verbose);
    else                      rdEvictErrs += 1000;
    if(opts.basicWrEvictTest) basicWrEvictTest(wrEvictErrs,verbose);
    else                      wrEvictErrs += 1000;

    msg.imsg("==================================================");
    msg.imsg("basic lru errors      : "+::to_string(lruErrs));
    msg.imsg("basic rd hit errors   : "+::to_string(rdHitErrs));
    msg.imsg("basic wr hit errors   : "+::to_string(wrHitErrs));
    msg.imsg("basic rd alloc errors : "+::to_string(rdAllocErrs));
    msg.imsg("basic wr alloc errors : "+::to_string(wrAllocErrs));
    msg.imsg("basic rd evict errors : "+::to_string(rdEvictErrs));
    msg.imsg("basic wr evict errors : "+::to_string(wrEvictErrs));
    msg.imsg("");

    basicErrs += lruErrs + rdHitErrs + wrHitErrs + rdAllocErrs
               + wrAllocErrs + rdEvictErrs + wrEvictErrs;

    if(basicErrs) {
      msg.imsg("Basic tests           : FAIL");
    } else {
      msg.imsg("Basic tests           : PASS");
    }
    msg.imsg("==================================================");
  }

  uint32_t totalErrs = basicErrs;

  if(totalErrs) {
    msg.emsg("Total errors          : "+::to_string(totalErrs));
  } else {
    msg.imsg("Total errors          : "+::to_string(totalErrs));
  }
  msg.imsg("==================================================");

  if(totalErrs) return false;
  return true;
}
// ----------------------------------------------------------------
bool CacheModel::simulate(bool verbose)
{

//  if(!runFileChecks()) {
//    msg.emsg("Simulation run aborted, missing files");
//    return false;
//  }
//
//  msg.imsg("Begin simulation");
//
//  uint32_t errs = 0;
////  uint32_t act,exp;
//
//  if(opts.basicLruTest)     basicLruTest(errs);
//  if(opts.basicRdHitTest)   basicRdHitTest(errs);
//  if(opts.basicWrHitTest)   basicWrHitTest(errs);
//  if(opts.basicRdAllocTest) basicRdAllocTest(errs);
//  if(opts.basicWrAllocTest) basicWrAllocTest(errs);
//  if(opts.basicRdEvictTest) basicRdEvictTest(errs);
//  if(opts.basicWrEvictTest) basicWrEvictTest(errs);


//  //mm addr:00008000  tag/way idx  wrd byt
//  act = ld(ADDR(0x004,0x000,0x3,0x0),0xF,verbose);
//  exp = 0x08000003;
//  report(errs,act,exp,verbose);
//
//  //mm addr:00000009    tag/way idx  wrd byt
//  act = ld(ADDR(0x000,0x009,0x1,0x0),0xF,verbose);
//  exp = 0x00009001;
//  report(errs,act,exp,verbose);
//
//  for(size_t i=0;i<tags.size();++i) {
//    tags[i]->info(cout);
//  }
//
//  if(errs) { 
//    msg.emsg("test failed");
//  }
//  msg.imsg("End simulation");
//  return true;
  return false;
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
