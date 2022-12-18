#pragma once
#include "options.h"
#include "msg.h"
#include "utils.h"
#include "ram.h"
#include <fstream>
struct Ram;
// ====================================================================
// ====================================================================
struct CacheModel
{
  CacheModel(int ac,char** av);
  // ------------------------------------------------------------------
  void initSize(std::vector<Tag*>&,std::string="");
  void initSize(std::vector<Ram*>&,std::string="");
  void initSize(BitArray*,uint32_t,std::string="");

  bool initializeMM();

  void clearResizeArrays();
  void clearResizeMdlArrays();
  void clearResizeExpArrays();
  // ------------------------------------------------------------------
  bool runFileChecks();
  bool runTests(bool verbose=false);
  bool simulate(bool verbose=false);
  // ------------------------------------------------------------------
  // ------------------------------------------------------------------
  uint32_t ld(uint32_t a,uint32_t be,bool verbose=false);
  void     st(uint32_t a,uint32_t be,uint32_t data,bool verbose=false);

  uint64_t ldd(uint64_t a,uint32_t be,bool verbose=false) { return 0; }
  void     std(uint64_t a,uint32_t be,uint64_t data,bool verbose=false) { }
  // ------------------------------------------------------------------
  // Get fields from address
  // ------------------------------------------------------------------
  uint32_t getTagField(uint32_t a)
           { return (a >> opts.l1_tagShift) & opts.l1_tagMask; }
  uint32_t getIndexField(uint32_t a)
           { return (a >> opts.l1_setShift) & opts.l1_setMask; }
  uint32_t getOffsetField(uint32_t a)
           { return (a >> opts.l1_offShift) & opts.l1_offMask; }
  uint32_t getMMAddr(uint32_t a)
           { return (a >> opts.mm_lineShift) & opts.mm_lineMask; }

//  void updateVal(uint32_t way,uint32_t b,uint32_t &in) {
//  }
//
//  void updateMod(uint32_t way,uint32_t b,uint32_t &in) {
//  }
//
//  void updateLru(uint32_t way,uint32_t b,uint32_t &in) {
//  }


  void report(uint32_t&,uint32_t,uint32_t,bool verbose);
  // ------------------------------------------------------------------
  // Get attributes from specified way
  // ------------------------------------------------------------------
  uint32_t getTag(uint32_t idx,uint32_t way);
  // ------------------------------------------------------------------
  std::bitset<4> getValidBits(uint32_t idx);
  std::bitset<4> getModBits(uint32_t idx);
  std::bitset<4> getLruBits(uint32_t idx);

  bool wayIsMod(uint32_t way) { return (pckt.mod[way] == 1); }
  bool wayIsVal(uint32_t way) { return (pckt.val[way] == 1); }

  // ------------------------------------------------------------------
  int32_t waySelectByVal();
  int32_t waySelectByLru();

  uint32_t rdAllocate(uint32_t way);
  void writeBack(uint32_t way);

  void bitsLookup(AddressPacket&,bool verbose=false);
  void tagLookup (AddressPacket&,bool verbose=false);
  // ------------------------------------------------------------------
  uint32_t readHit (bool verbose=false);
  uint32_t readMiss(bool verbose=false);
  // ------------------------------------------------------------------
  void writeHit (uint32_t,bool verbose=false);
  void writeMiss(uint32_t,bool verbose=false);
  // ------------------------------------------------------------------
  void basicLruTest(uint32_t&,   bool verbose=false);
  void basicRdHitTest(uint32_t&, bool verbose=false);
  void basicWrHitTest(uint32_t&, bool verbose=false);
  void basicRdAllocTest(uint32_t&, bool verbose=false);
  void basicWrAllocTest(uint32_t&, bool verbose=false);
  void basicRdEvictTest(uint32_t&, bool verbose=false);
  void basicWrEvictTest(uint32_t&, bool verbose=false);

  void beginTest(std::string);
  void endTest(uint32_t&,std::string);
  // ------------------------------------------------------------------
  BitArray *bits{nullptr};
  std::vector<Tag*> tags{};
  std::vector<Ram*> dary{};
  Ram *mm;

  BitArray *expectBits{nullptr};
  std::vector<Tag*> expectTags{};
  std::vector<Ram*> expectDary{};
  Ram *expectMm{nullptr};

  std::vector<uint32_t> captureData{};
  std::vector<uint32_t> expectCaptureData{};

  //This assumes max one request at a time, all that is needed atm.
  AddressPacket pckt;

  std::ofstream &out;
  Options opts;
  Msg msg;
  Utils u;

};
