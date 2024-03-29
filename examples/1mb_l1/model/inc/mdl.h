#pragma once
#include "options.h"
#include "msg.h"
#include "utils.h"
#include "ram.h"
#include <fstream>
struct Ram;
// ====================================================================
// FIXME: some methods are not passed pckt, other's are, standardize
// for all functions which use the pckt member consider making them
// private and do not pass the pckt member as an arg
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

  uint32_t formMMAddress(uint32_t tag,uint32_t idx) {
    uint32_t mmA = ((tag & opts.l1_tagMask) << opts.l1_tagShift)
         | ((idx & opts.l1_setMask) << opts.l1_setShift);
    return mmA;
  }

  void report(uint32_t&,uint32_t,uint32_t,bool verbose);
  // ------------------------------------------------------------------
  // Get attributes from specified way
  // ------------------------------------------------------------------
//  uint32_t getTag(uint32_t idx,uint32_t way) {
//    return tags[way]->mem[idx]; //FIXME: no error checking
//  }
  // ------------------------------------------------------------------
  std::bitset<4> getValidBits(uint32_t idx);
  std::bitset<4> getModBits(uint32_t idx);
  std::bitset<4> getLruBits(uint32_t idx);

  bool wayIsMod(uint32_t way) { return (pckt.mod[way] == 1); }
  bool wayIsVal(uint32_t way) { return (pckt.val[way] == 1); }

  // ------------------------------------------------------------------
  int32_t waySelectByVal();
  int32_t getLruWay() { 
    uint32_t value = (uint32_t) pckt.lru.to_ulong();
    switch(value) {
      case 0:
      case 1: return (int32_t) 3;
      case 2:
      case 3: return (int32_t) 2;
      case 4:
      case 6: return (int32_t) 1;
      case 5:
      case 7: return (int32_t) 0;
      default: return -1; 
    }
  }

  void allocate(uint32_t way,line_t &,bool verbose=false);
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
