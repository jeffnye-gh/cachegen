#pragma once
#include "options.h"
#include "msg.h"
#include "utils.h"
#include <fstream>
struct Ram;

// ====================================================================
// ====================================================================
struct AddressPacket
{
  AddressPacket(uint32_t _a=0,uint32_t _be=0,uint32_t _tag=0,
                uint32_t _idx=0,uint32_t _off=0,uint32_t _mmAddr=0)
    : a(_a),
      be(_be),
      tag(_tag),
      idx(_idx),
      off(_off),
      mmAddr(_mmAddr),
      hit(false),
      val(0),
      mod(0),
      lru(0)
  { }

  uint32_t a;   // the original request
  uint32_t be;  // the original byte enables
  uint32_t tag; // extracted tag field
  uint32_t idx; // extracted index field
  uint32_t off; // extracted offset field
  uint32_t mmAddr;

  bool hit{false};    // updated as part of the process
  std::bitset<4> val; // valid bits updated later
  std::bitset<4> mod; // dirty bits updated later
  std::bitset<4> lru; // dirty bits updated later

};
// ====================================================================
// ====================================================================
struct CacheModel
{
  CacheModel(int ac,char** av);

  // ------------------------------------------------------------------
  bool initializeMM();
  bool runFileChecks();
  bool simulate();
  // ------------------------------------------------------------------
  // ------------------------------------------------------------------
  uint32_t ld(uint32_t a,uint32_t be);
  void     st(uint32_t a,uint32_t be,uint32_t data) { }

  uint64_t ldd(uint64_t a,uint32_t be) { return 0; }
  void     std(uint64_t a,uint32_t be,uint64_t data) { }
  // ------------------------------------------------------------------
  //void req_msg(std::ostream&,uint32_t,uint32_t);
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
  bool tagLookup(bool verbose=false);
  // ------------------------------------------------------------------
  uint32_t readHit (bool verbose=false);
  uint32_t readMiss(bool verbose=false);
  // ------------------------------------------------------------------
  Ram *bits;
  Ram *tags;
  std::vector<Ram*> dary;
  Ram *mm;

  //This assumes max one request at a time, all that is needed atm.
  AddressPacket pckt;

  std::ofstream &out;
  Options opts;
  Msg msg;
  Utils u;

};
