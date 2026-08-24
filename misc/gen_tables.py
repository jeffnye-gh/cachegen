#!/usr/bin/env python3
"""Reference geometry + tree-PLRU solve. Tables are constructed, not
transcribed -- this is the piece VOCABULARY.md section 6 is about."""
import json, math, hashlib, subprocess

def plru_tree(ways):
    """Ways must be a power of two. Node i has children 2i+1, 2i+2.
    Bit value selects the subtree that is the victim side: 0 -> left.
    Update sets each node on the path to point away from the way used."""
    nodes = ways - 1
    depth = int(math.log2(ways))

    def path(way):
        """Nodes visited and the direction taken, root to leaf."""
        n, out = 0, []
        for lvl in range(depth):
            # bit of `way` from MSB: 0 -> left subtree, 1 -> right
            d = (way >> (depth - 1 - lvl)) & 1
            out.append((n, d))
            n = 2 * n + 1 + d
        return out

    update = []
    for w in range(ways):
        row = ["hold"] * nodes
        for n, d in path(w):
            row[n] = "1" if d == 0 else "0"   # point away from w
        update.append(row)

    victim = []
    for state in range(1 << nodes):
        n = 0
        for _ in range(depth):
            b = (state >> n) & 1
            n = 2 * n + 1 + b
        victim.append(n - (nodes))           # leaf index -> way
    return nodes, depth, update, victim

def field(bits, lsb):
    return {"bits": bits, "lsb": lsb, "msb": lsb + bits - 1,
            "mask": hex((1 << bits) - 1), "shift": lsb}

cfg = json.load(open("example_pacino_icache.json"))
g, a, i = cfg["geometry"], cfg["addressing"], cfg["interfaces"]

line, assoc, cap = g["line_bytes"], g["associativity"], g["capacity_bytes"]
off_b   = int(math.log2(line))
lines   = cap // line
sets    = lines // assoc
idx_b   = int(math.log2(sets))
tag_b   = a["pa_bits"] - idx_b - off_b
bpw     = cap // assoc
pg_b    = int(math.log2(a["page_bytes"]))

nodes, depth, upd, vic = plru_tree(assoc)

data_bits = cap * 8
tag_bits  = sets * assoc * tag_b
val_bits  = sets * assoc
rpl_bits  = sets * nodes
ovh       = tag_bits + val_bits + rpl_bits
beats     = (line * 8) // i["refill_width_bits"]

amap = (f"  {'t'*tag_b}{'i'*idx_b}{'o'*off_b}\n"
        f"  tag   [{a['pa_bits']-1}:{idx_b+off_b}]  {tag_b} bits\n"
        f"  index [{idx_b+off_b-1}:{off_b}]  {idx_b} bits\n"
        f"  off   [{off_b-1}:0]   {off_b} bits")

sha = hashlib.sha256(open("example_pacino_icache.json","rb").read()).hexdigest()
now = subprocess.check_output(["date","-u","+%Y-%m-%dT%H:%M:%SZ"]).decode().strip()

out = {
  "provenance": {
    "tool_version": "0.1.0-draft",
    "input_schema_version": cfg["schema_version"],
    "elaborated_schema_version": "1.0.0",
    "input_path": "example_pacino_icache.json",
    "input_sha256": sha,
    "generated_utc": now
  },
  "input": cfg,
  "derived": {
    "sets": sets, "lines_total": lines, "bytes_per_way": bpw,
    "data_array_bits": data_bits, "tag_array_bits": tag_bits,
    "overhead_bits": ovh, "overhead_percent": round(100*ovh/data_bits, 2),
    "offset": field(off_b, 0),
    "index":  field(idx_b, off_b),
    "tag":    field(tag_b, idx_b + off_b),
    "replacement_state": {
      "bits_per_set": nodes, "tree_depth": depth,
      "update_table": upd, "victim_table": vic
    },
    "refill_beats": beats,
    "beat_index_bits": max(1, int(math.log2(beats))) if beats > 1 else 0,
    "mshr": {
      "index_bits": int(math.log2(cfg["miss_handling"]["mshrs"])),
      "target_index_bits": int(math.log2(cfg["miss_handling"]["mshr_targets"])),
      "blocking": cfg["miss_handling"]["mshrs"] == 0
    },
    "vipt": {
      "page_offset_bits": pg_b,
      "index_offset_budget_used": idx_b + off_b,
      "budget_headroom_bits": pg_b - (idx_b + off_b),
      "translated_index_bits": max(0, (idx_b + off_b) - pg_b)
    },
    "address_map": amap
  },
  "checks": [
    {"id":"GEO-1","result":"pass" if cap & (cap-1)==0 else "fail",
     "description":"capacity_bytes is a power of two"},
    {"id":"GEO-3","result":"pass" if sets & (sets-1)==0 else "fail",
     "description":"sets is a power of two"},
    {"id":"GEO-4","result":"pass" if sets % g["banks"]==0 else "fail",
     "description":"banks divides sets"},
    {"id":"ADDR-1","result":"pass" if off_b+idx_b+tag_b==a["pa_bits"] else "fail",
     "description":"offset + index + tag == pa_bits"},
    {"id":"VIPT-1","result":"pass" if bpw <= a["page_bytes"] else "fail",
     "description":"bytes_per_way <= page_bytes",
     "detail":f"bytes_per_way {bpw}, page {a['page_bytes']}, headroom {pg_b-(idx_b+off_b)} bits"},
    {"id":"IF-1","result":"pass" if i["core_read_width_bits"] <= line*8 else "fail",
     "description":"core_read_width_bits <= line_bytes * 8"},
    {"id":"IF-2","result":"pass" if (line*8) % i["refill_width_bits"]==0 else "fail",
     "description":"line bits is a multiple of refill_width_bits"},
    {"id":"IF-3","result":"pass" if beats > 1 else "not_applicable",
     "description":"critical_word_first meaningful only when refill_beats > 1",
     "detail":f"refill_beats {beats}"},
    {"id":"RPL-1","result":"pass" if assoc>=2 and assoc&(assoc-1)==0 else "fail",
     "description":"tree_plru requires associativity a power of two >= 2"}
  ]
}
json.dump(out, open("example_pacino_icache.elaborated.json","w"), indent=2)

print(f"sets {sets}  ways {assoc}  bytes/way {bpw}  tag {tag_b}b  index {idx_b}b  offset {off_b}b")
print(f"VIPT budget {idx_b+off_b}/{pg_b} bits, headroom {pg_b-(idx_b+off_b)}")
print(f"PLRU {nodes} bits, depth {depth}, refill {beats} beats")
print(f"overhead {ovh} bits over {data_bits} data = {round(100*ovh/data_bits,2)}%")
print("victim_table:", vic)
print("checks:", " ".join(f"{c['id']}={c['result']}" for c in out["checks"]))
