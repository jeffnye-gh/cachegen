#include "mdl.h"
using namespace std;

// ----------------------------------------------------------------
// ----------------------------------------------------------------
void CacheModel::basicRdHitTest(uint32_t &errs,bool verbose)
{
  msg.imsg("Begin basicRdHitTest v="+::to_string(verbose));

  errs = 0;

  string fn = "../1mb_l1/data/alt_basicRdHit.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) { ++errs; return; }

  fn = "../1mb_l1/data/basicRdHit.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) { ++errs; return; }

  string basefn = "../1mb_l1/data/basicRdHit.dsram";
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
  //Load all the expect data - pre-size the arrays
  // -------------------------------------------------------------------
  initSize(expectTags,"expTags");

  //FIXME: this core dumps on insert, I do not know why yet
  //initSize(expectBits,bits->width,"expBits");
  size_t numBits = opts.l1_associativity //valid
                 + opts.l1_associativity //modified
                 + (opts.l1_lru_bits+1); //pad LRU to 4 bits to match vlg

  expectBits = new BitArray("expBits",opts.default_mm_entries,numBits);

  //CAPTURE DATA
  fn = "../1mb_l1/golden/basicRdHit.d.cfg0.memh";
  if(!u.loadCaptureFromVerilog(expectCaptureData,fn,verbose)) ++errs;

  //TAGS
  fn = "../1mb_l1/golden/basicRdHit.t.cfg0.memh";
  if(!u.loadRamFromVerilog(expectTags,fn,verbose)) ++errs;

  //BITS
  fn = "../1mb_l1/golden/alt_basicRdHit.b.cfg0.memb";
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
  string fn = "../1mb_l1/data/basicLru.bits.memb";
  if(!u.loadRamFromVerilog(bits,fn,verbose)) { ++errs; return; }
  //bits->info(cout);

  fn = "../1mb_l1/data/basicLru.tags.memh";
  if(!u.loadRamFromVerilog(tags,fn,verbose)) { ++errs; return; }

  string basefn = "../1mb_l1/data/basicLru.dsram";
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
//// ----------------------------------------------------------------
//// ----------------------------------------------------------------
//void CacheModel::basicRdHitTest(uint32_t &errs)
//{
//}
//// ----------------------------------------------------------------
//// ----------------------------------------------------------------
//void CacheModel::basicWrHitTest(uint32_t &errs)
//{
//}
//// ----------------------------------------------------------------
//// ----------------------------------------------------------------
//void CacheModel::basicRdAllocTest(uint32_t &errs)
//{
//}
//// ----------------------------------------------------------------
//// ----------------------------------------------------------------
//void CacheModel::basicWrAllocTest(uint32_t &errs)
//{
//}
//// ----------------------------------------------------------------
//// ----------------------------------------------------------------
//void CacheModel::basicRdEvictTest(uint32_t &errs)
//{
//}
//// ----------------------------------------------------------------
//// ----------------------------------------------------------------
//void CacheModel::basicWrEvictTest(uint32_t &errs)
//{
//}
//// ----------------------------------------------------------------
//// ----------------------------------------------------------------
//
//
