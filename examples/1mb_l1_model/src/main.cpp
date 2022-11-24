#include "cachegen.h"
#include <iostream>
using namespace std;

int main(int ac,char **av)
{
  CacheGen cg(ac,av);
  if(!cg.execute()) return 1;
  return 0;
}
