// --------------------------------------------------------------------
// FILE:    feature_table.cpp
// SOURCE:  CLI-005
// STATUS:  WORKING
// UPDATED: 2026-08-27
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "feature_table.h"
#include <algorithm>

using nlohmann::json;

namespace cgen
{

namespace {

Features *g_active = nullptr;

// ------------------------------------------------------------------
// The declared value, rendered on one line. An object or an array is
// not a leaf, so only scalars reach here.
// ------------------------------------------------------------------
std::string render(const json &v)
{
  if(v.is_string())   return v.get<std::string>();
  if(v.is_boolean())  return v.get<bool>() ? "true" : "false";
  if(v.is_number())   return v.dump();
  if(v.is_null())     return "null";
  return v.dump();
}

// ------------------------------------------------------------------
// The definition name inside a /caches/<def>/... or /links/<def>/...
// pointer, empty when the pointer is neither.
// ------------------------------------------------------------------
std::string def_of(const std::string &ptr, const char *group)
{
  const std::string want = std::string("/") + group + "/";
  if(ptr.compare(0, want.size(), want) != 0) return "";
  const size_t beg = want.size();
  const size_t end = ptr.find('/', beg);
  if(end == std::string::npos) return "";
  return ptr.substr(beg, end - beg);
}

} // namespace

// --------------------------------------------------------------------
Features::Scope::Scope(Features &f)
{
  prev_    = g_active;
  g_active = &f;
}

// --------------------------------------------------------------------
Features::Scope::~Scope() { g_active = prev_; }

// --------------------------------------------------------------------
Features *Features::active() { return g_active; }

// --------------------------------------------------------------------
void Features::record(const std::string &node, const std::string &rel,
                      Level level, const std::string &bench,
                      const std::string &test)
{
  Reg r;
  r.node        = node;
  r.rel         = rel;
  r.cover.level = level;
  r.cover.bench = bench;
  r.cover.test  = test;
  regs_.push_back(r);
}

// --------------------------------------------------------------------
void cov_test(const std::string &node, const std::string &rel,
              Features::Level level, const std::string &bench,
              const std::string &test)
{
  Features *f = Features::active();
  if(f != nullptr) f->record(node, rel, level, bench, test);
}

// --------------------------------------------------------------------
void Features::build(const Model &m, const FieldUse &u,
                     const std::vector<Loader::File> &files)
{
  feats_.clear();

  // the document each file holds, so a pointer can be resolved to the
  // value the configuration declares
  auto value_at = [&](const std::string &file,
                      const std::string &ptr) -> std::string {
    for(const Loader::File &f : files) {
      if(f.disp != file) continue;
      const json::json_pointer p(ptr);
      if(!f.doc.contains(p)) return "";
      return render(f.doc[p]);
    }
    return "";
  };

  for(const FieldUse::Leaf &l : u.leaves()) {
    // ----------------------------------------------------------------
    // A feature belongs to a NODE. A cache definition's fields belong
    // to every topology instance of it, and a link definition's
    // fields belong to the link, which every interface naming it
    // shares.
    // ----------------------------------------------------------------
    std::vector<std::string> owners;
    std::string rel;

    const std::string cdef = def_of(l.ptr, "caches");
    const std::string ldef = def_of(l.ptr, "links");

    if(!cdef.empty()) {
      rel = l.ptr.substr(std::string("/caches/").size() + cdef.size());
      for(const Model::Node &n : m.nodes) {
        if(n.cache == cdef) owners.push_back(n.name);
      }
      if(owners.empty()) owners.push_back(cdef);
    } else if(!ldef.empty()) {
      rel = l.ptr.substr(std::string("/links/").size() + ldef.size());
      owners.push_back("link " + ldef);
    } else {
      continue;   // addressing, includes, file_type: not a node feature
    }

    for(const std::string &owner : owners) {
      Feature ft;
      ft.node     = owner;
      ft.ptr      = l.ptr;
      ft.value    = value_at(l.file, l.ptr);
      ft.consumed = u.was_read(l.file, l.ptr);

      for(const Reg &r : regs_) {
        if(r.node != owner || r.rel != rel) continue;
        bool dup = false;
        for(const Cover &c : ft.covers) {
          if(c.bench == r.cover.bench && c.test == r.cover.test) {
            dup = true;
            break;
          }
        }
        if(!dup) ft.covers.push_back(r.cover);
      }

      // ------------------------------------------------------------
      // R-8. Where there is no test, WHY there is none. The first
      // reason is R-6b's and it is expected to be the common one.
      // ------------------------------------------------------------
      if(ft.covers.empty()) {
        if(!ft.consumed) {
          ft.why = "no stage reads it, so nothing in the emitted "
                   "design moves when it changes. Not testable, "
                   "R-6b";
        } else {
          ft.why = "read by the tool, and its effect is not "
                   "observable from a port. It shapes emitted text "
                   "or a derived width rather than behaviour";
        }
      }

      feats_.push_back(ft);
    }
  }

  std::sort(feats_.begin(), feats_.end(),
            [](const Feature &a, const Feature &b) {
              if(a.node != b.node) return a.node < b.node;
              return a.ptr < b.ptr;
            });
}

// --------------------------------------------------------------------
int Features::tested() const
{
  int n = 0;
  for(const Feature &f : feats_) if(!f.covers.empty()) ++n;
  return n;
}

// --------------------------------------------------------------------
int Features::untested() const
{
  return int(feats_.size()) - tested();
}

} // namespace cgen
