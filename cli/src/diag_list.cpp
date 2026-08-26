// --------------------------------------------------------------------
// FILE:    diag_list.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "diag_list.h"
#include "msg.h"

namespace cgen
{

// --------------------------------------------------------------------
void DiagList::add(const Diag &d)
{
  list_.push_back(d);
  if(d.is_error()) {
    ++errors_;
    if(eoe_) {
      msg->emsg(d.format());
      throw Halt();
    }
  }
}

// --------------------------------------------------------------------
void DiagList::error(const std::string &file,
                     const std::string &path,
                     const std::string &code,
                     const std::string &message)
{
  add(Diag(Diag::Sev::Error, file, path, code, message));
}

// --------------------------------------------------------------------
void DiagList::warn(const std::string &file,
                    const std::string &path,
                    const std::string &code,
                    const std::string &message)
{
  add(Diag(Diag::Sev::Warn, file, path, code, message));
}

// --------------------------------------------------------------------
void DiagList::info(const std::string &file,
                    const std::string &path,
                    const std::string &code,
                    const std::string &message)
{
  add(Diag(Diag::Sev::Info, file, path, code, message));
}

// --------------------------------------------------------------------
std::vector<Diag> DiagList::with_code(const std::string &code) const
{
  std::vector<Diag> out;
  for(const Diag &d : list_) {
    if(d.code() == code) out.push_back(d);
  }
  return out;
}

// --------------------------------------------------------------------
size_t DiagList::count_code(const std::string &code) const
{
  size_t n = 0;
  for(const Diag &d : list_) {
    if(d.code() == code) ++n;
  }
  return n;
}

// --------------------------------------------------------------------
void DiagList::print() const
{
  for(const Diag &d : list_) {
    switch(d.severity()) {
      case Diag::Sev::Error: msg->emsg(d.format()); break;
      case Diag::Sev::Warn:  msg->wmsg(d.format()); break;
      case Diag::Sev::Info:  msg->imsg(d.format()); break;
    }
  }
}

// --------------------------------------------------------------------
void DiagList::clear()
{
  list_.clear();
  errors_ = 0;
}

} // namespace cgen
