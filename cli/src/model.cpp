// --------------------------------------------------------------------
// FILE:    model.cpp
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
// --------------------------------------------------------------------
#include "model.h"

namespace cgen
{

// --------------------------------------------------------------------
Model::Node *Model::node(const std::string &n)
{
  for(Node &x : nodes) {
    if(x.name == n) return &x;
  }
  return nullptr;
}

// --------------------------------------------------------------------
const Model::Node *Model::node(const std::string &n) const
{
  for(const Node &x : nodes) {
    if(x.name == n) return &x;
  }
  return nullptr;
}

// --------------------------------------------------------------------
// Edge names are optional, fall back to the endpoint pair.
// --------------------------------------------------------------------
std::string Model::label(const Edge &e) const
{
  if(!e.name.empty()) return e.name;
  return e.from + "->" + e.to;
}

} // namespace cgen
