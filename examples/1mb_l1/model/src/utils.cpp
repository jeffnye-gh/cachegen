#include "utils.h"
#include "ram.h"
#include <boost/algorithm/string.hpp>
#include <algorithm>
//#include <iostream>
using namespace std;

const string Utils::vlgSep =
"// ---------------------------------------------------------------\n";

// ----------------------------------------------------------------
// FIXME: consider combining these loadRamFrom... methods to share code
// ----------------------------------------------------------------
bool Utils::compare(BitArray &exp,BitArray &act,
                    uint32_t &errs,size_t start, size_t count,bool verbose)
{
  if(!sizeChecks(errs,exp.mem.size(),act.mem.size())) return false;

  bool ok = true;

  exp.q = exp.mem.find(start);
  if(exp.q == exp.mem.end()) {
    msg.emsg("Start index not found in exp array");
    ++errs;
    return false;
  }

  act.q = act.mem.find(start);
  if(act.q == act.mem.end()) {
    msg.emsg("Start index not found in act array");
    ++errs;
    return false;
  }

  for(size_t i=0;i<count;++i) {

    stringstream ss;
    ss<<"idx:"<<dec<<setw(2)<<i<<":bits:";

    uint32_t expD = exp.q->second;
    bitset<4> expV((expD>>8)&0xF);
    bitset<4> expM((expD>>4)&0xF);
    bitset<4> expL((expD>>0)&0xF);
    ss<<" exp:"<<expV<<" "<<expM<<" "<<expL;

    uint32_t actD = act.q->second;
    bitset<4> actV((actD>>8)&0xF);
    bitset<4> actM((actD>>4)&0xF);
    bitset<4> actL((actD>>0)&0xF);
    ss<<" act:"<<actV<<" "<<actM<<" "<<actL;

    if(expD != actD) {
      ss<<" FAIL";
      msg.emsg(ss.str());
      ok = false;
    } else if(verbose) {
      ss<<" PASS";
      msg.imsg(ss.str());
    }
    ++exp.q;
    act.q = act.mem.find(exp.q->first);
    if(act.q == act.mem.end()) {
      msg.emsg("Index error at "+::to_string(exp.q->first));
    }
  } 

  return ok;
}
// ----------------------------------------------------------------
bool Utils::compare(vector<Ram*> &exp,vector<Ram*> &act,
                    uint32_t &errs,size_t start, size_t count,bool verbose)
{
  if(!sizeChecks(errs,exp.size(),act.size())) return false;
  if(exp.size() != 4 || act.size() !=4) {
    msg.emsg("compare requires 4 ways per array");
    return false;
  }

  bool ok = true;

  for(size_t way=0;way<4;++way) {
    for(size_t idx=start;idx<start+count;++idx) {

      exp[way]->q = exp[way]->mem.find(idx);
      act[way]->q = act[way]->mem.find(idx);

      bool expFound = exp[way]->q != exp[way]->mem.end();
      bool actFound = act[way]->q != act[way]->mem.end();

      if( !expFound || !actFound) {
        msg.emsg("Index not found, way:"+::to_string(way)
                                +" idx:"+::to_string(idx));
        msg.emsg("   exp found: "+::to_string(expFound));
        msg.emsg("   act found: "+::to_string(actFound));
        ++errs;
        continue;
      }

      string expLine = tostr(exp[way]->q->second);
      string actLine = tostr(act[way]->q->second);

      string idxs = "w"+::to_string(way)+":"+::to_string(idx);
      if(expLine != actLine) {
        ++errs;
        ok = false;
        //msg.emsg("compareR: ");
        msg.msg(idxs+":exp:"+expLine+" F");
        msg.msg(idxs+":act:"+actLine+" F");
        msg.msg("");
      } else if(verbose) {
        //msg.imsg("compareR: ");
        msg.msg(idxs+":exp:"+expLine+" P");
        msg.msg(idxs+":act:"+actLine+" P");
        msg.msg("");
      }
    }
  }
  return ok;
}
// ----------------------------------------------------------------
bool Utils::compare(vector<Tag*> &exp,vector<Tag*> &act,
                    uint32_t &errs,size_t start, size_t count,bool verbose)
{ 
  if(!sizeChecks(errs,exp.size(),act.size())) return false;
  if(exp.size() != 4 || act.size() !=4) {
    msg.emsg("compare requires 4 ways per array");
    return false;
  }

  bool ok = true;

  for(size_t way=0;way<4;++way) {
    for(size_t idx=start;idx<start+count;++idx) {

      exp[way]->q = exp[way]->mem.find(idx);
      act[way]->q = act[way]->mem.find(idx);

      bool expFound = exp[way]->q != exp[way]->mem.end();
      bool actFound = act[way]->q != act[way]->mem.end();

      if( !expFound || !actFound) {
        msg.emsg("Index not found, idx:"+::to_string(idx));
        msg.emsg("   exp found: "+::to_string(expFound));
        msg.emsg("   act found: "+::to_string(actFound));
        ++errs;
        continue;
      }

      uint32_t expData = exp[way]->q->second;
      uint32_t actData = act[way]->q->second;
      stringstream ss;
      ss<<"tag way "<<dec<<way<<" idx:"<<setw(4)<<dec<<idx
        <<" exp:"<<HEX<<expData<<" act:"<<HEX<<actData;

      if(expData != actData) {
        ++errs;
        ss<<" FAIL";
        ok = false;
        msg.emsg(ss.str());
      } else if(verbose) {
        ss<<" PASS";
        msg.imsg(ss.str());
      }

    }
  }
  return ok;
}
// ----------------------------------------------------------------
bool Utils::compare(vector<uint32_t> &exp,vector<uint32_t> &act,
                    uint32_t &errs,size_t start, size_t count,bool verbose)
{
  if(!sizeChecks(errs,exp.size(),act.size())) return false;

  bool ok = true;
  bool first = true;

  for(size_t i=start;i<start+count;++i) {
    if(i >= exp.size() || i >= act.size()) {
      if(first) msg.emsg("Compare extent error detected" );
      first = false;
      ok    = false;
      ++errs;
      //continue;
    }

    stringstream ss;
    ss<<dec<<setw(2)<<i<<": compare exp:"<<HEX<<exp[i]<<" act:"<<HEX<<act[i];

    if(exp[i] != act[i]) {
      ++errs;
      ss<<" FAIL";
      msg.emsg(ss.str());
      ok = false;
    } else if(verbose) {
      ss<<" PASS";
      msg.imsg(ss.str());
    }
  }

  return ok;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Utils::sizeChecks(uint32_t &errs,size_t expSize,size_t actSize)
{
  if(expSize == 0)  {
    ++errs;
    msg.emsg("Expect vector has zero size");
    return false;
  }

  if(actSize == 0)  {
    ++errs;
    msg.emsg("Actual vector has zero size");
    return false;
  }

  if(expSize != actSize) {
    ++errs;
    msg.emsg("Expect and actual have unequal size");
    msg.emsg("exp "+::to_string(expSize)+" act "+::to_string(actSize));
    return false;
  }

  return true;
}
// ----------------------------------------------------------------
// FIXME: consider combining these loadRamFrom... methods to share code
// ----------------------------------------------------------------
bool Utils::loadCaptureFromVerilog(vector<uint32_t> &vec,string fn,bool verbose)
{
  if(verbose) msg.imsg("Loading file for capture array: "+tq(fn));

  ifstream in(fn);
  if(!in.is_open()) {
    msg.emsg("Could not open file "+tq(fn));
    return false;
  }

  uint32_t lineNum=0;
  uint32_t runningAddr = 0,address;
  string line;

  while(getline(in, line)) {
    ++lineNum;
    size_t pos = line.find("//");
    if(pos != string::npos) line.erase(pos);
    if(line.find_first_not_of(' ') == string::npos) continue;

    vector<string> lvec;
    boost::split(lvec,line,boost::is_any_of(" \t"),boost::token_compress_on);

    //boost will keep empty elements in the vector, remove them
    lvec.erase(remove_if(lvec.begin(),lvec.end(),
      [&]( const string &s ) { return s.length() == 0; }), lvec.end());

    if(lvec.size() < 1) continue;

    //FIXME: Address isnt used for capture addr or data, consider changing 
    //the file format 
    string data;
    if(lvec.size() == 2) {
      data = lvec[1];
      if(lvec[0][0] == '@') {
        lvec[0].erase(0,1);
        address = hexStrToUint(lvec[0]);
      } else {
        msg.emsg("Address syntax error, line "+::to_string(lineNum)+", "+tq(fn));
        return false;
      }
    } else {
      address = runningAddr;
      data = lvec[0];
    }

    uint32_t d = (uint32_t) ::stoi(data,nullptr,16);
    vec.push_back(d);
    runningAddr = address + 1;
  }

  return true;
}
// ----------------------------------------------------------------
bool Utils::loadRamFromVerilog(vector<Tag*> &tags,string fn,bool verbose)
{
  if(verbose) msg.imsg("Loading file for tag array: "+tq(fn));

  ifstream in(fn);
  if(!in.is_open()) {
    msg.emsg("Could not open file "+tq(fn));
    return false;
  }

  uint32_t lineNum=0;
  uint32_t runningAddr = 0,address;
  string line;

  while(getline(in, line)) {
    ++lineNum;
    size_t pos = line.find("//");
    if(pos != string::npos) line.erase(pos);
    if(line.find_first_not_of(' ') == string::npos) continue;

    vector<string> lvec;
    boost::split(lvec,line,boost::is_any_of(" \t"),boost::token_compress_on);

    //boost will keep empty elements in the vector, remove them
    lvec.erase(remove_if(lvec.begin(),lvec.end(),
      [&]( const string &s ) { return s.length() == 0; }), lvec.end());

    if(lvec.size() < 1) continue;

    string data;
    if(lvec.size() == 2) {
      data = lvec[1];
      if(lvec[0][0] == '@') {
        lvec[0].erase(0,1);
        address = hexStrToUint(lvec[0]);
      } else {
        msg.emsg("Address syntax error, line "
                 +::to_string(lineNum)+", "+tq(fn));
        return false;
      }
    } else {
      address = runningAddr;
      data = lvec[0];
    }

    vector<string> dvec;
    boost::split(dvec,data,boost::is_any_of("_"),boost::token_compress_on);
    if(dvec.size() != 4) {
      msg.emsg("Data syntax error, line "+::to_string(lineNum)+", "+tq(fn));
      return false;
    }

    size_t idx = 0;
    for(int way=3;way>=0;--way) {
      string s = "0x"+dvec[idx++];
      uint32_t d = stoul(s,nullptr,16);
      tags[way]->mem.insert({address,d});
    }

    runningAddr = address + 1;
  }

  //for(auto t : tags) { t->info(cout,0,8); }
  return true;
}
// ----------------------------------------------------------------
// ----------------------------------------------------------------
bool Utils::loadRamFromVerilog(BitArray *bits,string fn,bool verbose)
{
  if(verbose) msg.imsg("Loading file for bit array: "+tq(fn));

  ifstream in(fn);
  if(!in.is_open()) {
    msg.emsg("Could not open file "+tq(fn));
    return false;
  }

  uint32_t lineNum=0;
  uint32_t runningAddr = 0,address;
  string line;

  while(getline(in, line)) {
    ++lineNum;
    size_t pos = line.find("//");
    if(pos != string::npos) line.erase(pos);
    if(line.find_first_not_of(' ') == string::npos) continue;

    vector<string> lvec;
    boost::split(lvec,line,boost::is_any_of(" \t"),boost::token_compress_on);

    //boost will keep empty elements in the vector, remove them
    lvec.erase(remove_if(lvec.begin(),lvec.end(),
      [&]( const string &s ) { return s.length() == 0; }), lvec.end());

    if(lvec.size() < 1) continue;

    string data;
    if(lvec.size() == 2) {
      data = lvec[1];
      if(lvec[0][0] == '@') {
        lvec[0].erase(0,1);
        address = hexStrToUint(lvec[0]);
      } else {
        msg.emsg("Address syntax error, line "
                 +::to_string(lineNum)+", "+tq(fn));
        return false;
      }
    } else {
      address = runningAddr;
      data = lvec[0];
    }

    vector<string> dvec;
    boost::split(dvec,data,boost::is_any_of("_"),boost::token_compress_on);
    if(dvec.size() != 3) {
      msg.emsg("Data syntax error, line "+::to_string(lineNum)+", "+tq(fn));
      return false;
    }

    bitset<4> val(dvec[0]);
    bitset<4> mod(dvec[1]);
    bitset<4> lru(dvec[2]);
    uint32_t d = (val.to_ulong()& 0xF) << 8
               | (mod.to_ulong()& 0xF) << 4
               | (lru.to_ulong()& 0xF) << 0;

    bits->mem.insert({address,d});
    runningAddr = address + 1;
  }

  //if(verbose) msg.imsg("Loading file complete");
  //bits->info(cout,0,16);
  in.close();
  return true; 
}
// ----------------------------------------------------------------
bool Utils::loadRamFromVerilog(Ram *ram,string fn,bool verbose)
{
  if(verbose) msg.imsg("Loading file for ram array: "+tq(fn));

  ifstream in(fn);
  if(!in.is_open()) {
    msg.emsg("Could not open file "+tq(fn));
    return false;
  }

  return loadRamFromVerilog(ram,in);
}
// ----------------------------------------------------------------
bool Utils::loadRamFromVerilog(Ram *ram,ifstream &in,bool verbose)
{
  if(verbose) msg.imsg("Loading file for ram:"+tq(ram->name));
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

  //if(verbose) msg.imsg("Loading file complete");
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
// ----------------------------------------------------------------
// ----------------------------------------------------------------
std::string Utils::tostr(vector<uint32_t> &v)
{
  vector<uint32_t> ll = v;
  std::reverse(ll.begin(),ll.end());

  string ret = "";
  bool first = true;

  for(auto u : ll) {
    stringstream ss;
    ss<<HEX<<u;
    string sep = first ? "" : "_";
    first = false;
    ret += sep + ss.str();
  }

  return ret;
}
