#include "mdl.h"
using namespace std;
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicRdEvictTest(uint32_t &errs,bool verbose)
{
  beginTest("basicRdEvictTest");

  errs = 0;
  clearResizeArrays();

  bool die = true;
  bool v = false;

  string fn = "../rtl/data/basicRdEvict.mm.memh";
  if(!u.loadRamFromVerilog(mm,fn,v)) {
    u.fileLoadError(fn,errs,die);
  }

  fn = "../rtl/data/basicRdEvict.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,v)) {
    u.fileLoadError(fn,errs,die);
  }

  fn = "../rtl/data/basicRdEvict.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,v)) {
    u.fileLoadError(fn,errs,die);
  }

  string baseFn = "../rtl/data/basicRdEvict.d";
  for(size_t i=0;i<4;++i) {
    fn = baseFn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(dary[i],fn,v)) {
      u.fileLoadError(fn,errs,die);
    }
  }
//  mm->info(cout,0,8);

  // -------------------------------------------------------------------
  // -------------------------------------------------------------------
  //a:00008000 #0
  captureData.push_back(ld(ADDR(0x004,0x000,0x3,0x0),0xF,v));
  //a:00002001 #1
  captureData.push_back(ld(ADDR(0x001,0x001,0x7,0x0),0xF,v));
  //a:00004002
  captureData.push_back(ld(ADDR(0x002,0x002,0x6,0x0),0xF,v));
  //a:00006003 #3
  captureData.push_back(ld(ADDR(0x003,0x003,0x6,0x0),0xF,v));
  //a:00006004
  captureData.push_back(ld(ADDR(0x003,0x004,0x5,0x0),0xF,v));
  //a:00002005 #5
  captureData.push_back(ld(ADDR(0x001,0x005,0x1,0x0),0xF,v));
  //a:00004006 #6
  captureData.push_back(ld(ADDR(0x002,0x006,0x5,0x0),0xF,v));
  //a:00006007 #7
  captureData.push_back(ld(ADDR(0x003,0x007,0x3,0x0),0xF,v));
  //a:00002008 #8
  captureData.push_back(ld(ADDR(0x001,0x008,0x2,0x0),0xF,v));
  //a:00000009 #9
  captureData.push_back(ld(ADDR(0x000,0x009,0x1,0x0),0xF,v));
  //a:0000200a #10
  captureData.push_back(ld(ADDR(0x001,0x00a,0x1,0x0),0xF,v));

//  // -------------------------------------------------------------------
//  // Load the expect data
//  // -------------------------------------------------------------------
//  //CAPTURE DATA
//  fn = "../rtl/golden/basicRdEvict.capd.memh";
//  if(!u.loadCaptureFromVerilog(expectCaptureData,fn,v)) {
//    u.fileLoadError(fn,errs,die);
//  }
//
//  //TAGS
//  fn = "../rtl/golden/basicRdEvict.tags.memh";
//  if(!u.loadRamFromVerilog(expectTags,fn,v)) {
//    u.fileLoadError(fn,errs,die);
//  }
//
//  //BITS
//  fn = "../rtl/golden/basicRdEvict.bits.memb";
//  if(!u.loadRamFromVerilog(expectBits,fn,v)) {
//    u.fileLoadError(fn,errs,die);
//  }
//
//  //DARY
//  baseFn = "../rtl/golden/basicRdEvict.d";
//  for(size_t i=0;i<4;++i) {
//    fn = baseFn+::to_string(i)+".memh";
//    if(!u.loadRamFromVerilog(expectDary[i],fn,v)) {
//      u.fileLoadError(fn,errs,die);
//    }
//  }
//
  //Main memory
  fn = "../rtl/golden/basicRdEvict.mm.memh";
  if(!u.loadRamFromVerilog(expectMm,fn,v)) {
    u.fileLoadError(fn,errs,die);
  }
//  mm->info(cout,0,8);
//
//  // -------------------------------------------------------------------
//  //CHECK
//  // -------------------------------------------------------------------
//  //Check CAPTURE DATA 
//  if(v) msg.imsg("Compare capture data");
//  u.compare(expectCaptureData,captureData,errs,0,11,v);
//
//  //Check TAGS 
//  if(v) msg.imsg("Compare tags");
//  u.compare(expectTags,tags,errs,0,16,v);
//
//  //Check BITS 
//  if(v) msg.imsg("Compare bits");
//  u.compare(*expectBits,*bits,errs,0,16,v);
//
//  //Check DARY 
//  if(v) msg.imsg("Compare dary");
//  u.compare(expectDary,dary,errs,0,16,v);
//
  //Check MM 
  if(true) msg.imsg("Compare main memory");
  u.compare(expectMm,mm,errs,0,16,v,-1);

  ++errs;
  endTest(errs,"basicRdEvictTest");
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicWrAllocTest(uint32_t &errs,bool verbose)
{
  beginTest("basicWrAllocTest");

  errs = 0;
  clearResizeArrays();

  bool die = true;

  string fn = "../rtl/data/basicWrAlloc.mm.memh";
  if(!u.loadRamFromVerilog(mm,fn,verbose)) {
    u.fileLoadError(fn,errs,die);
  }

  fn = "../rtl/data/basicWrAlloc.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) {
    u.fileLoadError(fn,errs,die);
  }

  fn = "../rtl/data/basicWrAlloc.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) {
    u.fileLoadError(fn,errs,die);
  }

  string baseFn = "../rtl/data/basicWrAlloc.d";
  for(size_t i=0;i<4;++i) {
    fn = baseFn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(dary[i],fn,verbose)) {
      u.fileLoadError(fn,errs,die);
    }
  }
          //tag/way index   word
  //a:00000000
  st(ADDR(0x000,0x000,0x3,0x0),0xF,0x11111111,verbose);
  //        way   index    tag      val     mod    lru
  //a:00002001
  st(ADDR(0x001,0x001,0x7,0x0),0xF,0x22222222,verbose);
  //a:00004002
  st(ADDR(0x002,0x002,0x6,0x0),0xF,0x23232323,verbose);
  //a:00006003
  st(ADDR(0x003,0x003,0x6,0x0),0xF,0x34353637,verbose);
  //a:00006004
  st(ADDR(0x003,0x004,0x5,0x0),0xF,0x45464748,verbose);
  //a:00002005  way 1 index 5 be 1010
  st(ADDR(0x001,0x005,0x1,0x0),0xA,0xFFFFFFFF,verbose);
  //a:00004006  way 2 index 6 be 0101
  st(ADDR(0x002,0x006,0x5,0x0),0x5,0x77777777,verbose);
  //a:00006007
  st(ADDR(0x003,0x007,0x3,0x0),0xF,0x98979695,verbose);
  //a:00002008
  st(ADDR(0x001,0x008,0x2,0x0),0xF,0xabacadae,verbose);
  //a:00000009
  st(ADDR(0x000,0x009,0x1,0x0),0xF,0xbeefb0da,verbose);
  //a:0000200a
  st(ADDR(0x001,0x00a,0x1,0x0),0xF,0xab109876,verbose);

  // -------------------------------------------------------------------
  //Load the expect data
  // -------------------------------------------------------------------
  //TAGS
  fn = "../rtl/golden/basicWrAlloc.tags.memh";
  if(!u.loadRamFromVerilog(expectTags,fn,verbose)) {
    u.fileLoadError(fn,errs,die);
  }

  //BITS
  fn = "../rtl/golden/basicWrAlloc.bits.memb";
  if(!u.loadRamFromVerilog(expectBits,fn,verbose)) {
    u.fileLoadError(fn,errs,die);
  }

  //Dary
  baseFn = "../rtl/golden/basicWrAlloc.d";
  for(size_t i=0;i<4;++i) {
    fn = baseFn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(expectDary[i],fn,verbose)) {
      u.fileLoadError(fn,errs,die);
    }
  }

  //Main memory
  fn = "../rtl/golden/basicWrAlloc.mm.memh";
  if(!u.loadRamFromVerilog(expectMm,fn,verbose)) {
    u.fileLoadError(fn,errs,die);
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

  //Check MM 
  if(verbose) msg.imsg("Compare main memory");
  u.compare(expectMm,mm,errs,0,16,verbose,-1);
 
  endTest(errs,"basicWrAllocTest");
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicRdAllocTest(uint32_t &errs,bool verbose)
{
  beginTest("basicRdAllocTest");
  
  errs = 0;
  clearResizeArrays();

  bool die = true;

  string fn = "../rtl/data/basicRdAlloc.mm.memh";
  if(!u.loadRamFromVerilog(mm,fn,verbose)) { 
    u.fileLoadError(fn,errs,die); 
  }

  fn = "../rtl/data/basicRdAlloc.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) { 
    u.fileLoadError(fn,errs,die); 
  }

  fn = "../rtl/data/basicRdAlloc.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) { 
    u.fileLoadError(fn,errs,die); 
  }

  string baseFn = "../rtl/data/basicRdAlloc.d";
  for(size_t i=0;i<4;++i) {
    fn = baseFn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(dary[i],fn,verbose)) { 
      u.fileLoadError(fn,errs,die); 
    }
  }
                             //tag/way index   word
  //a:00000000                    0     0 inv             010 -> 0 b1 0    0 1 0
  captureData.push_back(ld(ADDR(0x000,0x000,0x3,0x0),0xF,verbose));//miss
  //a:00002001                    1     1 inv             100 -> 0 b1 1    0 0 1
  captureData.push_back(ld(ADDR(0x001,0x001,0x7,0x0),0xF,verbose));
  //a:00004002                    2     2 inv             000 -> 1 0  b0   1 0 0
  captureData.push_back(ld(ADDR(0x002,0x002,0x6,0x0),0xF,verbose));
  //a:00006003                    3     3 inv             011 -> 1 1  b0   1 1 1
  captureData.push_back(ld(ADDR(0x003,0x003,0x6,0x0),0xF,verbose));
  //a:00006004                    3     4 inv             000 -> 1 1  b0   1 1 0
  captureData.push_back(ld(ADDR(0x003,0x004,0x5,0x0),0xF,verbose));
  //a:00002005                    1     5 inv             000 -> 0 b1 1    0 0 1
  captureData.push_back(ld(ADDR(0x001,0x005,0x1,0x0),0xF,verbose));
  //a:00004006                    2     6 inv             010 -> 1 0  b0   1 0 0
  captureData.push_back(ld(ADDR(0x002,0x006,0x5,0x0),0xF,verbose));
  //a:00006007                    3     7 inv             001 -> 1 1  b0   1 1 1
  captureData.push_back(ld(ADDR(0x003,0x007,0x3,0x0),0xF,verbose));
  //a:00002008                    1     8 inv             111 -> 0 b1 1    0 1 1
  captureData.push_back(ld(ADDR(0x001,0x008,0x2,0x0),0xF,verbose));
  //a:00000009                    0     9 inv             101 ->0 b1 0     0 0 0
  captureData.push_back(ld(ADDR(0x000,0x009,0x1,0x0),0xF,verbose));
  //a:0000200a                    1     a inv             010 -> 0 b1 1    0 1 1
  captureData.push_back(ld(ADDR(0x001,0x00a,0x1,0x0),0xF,verbose));

  // -------------------------------------------------------------------
  //Load the expect data
  // -------------------------------------------------------------------
  //CAPTURE DATA
  fn = "../rtl/golden/basicRdAlloc.capd.memh";
  if(!u.loadCaptureFromVerilog(expectCaptureData,fn,verbose)) {
    u.fileLoadError(fn,errs,die); 
  }

  //TAGS
  fn = "../rtl/golden/basicRdAlloc.tags.memh";
  if(!u.loadRamFromVerilog(expectTags,fn,verbose)) {
    u.fileLoadError(fn,errs,die); 
  }

  //BITS
  fn = "../rtl/golden/basicRdAlloc.bits.memb";
  if(!u.loadRamFromVerilog(expectBits,fn,verbose)) {
    u.fileLoadError(fn,errs,die); 
  }

  //Dary
  baseFn = "../rtl/golden/basicRdAlloc.d";
  for(size_t i=0;i<4;++i) {
    fn = baseFn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(expectDary[i],fn,verbose)) {
      u.fileLoadError(fn,errs,die); 
    }
  }

  //Main memory
  fn = "../rtl/golden/basicRdAlloc.mm.memh";
  if(!u.loadRamFromVerilog(expectMm,fn,verbose)) {
    u.fileLoadError(fn,errs,die); 
  }

  // -------------------------------------------------------------------
  // Check
  // -------------------------------------------------------------------
  //Check CAPTURE DATA 
  if(verbose) msg.imsg("Compare capture data");
  u.compare(expectCaptureData,captureData,errs,0,11,verbose);

  //Check TAGS 
  if(verbose) msg.imsg("Compare tags");
  u.compare(expectTags,tags,errs,0,16,verbose);

  //Check BITS 
  if(verbose) msg.imsg("Compare bits");
  u.compare(*expectBits,*bits,errs,0,16,verbose);

  //Check dary 
  if(verbose) msg.imsg("Compare dary");
  u.compare(expectDary,dary,errs,0,16,verbose);

  //Check MM 
  if(verbose) msg.imsg("Compare main memory");
  u.compare(expectMm,mm,errs,0,16,verbose,-1);

  ++errs;
  endTest(errs,"basicRdAllocTest");
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicWrHitTest(uint32_t &errs,bool verbose)
{
  beginTest("basicWrHitTest");
  
  errs = 0;
  clearResizeArrays();
  bool die = true;

  string fn = "../rtl/data/basicWrHit.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) { 
    u.fileLoadError(fn,errs,die); 
  }
    
  fn = "../rtl/data/basicWrHit.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) { 
    u.fileLoadError(fn,errs,die); 
  }

  fn = "../rtl/data/basicWrHit.dN.memh";
  for(size_t i=0;i<4;++i) {
    if(!u.loadRamFromVerilog(dary[i],fn,verbose)) { 
      u.fileLoadError(fn,errs,die); 
    }
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
  fn = "../rtl/golden/basicWrHit.tags.memh";
  if(!u.loadRamFromVerilog(expectTags,fn,verbose)) {
    u.fileLoadError(fn,errs,die); 
  }

  //BITS
  fn = "../rtl/golden/basicWrHit.bits.memb";
  if(!u.loadRamFromVerilog(expectBits,fn,verbose)) {
    u.fileLoadError(fn,errs,die); 
  }

  string baseFn = "../rtl/golden/basicWrHit.d";
  for(size_t i=0;i<4;++i) {
    fn = baseFn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(expectDary[i],fn,verbose)) {
      u.fileLoadError(fn,errs,die); 
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

  endTest(errs,"basicWrHitTest");
}

// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicRdHitTest(uint32_t &errs,bool verbose)
{
  beginTest("basicRdHitTest");

  errs = 0;
  clearResizeArrays();

  bool die = true;
  string fn = "../rtl/data/basicRdHit.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) { 
    u.fileLoadError(fn,errs,die); 
  }

  fn = "../rtl/data/basicRdHit.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) { 
    u.fileLoadError(fn,errs,die); 
  }

  string basefn = "../rtl/data/basicRdHit.d";
  for(size_t i=0;i<4;++i) {
    fn =  basefn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(dary[i],fn,verbose)) { 
      u.fileLoadError(fn,errs,die); 
    }
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
  fn = "../rtl/golden/basicRdHit.data.memh";
  if(!u.loadCaptureFromVerilog(expectCaptureData,fn,verbose)) {
    u.fileLoadError(fn,errs,die); 
  }

  //TAGS
  fn = "../rtl/golden/basicRdHit.tags.memh";
  if(!u.loadRamFromVerilog(expectTags,fn,verbose)) {
    u.fileLoadError(fn,errs,die); 
  }

  //BITS
  fn = "../rtl/golden/basicRdHit.bits.memb";
  if(!u.loadRamFromVerilog(expectBits,fn,verbose)) {
    u.fileLoadError(fn,errs,die); 
  }

  // -------------------------------------------------------------------
  //CHECK
  // -------------------------------------------------------------------
  //Check CAPTURE DATA 
  if(verbose) msg.imsg("Compare capture data");
  u.compare(expectCaptureData,captureData,errs,0,16,verbose);

  //Check TAGS 
  if(verbose) msg.imsg("Compare tags");
  u.compare(expectTags,tags,errs,0,16,verbose);

  //Check BITS 
  if(verbose) msg.imsg("Compare bits");
  u.compare(*expectBits,*bits,errs,0,16,verbose);

  endTest(errs,"basicRdHitTest");
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicLruTest(uint32_t &errs,bool verbose)
{
  beginTest("basicLruTest");

  errs = 0;
  clearResizeArrays();

  bool die = true;
  string fn = "../rtl/data/basicLru.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) { 
    u.fileLoadError(fn,errs,die); 
  }
  //bits->info(cout);

  fn = "../rtl/data/basicLru.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) { 
    u.fileLoadError(fn,errs,die); 
  }

  //dary[0]->info(cout,0,16);
  string basefn = "../rtl/data/basicLru.d";
  for(size_t i=0;i<4;++i) {
    fn =  basefn+::to_string(i)+".memh";
    if(!u.loadRamFromVerilog(dary[i],fn,verbose)) { 
      u.fileLoadError(fn,errs,die); 
    }
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

  endTest(errs,"basicLruTest");
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicWrEvictTest(uint32_t &errs,bool verbose)
{
  beginTest("basicWrEvictTest");
  ++errs;
  endTest(errs,"basicWrEvictTest");
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::beginTest(std::string t) {
  msg.imsg("BEGIN TEST  : "+t);
}
// ----------------------------------------------------------------
void CacheModel::endTest(uint32_t &errs, std::string t)
{
  stringstream ss;
  ss<<"END TEST    : ";
  ss<<setw(16)<<setfill(' ')<<left<<t;
  ss<<" : errors ";
  ss<<setw(2)<<errs<<" : ";

  if(errs) {
    ss << " FAIL";
    msg.emsg(ss.str());
  } else {
    ss << " PASS";
    msg.imsg(ss.str());
  }
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
