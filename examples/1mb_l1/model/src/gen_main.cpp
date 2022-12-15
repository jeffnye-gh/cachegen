#include "gen.h"
#include <iostream>
using namespace std;

int main(int ac,char **av)
{
  CacheGen cg(ac,av);
  if(!cg.run()) return 1;
  return 0;
}
