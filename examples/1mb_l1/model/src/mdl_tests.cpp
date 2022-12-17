#include "mdl.h"
using namespace std;
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicWrHitTest(uint32_t &errs,bool verbose)
{
  msg.imsg("Begin basicWrHitTest v="+::to_string(verbose));
  
  errs = 0;
  clearResizeArrays();

  string fn = "../rtl/data/alt_basicWrHit.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) { ++errs; return; }
    
  fn = "../rtl/data/basicWrHit.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) { ++errs; return; }

  fn = "../rtl/data/basicWrHit.dsramN.memh";
  for(size_t i=0;i<4;++i) {
    if(!u.loadRamFromVerilog(dary[i],fn,verbose)) { ++errs; return; }
  }

  //    tag/way index wrd      be  wdata                              
  st(ADDR(0x000,0x000,0x0,0x0),0xF,0x01111111,verbose);//i0 0000 -> 0001
  st(ADDR(0x000,0x000,0x1,0x0),0x1,0x00000022,verbose);//i0 0001
  st(ADDR(0x000,0x000,0x2,0x0),0x2,0x00003300,verbose);//i0 0001
  st(ADDR(0x000,0x000,0x3,0x0),0x4,0x00440000,verbose);//i0 0001
  st(ADDR(0x000,0x000,0x4,0x0),0x8,0x05000000,verbose);//i0 0001
  st(ADDR(0x000,0x000,0x5,0x0),0xF,0x0CACACAC,verbose);//i0 0001
  st(ADDR(0x000,0x000,0x6,0x0),0xF,0x0A4367BC,verbose);//i0 0001
  st(ADDR(0x000,0x000,0x7,0x0),0xF,0x00E1D2C3,verbose);//i0 0001

  //    tag/way index wrd      be  wdata                              
  st(ADDR(0x000,0x001,0x0,0x0),0xF,0x11111111,verbose);//i1 0000 -> 0001
  st(ADDR(0x000,0x001,0x1,0x0),0xF,0x10000022,verbose);//i1
  st(ADDR(0x000,0x001,0x2,0x0),0xF,0x10003300,verbose);//i1
  st(ADDR(0x000,0x001,0x3,0x0),0xF,0x10440000,verbose);//i1
  st(ADDR(0x000,0x001,0x4,0x0),0xF,0x15000000,verbose);//i1
  st(ADDR(0x000,0x001,0x5,0x0),0xF,0x1CACACAC,verbose);//i1
  st(ADDR(0x000,0x001,0x6,0x0),0xF,0x1A4367BC,verbose);//i1
  st(ADDR(0x000,0x001,0x7,0x0),0xF,0x10E1D2C3,verbose);//i1

  //    tag/way index wrd      be  wdata                              
  st(ADDR(0x000,0x002,0x0,0x0),0xF,0x21111111,verbose);//i2 0000 -> 0001
  st(ADDR(0x000,0x002,0x1,0x0),0xF,0x20000022,verbose);//i2 
  st(ADDR(0x000,0x002,0x2,0x0),0xF,0x20003300,verbose);//i2 
  st(ADDR(0x000,0x002,0x3,0x0),0xF,0x20440000,verbose);//i2 
  st(ADDR(0x000,0x002,0x4,0x0),0xF,0x25000000,verbose);//i2 
  st(ADDR(0x000,0x002,0x5,0x0),0xF,0x2CACACAC,verbose);//i2 
  st(ADDR(0x000,0x002,0x6,0x0),0xF,0x2A4367BC,verbose);//i2 
  st(ADDR(0x000,0x002,0x7,0x0),0xF,0x20E1D2C3,verbose);//i2 

  //    tag/way index wrd      be  wdata                              
  st(ADDR(0x003,0x000,0x3,0x0),0x3,0x22221111,verbose); //i0 0001 -> 1001
  st(ADDR(0x001,0x001,0x7,0x0),0xC,0x33339999,verbose); //i1 
  st(ADDR(0x002,0x002,0x6,0x0),0x1,0x99999944,verbose); //i2 
  st(ADDR(0x000,0x003,0x5,0x0),0x4,0x99559999,verbose); //i3 
  st(ADDR(0x000,0x004,0x2,0x0),0x2,0x99996699,verbose); //i4 
  st(ADDR(0x001,0x001,0x1,0x0),0x8,0x77999999,verbose); //i1 
  st(ADDR(0x002,0x005,0x5,0x0),0xF,0x88888888,verbose); //i5 
  st(ADDR(0x003,0x007,0x3,0x0),0xF,0x99999999,verbose); //i7 

  st(ADDR(0x000,0x002,0x2,0x0),0xF,0xaaaaaaaa,verbose); //i2 

  st(ADDR(0x001,0x003,0x1,0x0),0xF,0xbbbbbbbb,verbose); //i3 
  st(ADDR(0x000,0x003,0x6,0x0),0xF,0xcccccccc,verbose); //i3 
  st(ADDR(0x001,0x003,0x7,0x0),0xF,0xdddddddd,verbose); //i3 
  st(ADDR(0x003,0x006,0x4,0x0),0xF,0xeeeeeeee,verbose); //i6 
  st(ADDR(0x000,0x006,0x3,0x0),0xF,0xffffffff,verbose); //i6 

  st(ADDR(0x000,0x002,0x0,0x0),0xF,0x01020304,verbose); //i2 

  st(ADDR(0x001,0x000,0x1,0x0),0xF,0x50607080,verbose); //i0 1001 -> 1011

  // -------------------------------------------------------------------
  //Load the expect data
  // -------------------------------------------------------------------
  //TAGS
  fn = "../rtl/golden/basicWrHit.t.memh";
  if(!u.loadRamFromVerilog(expectTags,fn,verbose)) {
    msg.emsg("Could not load file: "+fn);
    ++errs;
    exit(1); //return; //FIXME: add assert
  }

  //BITS
  fn = "../rtl/golden/alt_basicWrHit.b.memb";
  if(!u.loadRamFromVerilog(expectBits,fn,verbose)) {
    msg.emsg("Could not load file: "+fn);
    ++errs;
    exit(1); //return; //FIXME: add assert
  }

  string baseFn = "../rtl/golden/basicWrHit.d";
  for(size_t i=0;i<4;++i) {
    fn = baseFn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(expectDary[i],fn,verbose)) {
      msg.emsg("Could not load file: "+fn);
      ++errs;
      exit(1); //return; //FIXME: add assert
    }
  }

  // -------------------------------------------------------------------
  // Check
  // -------------------------------------------------------------------
  //Check TAGS 
  if(verbose) msg.imsg("Compare tags");
  u.compare(expectTags,tags,errs,0,16,verbose);

  //Check BITS 
  if(verbose) msg.imsg("Compare bits");
  u.compare(*expectBits,*bits,errs,0,16,verbose);

  //Check dary 
  if(verbose) msg.imsg("Compare dary");
  u.compare(expectDary,dary,errs,0,16,verbose);

  msg.imsg("End   basicWrHitTest");
}

// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicRdHitTest(uint32_t &errs,bool verbose)
{
  msg.imsg("Begin basicRdHitTest v="+::to_string(verbose));

  errs = 0;
  clearResizeArrays();

  string fn = "../rtl/data/alt_basicRdHit.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) { ++errs; return; }

  fn = "../rtl/data/basicRdHit.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) { ++errs; return; }

  string basefn = "../rtl/data/basicRdHit.dsram";
  for(size_t i=0;i<4;++i) {
    fn =  basefn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(dary[i],fn,verbose)) { ++errs; return; }
  }

  captureData.push_back(ld(ADDR(0x003,0x000,0x3,0x0),0xF,verbose));//00303000
  captureData.push_back(ld(ADDR(0x001,0x001,0x7,0x0),0xF,verbose));//00107001
  captureData.push_back(ld(ADDR(0x002,0x002,0x6,0x0),0xF,verbose));//00206002
  captureData.push_back(ld(ADDR(0x000,0x003,0x5,0x0),0xF,verbose));//00005003
  captureData.push_back(ld(ADDR(0x000,0x004,0x2,0x0),0xF,verbose));//00002004
  captureData.push_back(ld(ADDR(0x001,0x001,0x1,0x0),0xF,verbose));//00101001
  captureData.push_back(ld(ADDR(0x002,0x005,0x5,0x0),0xF,verbose));//00205005
  captureData.push_back(ld(ADDR(0x003,0x007,0x3,0x0),0xF,verbose));//00303007
  captureData.push_back(ld(ADDR(0x000,0x002,0x2,0x0),0xF,verbose));//00002002
  captureData.push_back(ld(ADDR(0x001,0x003,0x1,0x0),0xF,verbose));//00101003
  captureData.push_back(ld(ADDR(0x000,0x003,0x6,0x0),0xF,verbose));//00006003
  captureData.push_back(ld(ADDR(0x001,0x003,0x7,0x0),0xF,verbose));//00107003
  captureData.push_back(ld(ADDR(0x003,0x006,0x4,0x0),0xF,verbose));//00304006
  captureData.push_back(ld(ADDR(0x000,0x006,0x3,0x0),0xF,verbose));//00003006
  captureData.push_back(ld(ADDR(0x000,0x002,0x0,0x0),0xF,verbose));//00000002
  captureData.push_back(ld(ADDR(0x001,0x000,0x1,0x0),0xF,verbose));//00101000

  // -------------------------------------------------------------------
  // Load the expect data
  // -------------------------------------------------------------------
  //CAPTURE DATA
  fn = "../rtl/golden/basicRdHit.d.cfg0.memh";
  if(!u.loadCaptureFromVerilog(expectCaptureData,fn,verbose)) ++errs;

  //TAGS
  fn = "../rtl/golden/basicRdHit.t.cfg0.memh";
  if(!u.loadRamFromVerilog(expectTags,fn,verbose)) ++errs;

  //BITS
  fn = "../rtl/golden/alt_basicRdHit.b.cfg0.memb";
  if(!u.loadRamFromVerilog(expectBits,fn,verbose)) ++errs;

  // -------------------------------------------------------------------
  //CHECK
  // -------------------------------------------------------------------
  //Check CAPTURE DATA 
  if(verbose) msg.imsg("Compare capture data");
  if(!u.compare(expectCaptureData,captureData,errs,0,16,verbose)) ++errs;

  //Check TAGS 
  if(verbose) msg.imsg("Compare tags");
  if(!u.compare(expectTags,tags,errs,0,16,verbose)) ++errs;

  //Check BITS 
  if(verbose) msg.imsg("Compare bits");
  if(!u.compare(*expectBits,*bits,errs,0,16,verbose)) ++errs;

  msg.imsg("End   basicRdHitTest");
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicLruTest(uint32_t &errs,bool verbose)
{
  msg.imsg("Begin basicLruTest");

  errs = 0;
  clearResizeArrays();

  string fn = "../rtl/data/basicLru.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) { ++errs; return; }
  //bits->info(cout);

  fn = "../rtl/data/basicLru.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) { ++errs; return; }

  //dary[0]->info(cout,0,16);
  string basefn = "../rtl/data/basicLru.dsram";
  for(size_t i=0;i<4;++i) {
    fn =  basefn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(dary[i],fn,verbose)) { ++errs; return; }
  }

  uint32_t _byte = 0;
  uint32_t word  = 0;
  uint32_t index = 0;

  //(read) access way 3 of index 0    plru 000 -> 110
  ld(ADDR(0x003,index,word,_byte),0xF,verbose);

  //(write) access way 1 of index 0    plru 110 -> 011
  st(ADDR(0x001,index,word,_byte),0xF,0x11111111,verbose);

  //(read) access way 2 of index 0     plru 011 -> 101
  ld(ADDR(0x002,index,word,_byte),0xF,verbose);

  //(write) access way 0 of index 0    plru 101 -> 000
  st(ADDR(0x000,index,word,_byte),0xF,0x22222222,verbose);

  //(read) access way 2 of index 0     plru 000 -> 100
  ld(ADDR(0x002,index,word,_byte),0xF,verbose);

  bitset<3> act_lru(bits->getLru(0));
  bitset<3> exp_lru(0b100);

  stringstream ss;
  ss<<"basicLruTest : exp "<<exp_lru<<" act:"<<act_lru;

  if(act_lru != exp_lru) {
    ++errs;
    msg.emsg(ss.str());
  } else if(verbose) {
    msg.imsg(ss.str());
  }

  msg.imsg("End   basicLruTest");
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicRdAllocTest(uint32_t &errs,bool verbose)
{
  ++errs;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicWrAllocTest(uint32_t &errs,bool verbose)
{
  ++errs;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicRdEvictTest(uint32_t &errs,bool verbose)
{
  ++errs;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicWrEvictTest(uint32_t &errs,bool verbose)
{
  ++errs;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
