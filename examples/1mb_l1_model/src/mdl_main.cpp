#include "mdl.h"
#include <iostream>
using namespace std;

int main(int ac,char **av)
{
  CacheModel cm(ac,av);
  cm.opts.info();
//  if(!cm.simulate()) return 1;
  return 0;
}
