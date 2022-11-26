#include "mdl.h"
#include "msg.h"
#include "ram.h"
#include <fstream>

using namespace std;
// -----------------------------------------------------------------------
CacheModel::CacheModel(int _ac,char **_av)
  : out(opts.transactions)
{
  msg.setWho("cmdl");
  msg.imsg("+CacheModel::CacheModel");
  opts.msg.setWho("cmdl_opt");

  if(!opts.setupOptions(_ac,_av)) {
    throw std::invalid_argument("Option parsing failed");
  }

  tags = new Ram("tags",opts.l1_sets,opts.l1_tagBits);

  for(size_t way;way<opts.l1_associativity;++way) {
    string name = "d"+::to_string(way);
    dary.emplace_back(new Ram(name,opts.l1_sets,opts.l1_line_size*8));
  }

  mm   = new Ram("mm",opts.default_mm_entries,opts.mm_fetch_size*8);

  size_t numBits = opts.l1_associativity //valid
                 + opts.l1_associativity //modified
                 + (opts.l1_lru_bits+1); //pad LRU to 4 bits to match vlg

  bits = new Ram("bits",opts.default_mm_entries,numBits);

}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
bool CacheModel::initializeMM()
{
  string fn = opts.mm_file;
  ifstream in(fn);
  if(!in.is_open()) {
    msg.emsg("Can not open file "+u.tq(fn));
    return false;
  }

  if(!u.loadRamFromVerilog(mm,in)) { return false; }
  in.close();
  return true;
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
uint32_t CacheModel::ld(uint32_t a,uint32_t be) 
{
  u.req_msg(cout,"LOAD :",a,be);

  pckt = AddressPacket(a,be,getTagField(a),getIndexField(a),
                       getOffsetField(a),getMMAddr(a));

  pckt.hit = tagLookup(true);

  uint32_t value;
  if(pckt.hit) value = readHit();
  else         value = readMiss();
  //FIXME: form a logging packet and store to log
  return value;
}
// -----------------------------------------------------------------------
// Address look up combines tag look up and setting the packet control 
// bits
// -----------------------------------------------------------------------
bool CacheModel::tagLookup(bool verbose) 
{
  tags->q = tags->mem.find(pckt.tag);
  if(tags->q == tags->mem.end()) {
    if(verbose) u.tag_msg(cout,"MISS :",pckt.a,pckt.tag);
    return false;
  } 
  return true;
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
uint32_t CacheModel::readHit(bool verbose)
{
  return 0;
}
// -----------------------------------------------------------------------
// if(not all valid) 
//    select 1st invalid way
//    allocate from mm
//      read mm
//      update tags
//      update dary
//      update bits (set valid, clear dirty, update LRU)
//    return critical word
// else select LRU way
//    if(LRU way modified)
//      write LRU way to mm
//    allocate from mm
//      read mm
//      update tags
//      update dary
//      update bits (set valid, clear dirty, update LRU)
//    return critical word
// -----------------------------------------------------------------------
uint32_t CacheModel::readMiss(bool verbose)
{
  //select either by Val or Lru
  int32_t valWaySel = waySelectByVal();
  int32_t lruWaySel = waySelectByLru();

  bool lruSel = valWaySel < 0;
  uint32_t waySel = lruSel ? lruWaySel : (uint32_t) valWaySel;

  //if using LRU check for dirty and write back if needed
  if(lruSel && wayIsMod(waySel)) writeBack(waySel);

  return rdAllocate(waySel);
}
// -----------------------------------------------------------------------
// Allocate from main memory to the index into the targetWay
//
// pckt holds the line address, as well as the tag and index extracted
// from the original request address
// 
// -----------------------------------------------------------------------
uint32_t CacheModel::rdAllocate(uint32_t targetWay)
{
  //read mm and load the dary @ targetWay
  mm->q = mm->mem.find(pckt.mmAddr);

  //FIXME: possibly create data on the fly and update both mm and L1 ?
  //this should not have happened, must abort
  ASSERT(mm->q != mm->mem.end(),"missing main memory entry in rdAllocate()");

  line_t tmp =  mm->q->second;

//
//  dary[targetWay]->st_line(
//  uint32_t idx = pckt.idx;
//  uint32_t tag = readTag(idx,way); //read tags to get value in 'way'
//  
  return 0;
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
//uint32_t CacheModel::readTag(uint32_t idx,uint32_t way)
//{
//  tags->q = tags->mem.find(idx);
//  //this should not have happened, must abort
//  ASSERT(tags->q != tags.end(),"missing index in readTag()");
//
//  uint32_t tagSel = way*4;
//
//  uint32_t tag = (tags >> tagSel);
//  uint32_t mmAddr = (tag & opts.l1_tagMask) << opts.l1_tagShift 
//                  | (idx & opts.l1_idxMask) << opts.l1_idxShift;
//
//}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void CacheModel::writeBack(uint32_t way)
{
//  uint32_t idx = pckt.idx;
//  uint32_t tag = getTag(idx,way);
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
int32_t CacheModel::waySelectByVal()
{
       if(pckt.val[3] == 0) return 3;
  else if(pckt.val[2] == 0) return 2;
  else if(pckt.val[1] == 0) return 1;
  else if(pckt.val[0] == 0) return 0;
  else return -1;
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
int32_t CacheModel::waySelectByLru()
{
  uint32_t sum = pckt.lru[2] + pckt.lru[1] + pckt.lru[0];
  switch(sum) {
    case 0:
    case 1:  return 3;
    case 2:
    case 3:  return 2;
    case 4:
    case 6:  return 1;
    default: return 0;
  }
  return 0;
}
