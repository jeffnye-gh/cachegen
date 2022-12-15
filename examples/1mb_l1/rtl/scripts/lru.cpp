
#include <algorithm>
#include <iostream>
#include <string>
#include <bitset>
#include <vector>
#include <map>
using namespace std;

// --------------------------------------------------------
map<string,string> lruLut
{
  { "000", "way 3" },
  { "001", "way 3" },
  { "010", "way 2" },
  { "011", "way 2" },
  { "100", "way 1" },
  { "101", "way 0" },
  { "110", "way 1" },
  { "111", "way 0" }
};
// --------------------------------------------------------
string getLru(const string &t)
{
  if(lruLut.find(t) == lruLut.end()) {
     return "???"; 
  }
  return lruLut[t];
}

// --------------------------------------------------------
// --------------------------------------------------------
int main()
{
  vector<string> input = { "010","100","000","011",
                           "000","000","010","001",
                           "111","101","010","001",
                           "010","110","011","000" };

  for_each(input.begin(),input.end(),[](const string &in) 
    { cout << in << " : " <<getLru(in) << '\n'; });

  cout<<"LRU decoder"<<'\n';
  for(auto &[k,v] : lruLut) {
    cout << "3'b"<<k << " : " <<v << '\n';
  }
  return 0;
}
