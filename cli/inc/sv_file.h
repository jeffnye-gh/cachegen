// --------------------------------------------------------------------
// FILE:    sv_file.h
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// One emitted file, built in memory and written once. Every file the
// emitter produces goes through here, so the generated header of R-11
// is written in one place and cannot be forgotten on a file.
//
// Nothing in the header varies between two runs of one configuration:
// no timestamp, no host name, no user name, no absolute path and no
// git sha. The source is named by the system name and the BASE name
// of the configuration file, never the path the user typed, because
// that path can be absolute.
// --------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>

namespace cgen
{

class SvFile
{
public:
  // kind selects the comment leader: sv for //, make for #
  enum class Kind { Sv, Make, Flist };

  SvFile(const std::string &name,
         const std::string &system_name,
         const std::string &source_file,
         Kind kind = Kind::Sv);

  // one line, appended as given. Trailing space is trimmed so that
  // an indent written into an otherwise empty line cannot make two
  // emissions differ from each other on invisible text.
  void ln(const std::string &s = "");

  // a run of lines
  void lines(const std::vector<std::string> &v);

  // a banner comment, the width the style rules allow
  void bar();
  void note(const std::string &s);

  const std::string &name() const { return name_; }
  const std::string &system_name() const { return system_; }
  const std::string &text() const { return body_; }

  // header plus body, the exact bytes that reach the disk
  std::string content() const;

  // ------------------------------------------------------------------
  // A file scope wildcard import puts a package's members into $unit,
  // and $unit is shared by every file of one compilation. Four node
  // packages each declaring addr_t and line_addr therefore collide
  // the moment the system is built, and a module silently picks up
  // another node's type.
  //
  // The project verilog style requires the import to be at file scope
  // and forbids the module header form that would have scoped it, so
  // the names themselves are what has to be unique. Every member of a
  // node package carries that node's name.
  //
  // snake prefixes types and functions, camel prefixes constants.
  // ------------------------------------------------------------------
  void set_prefix(const std::string &snake, const std::string &camel);

  // the package members the prefix is applied to, one list, so a
  // declaration and every use of it move together
  static const std::vector<std::string> &members();

  static const int Width = 70;

private:
  std::string head() const;
  std::string lead() const;     // the comment leader for this kind

  std::string name_;
  std::string system_;
  std::string source_;
  Kind        kind_;
  std::string body_;
  std::string snake_;
  std::string camel_;
};

} // namespace cgen
