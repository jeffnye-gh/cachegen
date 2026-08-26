// --------------------------------------------------------------------
// FILE:    model.h
// SOURCE:  CLI-001
// STATUS:  WORKING
// UPDATED: 2026-08-25
// CONTACT: Jeff Nye
//
// The resolved design. Nodes carry their derived geometry, edges
// carry the port types and link width that resolution recovered.
// No value in here is read from the input, see D-37 and R-8.
// --------------------------------------------------------------------
#pragma once
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace cgen
{

class Model
{
public:
  // one contiguous address field
  struct Field {
    bool     valid{false};
    int      lsb{0};
    int      msb{-1};
    int      bits{0};
    int      shift{0};
    uint64_t mask{0};
  };

  // everything derived for one cache node
  struct Geom {
    bool     valid{false};       // arithmetic completed
    uint64_t capacity_bytes{0};
    uint64_t line_bytes{0};
    int      associativity{0};
    int      banks{0};

    uint64_t sets{0};
    uint64_t sets_per_bank{0};
    uint64_t bytes_per_way{0};

    int offset_bits{0};
    int index_bits{0};
    int tag_bits{0};
    int bank_bits{0};

    Field offset;
    Field index;
    Field tag;

    int         refill_beats{-1};   // -1 means not applicable
    std::string refill_note;
  };

  struct Node {
    std::string name;
    std::string cache;            // cache definition it instantiates
    std::string cache_type;
    std::string indexing;
    std::string file;             // site of the node entry
    std::string path;
    std::string cache_file;       // site of the cache definition
    std::string cache_path;
    const nlohmann::json *body{nullptr};   // the cache definition
    bool  resolved{false};
    Geom  geom;
  };

  struct Edge {
    std::string name;             // may be empty, name is optional
    std::string from;
    std::string to;
    std::string link;
    std::string from_port;
    std::string to_port;
    std::string from_port_type;   // type carried by the node port
    std::string to_port_type;
    std::string link_from_type;   // type the link definition declares
    std::string link_to_type;
    std::string protocol;
    std::string conformance;
    int         width_bytes{0};
    bool        width_known{false};
    std::string file;
    std::string path;
    bool from_ok{false};
    bool to_ok{false};
    bool link_ok{false};
  };

  Node *node(const std::string &n);
  const Node *node(const std::string &n) const;

  std::string label(const Edge &e) const;

  std::vector<Node> nodes;
  std::vector<Edge> edges;

  // edges landing on one port instance, keyed "node.port", T-7
  std::map<std::string, int> occupancy;

  std::string system_name;
  bool has_addressing{false};
  bool has_va_bits{false};
  bool has_page_bytes{false};
  int  pa_bits{0};
  int  va_bits{0};
  int  page_bytes{0};
};

} // namespace cgen
