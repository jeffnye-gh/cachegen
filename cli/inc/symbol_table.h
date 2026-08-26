// --------------------------------------------------------------------
// FILE:    symbol_table.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// One enumeration of every named object across the whole file tree,
// built once at load. This is the linking step of D-06. Duplicate
// definitions are reported with both sites, T-2, see R-5.
// --------------------------------------------------------------------
#pragma once
#include "diag_list.h"
#include "loader.h"
#include <map>
#include <string>
#include <vector>

namespace cgen
{

class SymbolTable
{
public:
  enum class Kind { PortType, Cache, Link, Node };

  struct Entry {
    std::string           name;
    Kind                  kind{Kind::Cache};
    std::string           file;   // display path of the defining file
    std::string           path;   // JSON pointer to the definition
    const nlohmann::json *body{nullptr};
  };

  explicit SymbolTable(DiagList &diags) : diags_(diags) {}

  void build(const std::vector<Loader::File> &files);

  const Entry *find(Kind k, const std::string &name) const;

  const std::map<std::string, Entry> &of(Kind k) const;

  static std::string kind_text(Kind k);

  // addressing block, taken from the first topology file seen
  bool has_addressing() const { return have_addr_; }
  int  pa_bits()    const { return pa_bits_; }
  int  va_bits()    const { return va_bits_; }
  int  page_bytes() const { return page_bytes_; }
  bool has_va_bits()    const { return have_va_; }
  bool has_page_bytes() const { return have_page_; }
  const std::string &addr_file() const { return addr_file_; }

private:
  std::map<std::string, Entry> &table(Kind k);

  void add(Kind k, const std::string &name, const std::string &file,
           const std::string &path, const nlohmann::json *body);

  void scan_map(const Loader::File &f, const char *key, Kind k);
  void scan_addressing(const Loader::File &f);

  DiagList &diags_;

  std::map<std::string, Entry> port_types_;
  std::map<std::string, Entry> caches_;
  std::map<std::string, Entry> links_;
  std::map<std::string, Entry> nodes_;

  bool        have_addr_{false};
  bool        have_va_{false};
  bool        have_page_{false};
  int         pa_bits_{0};
  int         va_bits_{0};
  int         page_bytes_{0};
  std::string addr_file_;
};

} // namespace cgen
