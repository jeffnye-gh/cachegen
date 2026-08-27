// --------------------------------------------------------------------
// FILE:    field_use.cpp
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "field_use.h"
#include <algorithm>

using nlohmann::json;

namespace cgen
{

namespace {

FieldUse *g_active = nullptr;

const char Sep = '\x1f';

// ------------------------------------------------------------------
// RFC 6901 escaping. The identifier pattern in every schema is
// ^[a-z][a-z0-9_]*$ so neither character can occur today, and the
// escape is here so a future key with one cannot fold two pointers
// into one.
// ------------------------------------------------------------------
std::string escape(const std::string &key)
{
  std::string o;
  for(char c : key) {
    if(c == '~')      o += "~0";
    else if(c == '/') o += "~1";
    else              o += c;
  }
  return o;
}

// ------------------------------------------------------------------
// Every leaf below one node. A scalar is a leaf. An EMPTY container
// is a leaf as well, so an "include": [] that nothing walks is still
// a field the report can name.
// ------------------------------------------------------------------
void walk(const json &j, const std::string &file, const std::string &at,
          std::vector<FieldUse::Leaf> &out)
{
  if(j.is_object() && !j.empty()) {
    for(auto it = j.begin(); it != j.end(); ++it) {
      walk(it.value(), file, at + "/" + escape(it.key()), out);
    }
    return;
  }
  if(j.is_array() && !j.empty()) {
    for(size_t i = 0; i < j.size(); ++i) {
      walk(j[i], file, at + "/" + std::to_string(i), out);
    }
    return;
  }
  out.push_back({ file, at });
}

} // namespace

// --------------------------------------------------------------------
FieldUse::Scope::Scope(FieldUse &u)
{
  prev_    = g_active;
  g_active = &u;
}

// --------------------------------------------------------------------
FieldUse::Scope::~Scope()
{
  g_active = prev_;
}

// --------------------------------------------------------------------
FieldUse *FieldUse::active()
{
  return g_active;
}

// --------------------------------------------------------------------
void cfg_read(const std::string &file, const std::string &ptr)
{
  FieldUse *u = FieldUse::active();
  if(u != nullptr) u->read(file, ptr);
}

// --------------------------------------------------------------------
void FieldUse::enumerate(const std::vector<Loader::File> &files)
{
  leaves_.clear();
  for(const Loader::File &f : files) {
    walk(f.doc, f.disp, "", leaves_);
  }

  std::sort(leaves_.begin(), leaves_.end(),
            [](const Leaf &a, const Leaf &b) {
              if(a.file != b.file) return a.file < b.file;
              return a.ptr < b.ptr;
            });
}

// --------------------------------------------------------------------
void FieldUse::read(const std::string &file, const std::string &ptr)
{
  const std::string m = file + Sep + ptr;
  if(std::find(marks_.begin(), marks_.end(), m) != marks_.end()) return;
  marks_.push_back(m);
}

// --------------------------------------------------------------------
// A mark covers its own pointer and everything below it, so a stage
// that consumes a whole object marks the object once.
// --------------------------------------------------------------------
bool FieldUse::was_read(const std::string &file,
                        const std::string &ptr) const
{
  const std::string want = file + Sep + ptr;
  for(const std::string &m : marks_) {
    if(m.size() > want.size())                  continue;
    if(want.compare(0, m.size(), m) != 0)        continue;
    if(m.size() == want.size())                  return true;
    if(want[m.size()] == '/')                    return true;
  }
  return false;
}

// --------------------------------------------------------------------
std::vector<FieldUse::Leaf> FieldUse::unread() const
{
  std::vector<Leaf> out;
  for(const Leaf &l : leaves_) {
    if(was_read(l.file, l.ptr)) continue;
    out.push_back(l);
  }
  return out;
}

} // namespace cgen
