#include "utils.h"
#include "ram.h"
#include <boost/algorithm/string.hpp>
//#include <iostream>
using namespace std;

const string Utils::vlgSep =
"// ---------------------------------------------------------------\n";

// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Utils::loadRamFromVerilog(Ram *ram,ifstream &in)
{
  msg.imsg("Loading file for ram:"+tq(ram->name));
  string line;
  uint32_t lineNum = 0;
  uint32_t runningAddr = 0,address;
  line_t empty;

  while(getline(in, line)) {
    ++lineNum;
    size_t pos = line.find("//");
    if(pos != string::npos) line.erase(pos);
    if(line.find_first_not_of(' ') == string::npos) continue;

    vector<string> lvec;
    boost::split(lvec,line,boost::is_any_of(" \t"),boost::token_compress_on);

    ASSERT(lvec.size() == 2,"FIXME: unhandled split vec size");

    if(lvec[0][0] == '@') {
      lvec[0].erase(0,1);
      address = hexStrToUint(lvec[0]);
    } else {
      address = runningAddr;
    }

    vector<string> dvec;
    boost::split(dvec,lvec[1],boost::is_any_of("_"),boost::token_compress_on);

    if(ram->mem.find(address) == ram->mem.end()) {
      ram->mem.insert(make_pair(address,empty));
    }

    ram->q = ram->mem.find(address);
    for(auto s : dvec) (*ram->q).second.push_back(hexStrToUint(s));
    std::reverse((*ram->q).second.begin(), (*ram->q).second.end());

    runningAddr = address + 1;
    
  }

  msg.imsg("Loading file complete");
  //ram->info(cout,0,1024);
  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
uint32_t Utils::hexStrToUint(string s)
{
  string ns = s;
  to_upper(ns);

  size_t pos = ns.find("0X");
  if(pos != string::npos) ns.erase(pos,2);

  pos = ns.find("@");
  if(pos != string::npos) ns.erase(pos,1);

  uint32_t i;

  istringstream iss(ns);
  iss >> std::hex >> i;
  return i;
}
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
  
  if(v < mb)      { vu.first = v/kb; vu.second = "KB"; }
  else if(v < gb) { vu.first = v/mb; vu.second = "MB"; }
  else            { vu.first = v/gb; vu.second = "GB"; }
  
  return vu;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
uint32_t Utils::makeMask(uint32_t msb,uint32_t lsb)
{
  return (1 << (msb-lsb+1)) - 1; 
}
