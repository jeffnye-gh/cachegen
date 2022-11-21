#include "cachegen.h"
#include <iostream>
using namespace std;

int main(int ac,char **av)
{
  cout<<"Begin: cache gen"<<endl;
  CacheGen cg(ac,av);
  cout<<"End:   cache gen"<<endl;
  return 0;
}
