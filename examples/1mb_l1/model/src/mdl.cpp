#include "mdl.h"
#include "msg.h"
#include "ram.h"
#include <fstream>

using namespace std;
// -----------------------------------------------------------------------
CacheModel::CacheModel(int _ac,char **_av)
  : out(opts.transactions)
{
  msg.setWho("cmdl ");
  msg.imsg("+CacheModel::CacheModel");
  opts.msg.setWho("cmdl_opt");


  if(!opts.setupOptions(_ac,_av)) {
    throw std::invalid_argument("Option parsing failed");
  }

//  size_t numBits = opts.l1_associativity //valid
//                 + opts.l1_associativity //modified
//                 + (opts.l1_lru_bits+1); //pad LRU to 4 bits to match vlg
//
//  bits = new BitArray("bits",opts.default_mm_entries,numBits);
//
//  if(opts.preload_mm) {
//    //If this fails sim can not continue
//    ASSERT(initializeMM(),"main memory init failed");
//  }
//
//  initSize(bits,numBits,"bits");

  clearResizeMdlArrays();

//  initSize(tags,"tags");
//  initSize(dary,"dary");
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
uint32_t CacheModel::ld(uint32_t a,uint32_t be,bool verbose) 
{
  if(verbose) u.req_msg(cout,"LOAD  :",a,be);

  pckt = AddressPacket(a,be,getTagField(a),getIndexField(a),
                            getOffsetField(a),getMMAddr(a));

//  //HERE
//  pckt.info(cout);

  bitsLookup(pckt,verbose);
  tagLookup(pckt,verbose);

  //if(verbose) pckt.info(cout);

  uint32_t value;
  if(pckt.hit) value = readHit(); 
  else         value = readMiss();

  //FIXME: form a logging packet and store to log
  return value;
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
uint32_t CacheModel::readHit(bool verbose)
{
  uint32_t data = dary[pckt.wayActive]->ld(pckt.idx,pckt.off);
  bits->updateLru(pckt.idx,pckt.wayActive);
  return data;
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
  if(verbose) msg.imsg("+CacheModel::readMiss");

  //select either by Val or Lru
  int32_t valWaySel = waySelectByVal();
  int32_t lruWaySel = getLruWay();

  //pckt.info(cout,"HERE readMiss info");
  bool lruSel = valWaySel < 0;
  pckt.wayActive = lruSel ? lruWaySel : (uint32_t) valWaySel;

  cout<<endl;
  cout<<"HERE access addr    "<<HEX<<pckt.a<<endl;
  cout<<"HERE access idx     "<<HEX<<pckt.idx<<endl;
  cout<<"HERE lru way        "<<lruWaySel<<endl;
  //if using LRU check for dirty and write back if needed
  if(lruSel && wayIsMod(pckt.wayActive)) writeBack(pckt.wayActive);
 
  line_t line; 
  allocate(pckt.wayActive,line,verbose);
  //update the bits  - set the way valid, clear the mod, update lru

  cout<<"HERE new tag        "<<pckt.tag<<endl;
  cout<<"HERE new dary       "<<dary[pckt.wayActive]->mem[pckt.idx];
  bits->updateVal(pckt.idx,pckt.wayActive,1);
  bits->updateMod(pckt.idx,pckt.wayActive,0);
  bits->updateLru(pckt.idx,pckt.wayActive);
  return line[pckt.off]; 
}
// -----------------------------------------------------------------------
// Allocate from main memory to the index into the targetWay
//
// pckt holds the line address, as well as the tag and index extracted
// from the original request address
// 
// -----------------------------------------------------------------------
void CacheModel::allocate(uint32_t targetWay,line_t &line,bool verbose)
{
  if(verbose) msg.imsg("+CacheModel::allocate");
  //read mm and load the dary @ targetWay
  mm->q = mm->mem.find(pckt.mmAddr);

  //this should not have happened, must abort
  stringstream ss;ss<<HEX<<pckt.mmAddr;
  ASSERT(mm->q != mm->mem.end(),
         "missing main memory entry in allocate() 0x"+ss.str());
  //Check the ranges, none of this should happen, must abort
  //this assumes all arrays have the same number of indexs as tags
  ASSERT(targetWay < opts.l1_associativity,
         "allocate(): targetWay is corrupted, way:"+::to_string(targetWay));

  //Main memory data, this will be stored in dary pckt.idx, targetWay
  line =  mm->q->second;

  ASSERT(pckt.off < line.size(),  "allocate(): pckt.off is corrupted");
  ASSERT(pckt.idx < opts.l1_sets, "allocate(): pckt.idx is corrupted");

  //update the dary at way = targetWay
  dary[targetWay]->st_line(pckt.idx,line);

  //update the tag  at way = targetWay
  tags[targetWay]->mem.emplace(pckt.idx,pckt.tag);

  bits->q = bits->mem.find(pckt.idx);
  if(bits->q == bits->mem.end()) {
    bits->mem.emplace(pckt.idx,0);
  }
}
// -----------------------------------------------------------------------
// STORE
// -----------------------------------------------------------------------
void CacheModel::st(uint32_t a,uint32_t be,uint32_t data, bool verbose) 
{
  if(verbose) u.req_msg(cout,"STORE :",a,be);

  pckt = AddressPacket(a,be,getTagField(a),getIndexField(a),
                            getOffsetField(a),getMMAddr(a));

  bitsLookup(pckt,verbose);
  tagLookup(pckt,verbose);

  if(pckt.hit) writeHit(data);
  else         writeMiss(data);
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void CacheModel::writeHit(uint32_t d,bool verbose)
{
  dary[pckt.wayActive]->st(pckt.idx,pckt.off,pckt.be,d);
  bits->updateMod(pckt.idx,pckt.wayActive,1);
  bits->updateLru(pckt.idx,pckt.wayActive);
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void CacheModel::writeMiss(uint32_t d,bool verbose)
{
  if(verbose) msg.imsg("+CacheModel::writeMiss");

  //select either by Val or Lru
  int32_t valWaySel = waySelectByVal();
  int32_t lruWaySel = getLruWay();

  bool lruSel = valWaySel < 0;
  pckt.wayActive = lruSel ? lruWaySel : (uint32_t) valWaySel;

  //if using LRU check for dirty and write back if needed
  if(lruSel && wayIsMod(pckt.wayActive)) writeBack(pckt.wayActive);

  line_t line;
  allocate(pckt.wayActive,line,verbose);

  //merge in write data - FIXME find a way to share this, see Ram::st(be)
  uint32_t rd = line[pckt.off];
  uint32_t newData = u.stBytes(rd,d,pckt.be);

  line[pckt.off] = newData;
  dary[pckt.wayActive]->st_line(pckt.idx,line);

  //update the bits  - set the way valid, set the mod, update lru
  bits->updateVal(pckt.idx,pckt.wayActive,1);
  bits->updateMod(pckt.idx,pckt.wayActive,1);
  bits->updateLru(pckt.idx,pckt.wayActive);
}
// -----------------------------------------------------------------------
// INIT FUNCTIONS
// -----------------------------------------------------------------------
// clear/resize arrays in model and expect data - but not mm
//
// bits        expectBits
// tags        expectTags
// dary        expectDary
// captureData expectCaptureData
// mm          expectMm
// -----------------------------------------------------------------------
void CacheModel::clearResizeArrays()
{
  clearResizeMdlArrays();
  clearResizeExpArrays();
}
// -----------------------------------------------------------------------
void CacheModel::clearResizeMdlArrays()
{
  if(bits) delete bits;
  size_t numBits = opts.l1_associativity //valid
                 + opts.l1_associativity //modified
                 + (opts.l1_lru_bits+1); //pad LRU to 4 bits to match vlg

  bits = new BitArray("bits",opts.default_mm_entries,numBits);

  initSize(tags,"tags");
  initSize(dary,"dary");

  captureData.clear();

  if(mm) delete mm;
  mm = new Ram("mm",opts.default_mm_entries,opts.mm_fetch_size*8);
}
// -----------------------------------------------------------------------
void CacheModel::clearResizeExpArrays()
{
  if(expectBits) delete expectBits;
  size_t numBits = opts.l1_associativity //valid
                 + opts.l1_associativity //modified
                 + (opts.l1_lru_bits+1); //pad LRU to 4 bits to match vlg

  expectBits = new BitArray("expBits",opts.default_mm_entries,numBits);

  initSize(expectTags,"expTags");
  initSize(expectDary,"expDary");

  expectCaptureData.clear();

  if(expectMm) delete expectMm;
  expectMm = new Ram("expMm",opts.default_mm_entries,opts.mm_fetch_size*8);
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void CacheModel::initSize(std::vector<Tag*> &_tags,std::string name)
{
  for (auto _tp : _tags) delete _tp;
  _tags.clear();

  for(size_t i=0;i<opts.l1_associativity;++i) {
    Tag *empty = new Tag(name+::to_string(i),opts.l1_sets,opts.l1_tagBits);
    _tags.push_back(empty); //push vs emplace, just better txt formatting
  }
}
// -----------------------------------------------------------------------
void CacheModel::initSize(std::vector<Ram*> &_dary,std::string name)
{
  for (auto _dp : _dary) delete _dp;
  _dary.clear();

  for(size_t i=0;i<opts.l1_associativity;++i) {
    Ram *empty = new Ram(name+::to_string(i),opts.l1_sets,opts.l1_line_size*8);
    _dary.push_back(empty);
  }
}
// -----------------------------------------------------------------------
// MM = main memory
// -----------------------------------------------------------------------
bool CacheModel::initializeMM()
{
  string fn = opts.mm_file;
  ifstream in(fn);
  if(!in.is_open()) {
    msg.emsg("Can not open file "+u.tq(fn));
    return false;
  }

  mm = new Ram("mm",opts.default_mm_entries,opts.mm_fetch_size*8);

  msg.imsg("loading MM ram from file: "+u.tq(fn));

  if(!u.loadRamFromVerilog(mm,in)) { return false; }
  in.close();
  return true;
}
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void CacheModel::bitsLookup(AddressPacket &pckt,bool verbose) 
{
  pckt.val = 0;
  pckt.mod = 0;
  pckt.lru = 0;

  bits->q = bits->mem.find(pckt.idx);
  if(bits->q != bits->mem.end()) {
    pckt.val = bits->getVal();
    pckt.mod = bits->getMod();
    pckt.lru = bits->getLru(pckt.idx);
  }
}
// -----------------------------------------------------------------------
// Scan the 4 tags, in pckt.idx of each compare contents to pckt.tag
// -----------------------------------------------------------------------
void CacheModel::tagLookup(AddressPacket &pckt,bool verbose) 
{
  for(int i=tags.size()-1;i>=0;--i) {

    //Does this tag have this index
    tags[i]->q = tags[i]->mem.find(pckt.idx);
    //Yes 
    if(tags[i]->q != tags[i]->mem.end()) {
      //this tag has this index, compare the contents and valid bit
      uint32_t contents = tags[i]->q->second;
      //The contents match
      if(contents == pckt.tag && pckt.val[i] == 1) {
        if(verbose) u.tag_msg(cout,"HIT   :",pckt.a,pckt.tag);
        pckt.hit    = true;
        pckt.wayActive = i;
        return;
      }
    }
  }

  pckt.hit = false;
  if(verbose) u.tag_msg(cout,"MISS  :",pckt.a,pckt.tag);
  //pckt.hit = false; >????
}
// -----------------------------------------------------------------------
// The LRU is not updated, this function does not know what 
// triggered the writeback, e.g. could be cache maintenance. The LRU is
// updated by the caller
// -----------------------------------------------------------------------
void CacheModel::writeBack(uint32_t victimWay)
{
  //Read the tag address for the victim
  uint32_t victimIdx = pckt.idx;
  uint32_t victimTag = tags[victimWay]->mem[victimIdx];

  cout<<"HERE lru Tag        "<<HEX<<victimTag<<endl;

  //Form the writeBack main memory address
  uint32_t fullAddress = victimTag << opts.l1_tagShift
                       | victimIdx << opts.l1_setShift;
  uint32_t lineAddress = getMMAddr(fullAddress);
  cout<<"HERE victim mm addr "<<HEX<<lineAddress<<endl;

  //Read the victim data
  line_t line = dary[victimWay]->mem[victimIdx];

  //Write the victim data to mm
  mm->st_line(lineAddress,line);
  //cout<<"HERE new mm line    "<<mm->mem[lineAddress];

  //Mark the MOD bit as clean
  bits->updateMod(victimIdx,victimWay,0);
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
