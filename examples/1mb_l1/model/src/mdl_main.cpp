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
  cm.opts.basicRdAllocTest = false;
  cm.opts.basicWrAllocTest = false;
  cm.opts.basicRdEvictTest = false;
  cm.opts.basicWrEvictTest = false;
  if(!cm.runTests(false)) return 1;
  return 0;
}
