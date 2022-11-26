#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <map>
// ===========================================================================
typedef uint32_t word_t;
typedef std::vector<word_t> line_t;
// ===========================================================================
struct Ram
{
  Ram(std::string _name,uint32_t _entries,uint32_t _width)
    : name(_name),
      entries(_entries),
      width(_width)
  {}

  void info(std::ostream&,uint32_t begin,uint32_t end);

  uint32_t ld(uint32_t a,uint32_t be);
  void     st(uint32_t a,uint32_t be,uint32_t d);

  line_t ld_line(uint32_t a,uint32_t be);
  void   st_line(uint32_t a,uint32_t be,line_t d);

  std::map<uint32_t,line_t>::iterator q;
  std::map<uint32_t,line_t> mem;

  std::string name;
  uint32_t entries;
  uint32_t width;
};
