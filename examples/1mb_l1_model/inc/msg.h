#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <bitset>

struct Msg
{
  //verbose=3 enables emsg, wmsg and imsg
  Msg(std::string _who="",int _verbose=3)
    : w(_who+": "),
      verbose(_verbose)
  {}

  void setWho(std::string _w) {  w = _w+": "; }
  // --------------------------------------------------------------------
  void  msg() { std::cout<<std::endl; }
  void  msg(std::string m) { std::cout<<m<<std::endl;  }
  void mmsg(std::string p,std::string m) { std::cout<<p<<w<<m<<std::endl; }
  // --------------------------------------------------------------------
  void dmsg(std::string m,int v)
    { if(TR && v<=verbose) mmsg("-D: ",m); }
  void dmsg(std::string m="")
    { if(TR) mmsg("-D: ",m); }

  void emsg(std::string m="",int v=1)
    { if(v<=verbose) mmsg("-E: ",m); }

  void imsg(std::string m="",int v=3)
    { if(v<=verbose || v==-1) mmsg("-I: ",m); }

  void wmsg(std::string m="",int v=2)
    { if(v<=verbose) mmsg("-W: ",m); }

  void mmsg(std::ostream& o,std::string p,std::string m)
    { o<<p<<w<<m<<std::endl; }

  // --------------------------------------------------------------------
  //dmsg should be v level 4
  void dmsg(std::ostream& o,std::string m="")
    { if(TR) mmsg(o,"-D: ",m); }

  void emsg(std::ostream& o,std::string m="",int v=1) { mmsg(o,"-E: ",m); }
  void wmsg(std::ostream& o,std::string m="",int v=2) { mmsg(o,"-W: ",m); }
  void imsg(std::ostream& o,std::string m="",int v=3) { mmsg(o,"-I: ",m); }
  // --------------------------------------------------------------------
//  void imsg(std::ostream& o,std::string m,uint32_t a,uint32_t be)
//  {
//    std::bitset<4>  _be(be);
//    std::stringstream ss;
//   ss<<m<<" 0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<a<<" "<<"0b"<<_be;
//
//    mmsg(o,"-I: ",ss.str());
//  }

  // --------------------------------------------------------------------
  std::string w;
  int  verbose;
  bool TR{false};
};

