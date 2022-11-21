#include "cachegen.h"
#include <string>
using namespace std;

// ----------------------------------------------------------------
// ----------------------------------------------------------------
string CacheGen::getUpperHeader(uint32_t msb,uint32_t lsb)
{
  string ret;

  for(int i=(int)msb;i>=(int)lsb;--i) {
    uint32_t n = i/10;
    if(n > 0) ret += ::to_string(n);
    else      ret += " ";
  }

  return ret;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
string CacheGen::getLowerHeader(uint32_t msb,uint32_t lsb)
{
  string ret;

  for(int i=(int)msb;i>=(int)lsb;--i) {
    uint32_t n = i%10;
    ret += ::to_string(n);
  }

  return ret;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
ValueUnit CacheGen::getValueAndUnits(uint64_t v)
{ 
  ValueUnit vu; 
  uint64_t kb = 1024;
  uint64_t mb = 1024*1024;
  uint64_t gb = 1024*1024*1024;
  
  if(v < mb)      { vu.value = v/kb; vu.units = "KB"; }
  else if(v < gb) { vu.value = v/mb; vu.units = "MB"; }
  else            { vu.value = v/gb; vu.units = "GB"; }
  
  return vu;
}

