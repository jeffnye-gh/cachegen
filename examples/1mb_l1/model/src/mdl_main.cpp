#include "mdl.h"
#include <iostream>
using namespace std;

int main(int ac,char **av)
{
  CacheModel cm(ac,av);

  //FIXME should these also be cmdline options
  cm.opts.basicTests       = true;
  cm.opts.basicLruTest     = true;
  cm.opts.basicRdHitTest   = true;
  cm.opts.basicWrHitTest   = true;
  cm.opts.basicRdAllocTest = true;
  cm.opts.basicWrAllocTest = true;
  cm.opts.basicRdEvictTest = true;
  cm.opts.basicWrEvictTest = true;
  if(!cm.runTests(false)) return 1;
  return 0;
}
