#include "cachegen.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ostream>
#include <cmath>
using namespace std;

// ----------------------------------------------------------------
// ----------------------------------------------------------------
CacheGen::CacheGen(int _ac,char **_av)
{
  msg.setWho("cgen ");
  msg.imsg("+CacheGen::CacheGen");
  opts.msg.setWho("cgen_opt");

  if(!opts.setupOptions(_ac,_av)) {
    throw std::invalid_argument("Option parsing failed");
  }

}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool CacheGen::execute()
{
  msg.imsg("+CacheGen::execute()");
  if(!opts.dry_run && opts.generate) {
    if(!generate()) return false;
  }

  if(!opts.dry_run && opts.run) {
    if(!run()) return false;
  }

  if(opts.dry_run) msg.imsg("Dry run complete");

  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
#define DEC dec
#define FLT fixed<<setprecision(3)
void CacheGen::createDataSheet(ostream &out)
{
  msg.imsg("+CacheGen::process()");

  stringstream ss;
  out<<'\n';

  out<<"L1 cache properties\n";
  ValueUnit l1_cap = u.getValueAndUnits(opts.l1_capacity);

  out<<"  Capacity      "<<FLT<<l1_cap.value<<" "<<l1_cap.units<<'\n';
  out<<"  Line size     "<<DEC<<opts.l1_line_size<<" bytes\n";
  out<<"  Associativity "<<DEC<<opts.l1_associativity<<" ways\n";

  out<<'\n';

  out<<"  Read miss          : "<<opts.l1_read_miss_policy<<'\n';
  out<<"  Write miss         : "<<opts.l1_write_miss_policy<<'\n';
  out<<"  Write hit          : "<<opts.l1_write_hit_policy<<'\n';
  out<<"  Replacement        : "<<opts.l1_replacement_policy<<'\n';
  out<<"  Coherency protocol : "<<opts.l1_coherency_protocol<<'\n';
  out<<"  Victim buffer size : "<<opts.l1_victim_buffer_size<<'\n';
  out<<"  Store buffer size  : "<<opts.l1_store_buffer_size<<'\n';
  out<<"  Tag type           : "<<opts.l1_tag_type<<'\n';
  out<<"  Critical word first: "<<opts.l1_critical_word_first<<'\n';
  out<<"  MMU                : "<<opts.l1_mmu_present<<'\n';
  out<<"  MPU                : "<<opts.l1_mpu_present<<'\n';

  out<<'\n';


  opts.l1_address_bits = log2(opts.l1_capacity);
  opts.l1_offsetBits   = log2(opts.l1_line_size);
  opts.l1_blocks       = opts.l1_capacity/opts.l1_line_size;
  opts.l1_blockBits    = log2(opts.l1_blocks);
  opts.l1_setBits      = log2(opts.l1_blocks/opts.l1_associativity);
  opts.l1_assocBits    = log2(opts.l1_associativity);

  opts.mm_address_bits = log2(opts.mm_capacity);

  out<<"  Capacity      "<<opts.l1_address_bits<<" bits\n";
  out<<"  Word offset   "<<opts.l1_offsetBits  <<" bits\n";
  out<<"  Associativity "<<opts.l1_assocBits   <<" bits\n";

  out<<'\n';

  out<<"  #Blocks = Capacity / Block size = 2^"<<opts.l1_address_bits
     <<                                 " / 2^"<<opts.l1_offsetBits
     <<                                 " = 2^"<<opts.l1_blockBits<<'\n';

  out<<"  #Sets   = #Blocks  / #Ways      = 2^"<<opts.l1_blockBits
     <<                                 " / 2^"<<opts.l1_assocBits
     <<                                 " = 2^"<<opts.l1_setBits<<'\n';

  opts.l1_tagBits
    = opts.mm_address_bits - opts.l1_setBits - opts.l1_offsetBits;

  opts.l1_tagMsb  = opts.mm_address_bits - 1;
  opts.l1_tagLsb  = opts.l1_tagMsb - opts.l1_tagBits + 1;
  opts.l1_tagShift = opts.l1_tagLsb;
  opts.l1_tagMask  = u.makeMask(opts.l1_tagMsb,opts.l1_tagLsb);

  out<<'\n';

  out<<"  With "<<opts.mm_address_bits<<" main memory address bits\n";
  out<<"    tag width    = "<<opts.mm_address_bits
                   <<" - "<<opts.l1_setBits
                   <<" - "<<opts.l1_offsetBits
                   <<" = "<<opts.l1_tagBits
                   <<" -> ["<<opts.l1_tagMsb<<":"<<opts.l1_tagLsb<<"]\n";
  out<<"    (capacity bits - index bits - offset bits)\n";


  opts.l1_setMsb  = opts.l1_tagLsb - 1;
  opts.l1_setLsb  = opts.l1_setMsb - opts.l1_setBits + 1;
  opts.l1_setShift = opts.l1_setLsb;
  opts.l1_setMask  = u.makeMask(opts.l1_setMsb,opts.l1_setLsb);

  out<<"    set index    = "<<opts.l1_setBits<<" bits "
     <<"         -> ["<<opts.l1_setMsb<<":"<<opts.l1_setLsb<<"]\n";

  opts.l1_offMsb  = opts.l1_setLsb - 1;
  opts.l1_offLsb  = opts.l1_offMsb - opts.l1_offsetBits + 1;
  opts.l1_offShift = opts.l1_offLsb;
  opts.l1_offMask  = u.makeMask(opts.l1_offMsb,opts.l1_offLsb);

  out<<"    offset width = "<<opts.l1_offsetBits<<" bits "
     <<"          -> ["<<opts.l1_offMsb<<":"<<opts.l1_offLsb<<"]\n";

  string upper = u.getUpperHeader(31,0);
  string lower = u.getLowerHeader(31,0);

  out<<'\n';

  out<<"    "<<upper<<'\n';
  out<<"    "<<lower<<'\n';

  string vx;
  for(size_t i=0;i<opts.l1_tagBits;++i)      vx += "t";
  for(size_t i=0;i<opts.l1_setBits;++i)      vx += "i";
  for(size_t i=0;i<opts.l1_offsetBits-2;++i) vx += "o";
  vx += "-- bbbb";

  out<<"    "<<vx<<'\n';

  out<<'\n';

  uint32_t numWays   = opts.l1_associativity;
  uint32_t indexBits = opts.l1_setBits;
  opts.l1_lru_bits = opts.l1_replacement_policy == "PLRU" 
                   ? numWays - 1 : 0;
  opts.l1_sets  = pow(2,opts.l1_setBits)/1024;
  uint32_t sets = opts.l1_sets;

  out<<"  Tags      :  "<<sets<<"K x "<<numWays<<" x "<<indexBits<<"b\n";
  out<<"  Valid bits:  "<<sets<<"K x "<<numWays<<" x 1b\n";
  out<<"  Dirty bits:  "<<sets<<"K x "<<numWays<<" x 1b\n";
  out<<"  LRU bits  :  "<<sets<<"K x "<<opts.l1_lru_bits<<"b\n";

  out<<dec<<'\n';

  out<<"  l1_tagBits        "<<opts.l1_tagBits<<'\n';
  out<<"  l1_setBits(index) "<<opts.l1_setBits<<'\n';
  out<<"  l1_offsetBits     "<<opts.l1_offsetBits<<'\n';
  out<<"  l1_lru_bits       "<<opts.l1_lru_bits<<'\n';
  out<<"  l1_line_size      "<<opts.l1_line_size<<'\n';

  out<<'\n';

  out<<"  l1_address_bits   "<<opts.l1_address_bits<<'\n';
  out<<"  l1_assocBits      "<<opts.l1_assocBits<<'\n';
  out<<'\n';
  out<<"  l1_tagMsb         "<<opts.l1_tagMsb<<'\n';
  out<<"  l1_tagLsb         "<<opts.l1_tagLsb<<'\n';
  bitset<32> l1_tagMask(opts.l1_tagMask);
  out<<"  l1_tagMask        "<<l1_tagMask<<'\n';
  out<<"  l1_tagShift       "<<opts.l1_tagShift<<'\n';
  out<<'\n';
  out<<"  l1_setMsb         "<<opts.l1_setMsb<<'\n';
  out<<"  l1_setLsb         "<<opts.l1_setLsb<<'\n';
  bitset<32> l1_setMask(opts.l1_setMask);
  out<<"  l1_setMask        "<<l1_setMask<<'\n';
  out<<"  l1_setShift       "<<opts.l1_setShift<<'\n';
  out<<'\n';
  out<<"  l1_offMsb         "<<opts.l1_offMsb<<'\n';
  out<<"  l1_offLsb         "<<opts.l1_offLsb<<'\n';
  bitset<32> l1_offMask(opts.l1_offMask);
  out<<"  l1_offMask        "<<l1_offMask<<'\n';
  out<<"  l1_offShift       "<<opts.l1_offShift<<'\n';
  out<<'\n';
  out<<"  l1_sets           "<<opts.l1_sets<<'\n';
  //out<<"  l1_blocks         "<<opts.l1_blocks<<'\n'; not pertinent
  //out<<"  l1_blockBits      "<<opts.l1_blockBits<<'\n'; confusing
  out<<'\n';
  out<<"  L1 numWays        "<<numWays<<'\n';
  out<<"  L1 indexBits      "<<indexBits<<'\n';
  out<<"  L1 bytes per line "<<indexBits<<'\n';
  out<<'\n';

  uint32_t mm_lineMsb = opts.mm_address_bits -1;
  uint32_t mm_lineLsb = opts.l1_setLsb;

  opts.mm_lineShift = opts.l1_setShift;
  opts.mm_lineMask  = u.makeMask(mm_lineMsb,mm_lineLsb);

  bitset<32> mm_lineMask(opts.mm_lineMask);
  out<<"  mm_lineMask       "<<mm_lineMask<<'\n';
  out<<"  mm_lineShift      "<<opts.mm_lineShift<<'\n';

  out<<'\n';

  out<<"Main memory properties\n";
  //FIXME add cmdline option
  opts.mm_entries = opts.default_mm_entries;
  ValueUnit mm_cap = u.getValueAndUnits(opts.mm_capacity);
  out<<"  Capacity          "<<FLT<<mm_cap.value<<" "<<mm_cap.units<<'\n';
  out<<"  Populated entries "<<opts.mm_entries<<'\n';
  out<<"  Fetch size        "<<DEC<<opts.mm_fetch_size<<'\n';

  out<<'\n';
}
#undef HEX
#undef FLT
