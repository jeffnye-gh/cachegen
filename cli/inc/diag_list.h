// --------------------------------------------------------------------
// FILE:    diag_list.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// Stages accumulate diagnostics here and they are reported together.
// With exit on error set, the first error throws Halt so the driver
// can stop without every stage growing an early return path.
// --------------------------------------------------------------------
#pragma once
#include "diag.h"
#include <exception>
#include <string>
#include <vector>

namespace cgen
{

class DiagList
{
public:
  // thrown by add() when exit on error is set and an error arrives
  struct Halt : public std::exception {
    const char *what() const noexcept override { return "eoe"; }
  };

  void set_eoe(bool v) { eoe_ = v; }
  bool eoe() const     { return eoe_; }

  void add(const Diag &d);

  void error(const std::string &file,
             const std::string &path,
             const std::string &code,
             const std::string &message);

  void warn(const std::string &file,
            const std::string &path,
            const std::string &code,
            const std::string &message);

  void info(const std::string &file,
            const std::string &path,
            const std::string &code,
            const std::string &message);

  bool   has_error()   const { return errors_ > 0; }
  size_t error_count() const { return errors_; }
  size_t size()        const { return list_.size(); }
  bool   empty()       const { return list_.empty(); }

  const std::vector<Diag> &all() const { return list_; }

  // every diagnostic carrying this code, for the test fixtures
  std::vector<Diag> with_code(const std::string &code) const;
  size_t count_code(const std::string &code) const;

  // one line per diagnostic through the msg singleton
  void print() const;

  void clear();

private:
  std::vector<Diag> list_;
  bool   eoe_{false};
  size_t errors_{0};
};

} // namespace cgen
