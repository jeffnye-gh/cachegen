#include "cachegen.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
using namespace std;

// ----------------------------------------------------------------
// ----------------------------------------------------------------
CacheGen::CacheGen(int _ac,char **_av)
{
  msg.setWho("cgen");
  opts.msg.setWho("cgen_opt");

  if(!opts.setupOptions(_ac,_av)) {
    throw std::invalid_argument("Option parsing failed");
  }

  process();
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
#define DEC dec
#define FLT fixed<<setprecision(3)
void CacheGen::process()
{
  if(opts.dry_run) {
    stringstream ss;
    cout<<'\n';

    cout<<"L1 cache parameters\n";
    ValueUnit l1_cap = getValueAndUnits(opts.l1_capacity);

    cout<<"  Capacity      "<<FLT<<l1_cap.value<<" "<<l1_cap.units<<'\n';
    cout<<"  Line size     "<<DEC<<opts.l1_line_size<<'\n';
    cout<<"  Associativity "<<DEC<<opts.l1_associativity<<'\n';
    cout<<"  Critical word "<<DEC<<opts.l1_critical_word_first<<'\n';

    cout<<'\n';

    cout<<"Main memory parameters\n";
    ValueUnit mm_cap = getValueAndUnits(opts.mm_capacity);
    cout<<"  Capacity      "<<FLT<<mm_cap.value<<" "<<mm_cap.units<<'\n';
    cout<<"  Fetch size    "<<DEC<<opts.mm_fetch_size<<'\n';

    cout<<'\n';

    opts.l1_capacityBits = log2(opts.l1_capacity);
    opts.l1_offsetBits   = log2(opts.l1_line_size);
    opts.l1_blocks       = opts.l1_capacity/opts.l1_line_size;
    opts.l1_blockBits    = log2(opts.l1_blocks);
    opts.l1_setBits      = log2(opts.l1_blocks/opts.l1_associativity);
    opts.l1_assocBits    = log2(opts.l1_associativity);

    opts.mm_capacityBits = log2(opts.mm_capacity);

    cout<<"  Capacity      "<<opts.l1_capacityBits<<" bits\n";
    cout<<"  Word offset   "<<opts.l1_offsetBits  <<" bits\n";
    cout<<"  Associativity "<<opts.l1_assocBits   <<" bits\n";

    cout<<'\n';

    cout<<"  #Blocks = Capacity / Block size = 2^"<<opts.l1_capacityBits
        <<                                 " / 2^"<<opts.l1_offsetBits
        <<                                 " = 2^"<<opts.l1_blockBits<<'\n';

    cout<<"  #Sets   = #Blocks  / #Ways      = 2^"<<opts.l1_blockBits
        <<                                 " / 2^"<<opts.l1_assocBits
        <<                                 " = 2^"<<opts.l1_setBits<<'\n';

    opts.l1_tagBits
      = opts.mm_capacityBits - opts.l1_setBits - opts.l1_offsetBits;

    opts.l1_tagMsb  = opts.mm_capacityBits - 1;
    opts.l1_tagLsb  = opts.l1_tagMsb - opts.l1_tagBits + 1;

    cout<<'\n';

    cout<<"  With "<<opts.mm_capacityBits<<" main memory address bits\n";
    cout<<"    tag width    = "<<opts.mm_capacityBits
                     <<" - "<<opts.l1_setBits
                     <<" - "<<opts.l1_offsetBits
                     <<" = "<<opts.l1_tagBits
                     <<" -> ["<<opts.l1_tagMsb<<":"<<opts.l1_tagLsb<<"]\n";
    cout<<"    (capacity bits - index bits - offset bits)\n";

  
    opts.l1_setMsb  = opts.l1_tagLsb - 1;
    opts.l1_setLsb  = opts.l1_setMsb - opts.l1_setBits + 1;
    cout<<"    set index    = "<<opts.l1_setBits<<" bits "
        <<"         -> ["<<opts.l1_setMsb<<":"<<opts.l1_setLsb<<"]\n";
    opts.l1_offMsb  = opts.l1_setLsb - 1;
    opts.l1_offLsb  = opts.l1_offMsb - opts.l1_offsetBits + 1;
    cout<<"    offset width = "<<opts.l1_offsetBits<<" bits "
        <<"          -> ["<<opts.l1_offMsb<<":"<<opts.l1_offLsb<<"]\n";

    string upper = getUpperHeader(31,0);
    string lower = getLowerHeader(31,0);

    cout<<'\n';

    cout<<"    "<<upper<<'\n';
    cout<<"    "<<lower<<'\n';

    string vx;
    for(size_t i=0;i<opts.l1_tagBits;++i)      vx += "t";
    for(size_t i=0;i<opts.l1_setBits;++i)      vx += "i";
    for(size_t i=0;i<opts.l1_offsetBits-2;++i) vx += "o";
    vx += "-- bbbb";

    cout<<"    "<<vx<<'\n';

  } 
}
#undef HEX
#undef FLT
