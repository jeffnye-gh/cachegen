// --------------------------------------------------------------------
// FILE:    diag.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// A diagnostic is an object, not a string thrown at the point of
// failure. It names the file, a JSON pointer into that document, a
// severity, a stable code and a message. See R-10.
// --------------------------------------------------------------------
#pragma once
#include <string>

namespace cgen
{

class Diag
{
public:
  enum class Sev { Info, Warn, Error };

  Diag(Sev sev,
       const std::string &file,
       const std::string &path,
       const std::string &code,
       const std::string &message);

  Sev severity() const { return sev_; }
  bool is_error() const { return sev_ == Sev::Error; }

  const std::string &file()    const { return file_; }
  const std::string &path()    const { return path_; }
  const std::string &code()    const { return code_; }
  const std::string &message() const { return message_; }

  // "file:pointer: [code] message", the pointer omitted when empty
  std::string format() const;

  static std::string sev_text(Sev sev);

private:
  Sev         sev_;
  std::string file_;
  std::string path_;
  std::string code_;
  std::string message_;
};

} // namespace cgen
