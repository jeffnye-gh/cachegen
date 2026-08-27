// --------------------------------------------------------------------
// FILE:    replacement.cpp
// SOURCE:  CLI-004
// STATUS:  WORKING
// UPDATED: 2026-08-26
// CONTACT: Jeff Nye
//
// The tree PLRU encoding, decided here and applied everywhere. There
// is no xxx_decisions.md file for it to live in, D-40, so it is
// stated at its one implementation and reported under R-9.
//
//   The state of a set is ways-1 bits, one per interior node of a
//   binary tree whose leaves are the ways. Node 1 is the root and
//   node k has children 2k and 2k+1, so node k's bit is state[k-1].
//
//   A NODE BIT IS THE DIRECTION TO THE VICTIM. 0 points at the left
//   subtree, 1 points at the right. Finding a victim is a walk from
//   the root following the bits. Touching a way is the same walk
//   with every bit on the path set to point AWAY from that way, so
//   the way just touched is as far from the pointer as the tree can
//   put it.
//
//   The reset state is all zeros, which points every node left and
//   makes way 0 the first victim of every set.
// --------------------------------------------------------------------
#include "replacement.h"

namespace cgen
{

namespace {

std::string bits_lit(int width, unsigned v)
{
  return std::to_string(width) + "'d" + std::to_string(v);
}

} // namespace

// --------------------------------------------------------------------
int Replacement::log2i(int v)
{
  int n = 0;
  while((1 << n) < v) ++n;
  return n;
}

// --------------------------------------------------------------------
Replacement::Replacement(const std::string &policy, int ways)
  : policy_(policy), ways_(ways < 1 ? 1 : ways)
{
  way_bits_ = log2i(ways_);
  if(way_bits_ < 1) way_bits_ = 1;   // a one way cache still needs a
                                     // way signal that can be zero

  if(policy_ == "tree_plru") {
    state_bits_ = ways_ > 1 ? ways_ - 1 : 1;
    const int states = ways_ > 1 ? (1 << state_bits_) : 2;
    tabled_ = states <= MaxTableStates;
  } else if(policy_ == "lru") {
    state_bits_ = ways_ * way_bits_;
  } else if(policy_ == "fifo" || policy_ == "random") {
    state_bits_ = way_bits_;
  } else {
    ok_  = false;
    why_ = "replacement policy '" + policy_ +
           "' is not one the emitter builds";
    state_bits_ = way_bits_;
  }
}

// --------------------------------------------------------------------
// The walk. state[k-1] is node k, 0 goes left.
// --------------------------------------------------------------------
int Replacement::plru_victim(unsigned state) const
{
  if(ways_ <= 1) return 0;
  int node = 1;
  while(node < ways_) {
    const int bit = int((state >> (node - 1)) & 1u);
    node = 2 * node + bit;
  }
  return node - ways_;
}

// --------------------------------------------------------------------
// The same walk, writing every node on the path to point away from
// the way that was touched.
// --------------------------------------------------------------------
int Replacement::plru_update(unsigned state, int way) const
{
  if(ways_ <= 1) return 0;
  unsigned s    = state;
  int      node = ways_ + way;
  while(node > 1) {
    const int parent = node / 2;
    const int mask   = 1 << (parent - 1);
    // came from the left child, so point right, and the other way
    if(node == 2 * parent) s = unsigned(int(s) | mask);
    else                   s = unsigned(int(s) & ~mask);
    node = parent;
  }
  return int(s);
}

// --------------------------------------------------------------------
std::string Replacement::reset_value() const
{
  if(policy_ == "lru") {
    // ------------------------------------------------------------
    // Rank ways-1-i for way i. Way 0 therefore carries the oldest
    // rank and is the victim of a cold set, which is the SAME cold
    // behaviour tree PLRU has out of its all zero reset.
    //
    // The other way round, rank i for way i, is just as valid a
    // permutation and makes the highest numbered way the first
    // victim. It was rejected because a cold cache would then fill
    // differently depending on which policy the configuration
    // named, and a testbench cannot predict a fill order that
    // moves with the policy.
    // ------------------------------------------------------------
    std::string s = "{";
    for(int w = ways_ - 1; w >= 0; --w) {
      s += bits_lit(way_bits_, unsigned(ways_ - 1 - w));
      if(w > 0) s += ", ";
    }
    s += "}";
    return s;
  }
  return "'0";
}

// --------------------------------------------------------------------
std::vector<std::string> Replacement::package_text() const
{
  std::vector<std::string> o;

  o.push_back("  // ----------------------------------------------"
              "------------------");
  o.push_back("  // Replacement, policy " + policy_ + ", " +
              std::to_string(ways_) + " ways.");
  o.push_back("  //");
  o.push_back("  // Built by cgen and emitted here so that the cache "
              "control, the");
  o.push_back("  // testbench and the self checking tests all read one "
              "encoding.");
  o.push_back("  // D-39, generated not transcribed.");
  o.push_back("  // ----------------------------------------------"
              "------------------");
  o.push_back("  localparam int unsigned ReplBits = " +
              std::to_string(state_bits_) + ";");
  o.push_back("");
  o.push_back("  typedef logic [ReplBits-1:0] repl_state_t;");
  o.push_back("");
  {
    // wrapped when the value is long, for the reason the package
    // gives: the node prefix arrives after this line is built
    const std::string v = reset_value();
    if(v.size() > 24) {
      o.push_back("  localparam repl_state_t ReplReset =");
      o.push_back("      " + v + ";");
    } else {
      o.push_back("  localparam repl_state_t ReplReset = " + v + ";");
    }
  }
  o.push_back("");

  if(policy_ == "tree_plru")                       tree_plru(o);
  else if(policy_ == "lru")                        lru(o);
  else if(policy_ == "fifo" || policy_ == "random") counter(o);
  else {
    o.push_back("  // " + why_);
    counter(o);
  }

  return o;
}

// --------------------------------------------------------------------
void Replacement::tree_plru(std::vector<std::string> &o) const
{
  const int states = ways_ > 1 ? (1 << state_bits_) : 1;

  o.push_back("  localparam int unsigned ReplStates = " +
              std::to_string(states) + ";");
  o.push_back("");

  if(ways_ <= 1) {
    o.push_back("  // one way, the victim is way 0 and the state "
                "never moves");
    o.push_back("  function automatic way_t repl_victim"
                "(input repl_state_t s);");
    o.push_back("    repl_victim = way_t'(s) & way_t'(0);");
    o.push_back("  endfunction");
    return;
  }

  if(!tabled_) {
    // ----------------------------------------------------------------
    // Above the table threshold the same walk is emitted as the walk
    // itself, one generated line per tree level. Same encoding, same
    // results, and it does not put ways * 2**(ways-1) entries into a
    // source file.
    // ----------------------------------------------------------------
    o.push_back("  // " + std::to_string(states) + " states is above "
                "the table threshold of " +
                std::to_string(MaxTableStates) + ",");
    o.push_back("  // so the walk is emitted instead of the table it "
                "would fill.");
    o.push_back("");
    o.push_back("  function automatic way_t repl_victim"
                "(input repl_state_t s);");
    o.push_back("    way_t v;");
    o.push_back("    v = '0;");
    for(int lvl = 0; lvl < way_bits_; ++lvl) {
      const int  b    = way_bits_ - 1 - lvl;
      const int  base = (1 << lvl) - 1;
      std::string idx = std::to_string(base);
      if(lvl > 0) {
        idx += " + int'(v[" + std::to_string(way_bits_ - 1) + ":" +
               std::to_string(b + 1) + "])";
      }
      o.push_back("    v[" + std::to_string(b) + "] = s[" + idx + "];");
    }
    o.push_back("    repl_victim = v;");
    o.push_back("  endfunction");
    o.push_back("");
    o.push_back("  function automatic repl_state_t repl_update");
    o.push_back("      (input repl_state_t s, input way_t w);");
    o.push_back("    repl_state_t n;");
    o.push_back("    n = s;");
    for(int lvl = 0; lvl < way_bits_; ++lvl) {
      const int  b    = way_bits_ - 1 - lvl;
      const int  base = (1 << lvl) - 1;
      std::string idx = std::to_string(base);
      if(lvl > 0) {
        idx += " + int'(w[" + std::to_string(way_bits_ - 1) + ":" +
               std::to_string(b + 1) + "])";
      }
      o.push_back("    n[" + idx + "] = ~w[" + std::to_string(b) + "];");
    }
    o.push_back("    repl_update = n;");
    o.push_back("  endfunction");
    return;
  }

  // ------------------------------------------------------------------
  // The tables. Both are filled by running the walk in the tool, so
  // the encoding above is the only place it is written down.
  // ------------------------------------------------------------------
  o.push_back("  // The victim of every state, and the state that "
              "every touch");
  o.push_back("  // leaves. Both filled by cgen from the one walk, "
              "so a consumer");
  o.push_back("  // cannot hold a different encoding from the one "
              "the tool used.");
  o.push_back("  localparam way_t ReplVictimTable [0:ReplStates-1] = '{");

  std::string line = "   ";
  for(int s = 0; s < states; ++s) {
    const std::string e = " " + bits_lit(way_bits_,
                                unsigned(plru_victim(unsigned(s)))) +
                          (s + 1 < states ? "," : "");
    if(line.size() + e.size() > 72) { o.push_back(line); line = "   "; }
    line += e;
  }
  o.push_back(line);
  o.push_back("  };");
  o.push_back("");

  o.push_back("  localparam repl_state_t");
  o.push_back("      ReplUpdateTable [0:ReplStates-1][0:Ways-1] = '{");
  for(int s = 0; s < states; ++s) {
    std::string row = "    '{";
    for(int w = 0; w < ways_; ++w) {
      row += bits_lit(state_bits_,
                      unsigned(plru_update(unsigned(s), w)));
      if(w + 1 < ways_) row += ", ";
    }
    row += "}";
    if(s + 1 < states) row += ",";
    o.push_back(row);
  }
  o.push_back("  };");
  o.push_back("");

  o.push_back("  function automatic way_t repl_victim"
              "(input repl_state_t s);");
  o.push_back("    repl_victim = ReplVictimTable[s];");
  o.push_back("  endfunction");
  o.push_back("");
  o.push_back("  function automatic repl_state_t repl_update");
  o.push_back("      (input repl_state_t s, input way_t w);");
  o.push_back("    repl_update = ReplUpdateTable[s][w];");
  o.push_back("  endfunction");
}

// --------------------------------------------------------------------
// True LRU as a rank per way. Rank 0 is the most recently used and
// rank ways-1 is the victim. A touch drops the touched way to 0 and
// pushes every way that ranked above it down by one.
// --------------------------------------------------------------------
void Replacement::lru(std::vector<std::string> &o) const
{
  o.push_back("  // Rank per way, " + std::to_string(way_bits_) +
              " bits each. Rank 0 is the most recently");
  o.push_back("  // used and rank " + std::to_string(ways_ - 1) +
              " is the victim. The ranks are a permutation");
  o.push_back("  // out of reset and every update keeps them one.");
  o.push_back("  //");
  o.push_back("  // Out of reset way 0 holds the oldest rank, so a");
  o.push_back("  // cold set fills from way 0 upward, which is what");
  o.push_back("  // tree PLRU does from its all zero reset. The cold");
  o.push_back("  // fill order does not move with the policy.");
  o.push_back("  localparam int unsigned RankBits = " +
              std::to_string(way_bits_) + ";");
  o.push_back("");
  o.push_back("  function automatic logic [RankBits-1:0] repl_rank");
  o.push_back("      (input repl_state_t s, input way_t w);");
  o.push_back("    repl_rank = s[int'(w)*RankBits +: RankBits];");
  o.push_back("  endfunction");
  o.push_back("");
  o.push_back("  function automatic way_t repl_victim"
              "(input repl_state_t s);");
  o.push_back("    repl_victim = '0;");
  o.push_back("    for(int unsigned i = 0; i < Ways; i++) begin");
  o.push_back("      if(repl_rank(s, way_t'(i)) == "
              "RankBits'(Ways-1)) begin");
  o.push_back("        repl_victim = way_t'(i);");
  o.push_back("      end");
  o.push_back("    end");
  o.push_back("  endfunction");
  o.push_back("");
  o.push_back("  function automatic repl_state_t repl_update");
  o.push_back("      (input repl_state_t s, input way_t w);");
  o.push_back("    logic [RankBits-1:0] hit_rank;");
  o.push_back("    logic [RankBits-1:0] r;");
  o.push_back("    repl_update = s;");
  o.push_back("    hit_rank    = repl_rank(s, w);");
  o.push_back("    for(int unsigned i = 0; i < Ways; i++) begin");
  o.push_back("      r = repl_rank(s, way_t'(i));");
  o.push_back("      if(way_t'(i) == w) begin");
  o.push_back("        repl_update[i*RankBits +: RankBits] = '0;");
  o.push_back("      end else if(r < hit_rank) begin");
  o.push_back("        repl_update[i*RankBits +: RankBits] = "
              "r + RankBits'(1);");
  o.push_back("      end");
  o.push_back("    end");
  o.push_back("  endfunction");
}

// --------------------------------------------------------------------
// fifo is a per set round robin pointer. random is the same shape
// driven by a shift of the pointer, which is deterministic and
// therefore reproducible in a testbench.
// --------------------------------------------------------------------
void Replacement::counter(std::vector<std::string> &o) const
{
  o.push_back("  // A pointer per set. " +
              (policy_ == "fifo"
                   ? std::string("fifo advances it by one on every "
                                 "allocation.")
                   : std::string("random advances it by a stride so "
                                 "that")));
  if(policy_ != "fifo") {
    o.push_back("  // the sequence is spread but still reproducible "
                "in a testbench.");
  }
  o.push_back("");
  o.push_back("  function automatic way_t repl_victim"
              "(input repl_state_t s);");
  o.push_back("    repl_victim = way_t'(s);");
  o.push_back("  endfunction");
  o.push_back("");
  o.push_back("  function automatic repl_state_t repl_update");
  o.push_back("      (input repl_state_t s, input way_t w);");
  if(policy_ == "fifo") {
    o.push_back("    repl_update = repl_state_t'(s + 1);");
  } else {
    o.push_back("    repl_update = repl_state_t'(s + 3);");
  }
  o.push_back("    if(w == '0) begin end   // fifo and random ignore "
              "the way that was touched");
  o.push_back("  endfunction");
}

} // namespace cgen
