#include "ram.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
using namespace std;

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void Ram::info(ostream &out,uint32_t first,uint32_t last)
{
  out << "// ----------------------------------------"<<endl;
  out << "// Ram, name : " << name    <<endl;
  out << "//   entries : " << entries <<endl;
  out << "//   width   : " << width   <<endl;
  out << "// ----------------------------------------"<<endl;

  q = mem.find(first);

  if(q == mem.end()) {
    out<<"// -W: Could not find first address"<<endl;
    return;
  }

  for(q == mem.find(first);q != mem.end();++q) {

    if(q->first > last) break;
    stringstream ss;
    ss<<"@"<<hex<<setw(8)<<setfill('0')<<q->first;

    line_t v = q->second;
    reverse(v.begin(),v.end());

    stringstream ss2;
    for(size_t i=0;i<v.size();++i) {
      string sep = i == 0 ? "" : "_";
      ss2<<sep<<hex<<setw(8)<<setfill('0')<<v[i];
    }
    out<<ss.str()<<" "<<ss2.str()<<endl;

  }
}
