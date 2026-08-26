// --------------------------------------------------------------------
// FILE:    diag.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "diag.h"

namespace cgen
{

// --------------------------------------------------------------------
Diag::Diag(Sev sev,
           const std::string &file,
           const std::string &path,
           const std::string &code,
           const std::string &message)
  : sev_(sev),
    file_(file),
    path_(path),
    code_(code),
    message_(message)
{}

// --------------------------------------------------------------------
std::string Diag::sev_text(Sev sev)
{
  switch(sev) {
    case Sev::Error: return "error";
    case Sev::Warn:  return "warning";
    case Sev::Info:  return "info";
  }
  return "unknown";
}

// --------------------------------------------------------------------
// The site comes first so the line reads like a compiler message.
// --------------------------------------------------------------------
std::string Diag::format() const
{
  std::string s = file_.empty() ? std::string("<no file>") : file_;
  if(!path_.empty()) s += ":" + path_;
  s += ": [" + code_ + "] " + message_;
  return s;
}

} // namespace cgen
