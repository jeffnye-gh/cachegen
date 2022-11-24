#include "cachegen.h"
#include <string>
using namespace std;

// ----------------------------------------------------------------
// ----------------------------------------------------------------
string Utils::getUpperHeader(uint32_t msb,uint32_t lsb)
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
string Utils::getLowerHeader(uint32_t msb,uint32_t lsb)
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
ValueUnit Utils::getValueAndUnits(uint64_t v)
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
// ----------------------------------------------------------------
// ----------------------------------------------------------------
uint32_t Utils::makeMask(uint32_t msb,uint32_t lsb)
{
  return (1 << (msb-lsb+1)) - 1; 
}
