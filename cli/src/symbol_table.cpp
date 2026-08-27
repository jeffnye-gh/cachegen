// --------------------------------------------------------------------
// FILE:    symbol_table.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "symbol_table.h"
#include "diag_codes.h"
#include "msg.h"

using nlohmann::json;

namespace cgen
{

// --------------------------------------------------------------------
std::string SymbolTable::kind_text(Kind k)
{
  switch(k) {
    case Kind::PortType: return "port type";
    case Kind::Cache:    return "cache definition";
    case Kind::Link:     return "link definition";
    case Kind::Node:     return "topology node";
  }
  return "symbol";
}

// --------------------------------------------------------------------
const std::map<std::string, SymbolTable::Entry> &
SymbolTable::of(Kind k) const
{
  switch(k) {
    case Kind::PortType: return port_types_;
    case Kind::Cache:    return caches_;
    case Kind::Link:     return links_;
    case Kind::Node:     return nodes_;
  }
  return caches_;
}

// --------------------------------------------------------------------
std::map<std::string, SymbolTable::Entry> &SymbolTable::table(Kind k)
{
  switch(k) {
    case Kind::PortType: return port_types_;
    case Kind::Cache:    return caches_;
    case Kind::Link:     return links_;
    case Kind::Node:     return nodes_;
  }
  return caches_;
}

// --------------------------------------------------------------------
const SymbolTable::Entry *SymbolTable::find(Kind k,
                                            const std::string &name) const
{
  const std::map<std::string, Entry> &m = of(k);
  auto it = m.find(name);
  return it == m.end() ? nullptr : &it->second;
}

// --------------------------------------------------------------------
// First definition wins so that resolution can continue after the
// duplicate is reported. Both sites are named, T-2.
// --------------------------------------------------------------------
void SymbolTable::add(Kind k, const std::string &name,
                      const std::string &file, const std::string &path,
                      const json *body)
{
  std::map<std::string, Entry> &m = table(k);

  auto it = m.find(name);
  if(it != m.end()) {
    diags_.error(file, path, code::t2_duplicate,
                 "duplicate " + kind_text(k) + " " + msg->tq(name) +
                 ", first defined at " + it->second.file +
                 it->second.path);
    return;
  }

  Entry e;
  e.name = name;
  e.kind = k;
  e.file = file;
  e.path = path;
  e.body = body;
  m[name] = e;
}

// --------------------------------------------------------------------
void SymbolTable::scan_map(const Loader::File &f, const char *key, Kind k)
{
  if(!f.doc.contains(key) || !f.doc[key].is_object()) return;

  const json &m = f.doc[key];
  for(auto it = m.begin(); it != m.end(); ++it) {
    add(k, it.key(), f.disp,
        std::string("/") + key + "/" + it.key(), &it.value());
  }
}

// --------------------------------------------------------------------
void SymbolTable::scan_addressing(const Loader::File &f)
{
  if(!f.doc.contains("addressing") || !f.doc["addressing"].is_object()) {
    return;
  }
  const json &a = f.doc["addressing"];

  int pa   = a.value("pa_bits", 0);
  bool hv  = a.contains("va_bits")    && a["va_bits"].is_number_integer();
  bool hp  = a.contains("page_bytes") && a["page_bytes"].is_number_integer();
  int va   = hv ? a["va_bits"].get<int>()    : 0;
  int page = hp ? a["page_bytes"].get<int>() : 0;

  if(!have_addr_) {
    have_addr_  = true;
    pa_bits_    = pa;
    have_va_    = hv;
    va_bits_    = va;
    have_page_  = hp;
    page_bytes_ = page;
    addr_file_  = f.disp;
    return;
  }

  if(pa != pa_bits_ || hv != have_va_ || va != va_bits_ ||
     hp != have_page_ || page != page_bytes_) {
    diags_.error(f.disp, "/addressing", code::topology_addressing,
                 "addressing disagrees with the block in " + addr_file_);
  }
}

// --------------------------------------------------------------------
void SymbolTable::build(const std::vector<Loader::File> &files)
{
  for(const Loader::File &f : files) {
    if(f.declared == "ports") {
      scan_map(f, "port_types", Kind::PortType);
    } else if(f.declared == "caches") {
      scan_map(f, "caches", Kind::Cache);
    } else if(f.declared == "links") {
      scan_map(f, "links", Kind::Link);
    } else if(f.declared == "topology") {
      scan_map(f, "nodes", Kind::Node);
      scan_addressing(f);
    }
  }
}

} // namespace cgen
