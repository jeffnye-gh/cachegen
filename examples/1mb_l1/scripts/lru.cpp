
#include <iostream>
#include <bitset>
using namespace std;

int main()
{
  bitset <3>way3(0);
  bitset <3>way2(0);
  bitset <3>way1(0);
  bitset <3>way0(0);


  typedef bitset<3> lru;
  vector<lru> indexes;

  
  bitset <3> initLru(0);
  for(size_t i=0;i<16;++i) {
    indexes.push_back(initLru);
  }

  return 0;
}

// access way0     !b2  b1 !b0
// access way1     !b2  b1 !b0
// access way2      b2 !b1 !b0
// access way3      b2 !b1 !b0

   0         b2
 0   0     b1  b0

access way 3 from 0, flip b2 to switch sides, switch b0 to switch half

   1
1    0

access way 2 from 0, flip b2 to switch sides, 

   0
0     0


index   way
0       3      000 -> 0 1 1
1       1      000 -> 1 0 1
2       2      000 -> 0 1 1
3       0      000 -> 1 0 1
4       0      000 -> 
1       1
5       2      000 ->
6       3      000 ->
7       0      000 ->
2       1
3       0
3       1
3       3
6       0
6       0
2       1
