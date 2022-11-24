#include "cachegen.h"
#include "cachemodel.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <filesystem>
using namespace std;

// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool CacheGen::run()
{
  msg.imsg("Begin simulation");
  if(!runFileChecks()) {
    msg.emsg("Simulation run aborted, missing files");
    return false;
  }
 
  model = new CacheModel(opts,u);

  if(!simulate()) {
    msg.emsg("Model simulation failed");
    return false;
  }
 
  msg.imsg("End simulation");
  return true;
}
// ----------------------------------------------------------------
bool CacheGen::simulate()
{
  uint32_t value = model->ld(0x00000000,0xF);

  cout<<"HERE value "<<value<<'\n';
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
bool CacheGen::runFileChecks()
{
  msg.imsg("Running file checks ");
  string outDir = opts.output_dir;
  string prefix = opts.output_file_prefix;

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

  opts.bitsFile = fn;

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

  opts.tagsFile = fn;

  //main memory array
  fn  = outDir+"/"+prefix+".mm.memh";
  filePath = fn;
  fileExists = filesystem::exists(filePath);
  if(!fileExists) {
    ok = false;
    msg.emsg("File not found: "+u.tq(fn));
  }

  opts.mmFile = fn;

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
