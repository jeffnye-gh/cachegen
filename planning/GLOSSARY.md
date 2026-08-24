<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->

```
 FILE:    GLOSSARY.md
 SOURCE:  various
 STATUS:  WORKING
 UPDATED: 2026-08-24
 CONTACT: Jeff Nye
```

# cachegen parameter vocabulary

Draft. Companion to `cache_config.schema.json` (input) and
`cache_elaborated.schema.json` (output).

## 1. Two files, one solve

The input is a sparse statement of intent. Every field in it is a
decision a person made. It contains no derived value.

The elaborated output is the complete solve, written next to the
emitted RTL. It contains the input verbatim, every derived field,
a provenance record, a manifest of what was written, and the result
of every check. It is machine-produced and never hand-edited.

The split exists because a person writing `sets: 64` next to
`capacity_bytes: 32768` is stating the same fact twice, and the two
statements can disagree. The predecessor tool carried
`l1_capacity: 1048576` beside `l1_capacity_value: 1` and
`l1_capacity_units: "MB"`, and its reload path read
`mm_capacity_value` into `mm_capacity`, so a round trip silently
turned 4GB into 4. Derived values belong on one side of the tool
only.

## 2. Where the input lives

The input JSON is a fenced block inside the planning document that
justifies it:

    ```json cachegen
    { ... }
    ```

The prose around the block is the rationale and remains the source
of truth for why. The block is its machine-readable projection. The
tool reads only the block, keyed on the `cachegen` fence label.

This keeps one file, reviewed once, under PA ownership. A separate
config file elsewhere in the tree would be a second thing that must
agree with the prose, with nothing making it agree.

The tool also accepts a standalone `.json` file for use outside a
planning flow. When it extracts from a document instead, provenance
records the document path, its digest, and the fence label.

## 3. Field groups

| Group | Holds | Notes |
|---|---|---|
| top level | name, cache_type, level | `name` drives every emitted filename |
| geometry | capacity, line size, ways, banks | powers of two, checked by the tool |
| addressing | PIPT/VIPT, VA/PA widths, page size | VIPT requires va_bits and page_bytes |
| policies | miss and hit policy, replacement | write fields rejected for icache |
| miss_handling | MSHRs, targets, victim and fill buffers | mshrs 0 selects the blocking FSM |
| interfaces | core and refill widths, next level | refill width sets the beat count |
| maintenance | invalidate and flush support | invalidate_all is fence.i |
| emission | profile, artifacts, SRAM model | profile carries the house style |
| verification | vectors, directed tests, assertions | tests are checked against cache_type |

## 4. Decisions worth stating plainly

**cache_type is structural, not cosmetic.** `icache` selects a
read-only datapath: no dirty bits, no store merge, no writeback path,
no eviction of modified data. The schema rejects `write_hit`,
`write_miss`, `core_write_width_bits` and `flush_all` on an icache
rather than accepting and ignoring them. A parameter that is accepted
and ignored is how a config comes to mean something other than what it
says.

**mshrs: 0 is not a small number of MSHRs.** It selects a different
control structure. A blocking FSM does not become non-blocking by
raising a count, so the emitter branches on it.

**Invalid ways beat the replacement pick, always.** Not a parameter.
Every replacement policy prefers an invalid way; `replacement` only
decides how the victim is chosen once all ways are valid.

**Capacity excludes overhead.** `capacity_bytes` is data only. Tag,
valid, dirty and replacement state are reported in
`derived.overhead_bits`, which is usually the number that surprises
people at floorplan.

**Units are bytes and bits, as integers.** No `value` plus `units`
pair anywhere. The elaborated output may carry a rendered string for
the datasheet; the input never does.

**profile is a backend, not a flag.** `pacino` applies 80 columns,
2-space indent, `clk`/`rstn`, one module per file with the filename
matching, file-scope imports with defines before structs,
`always_comb` over chained `assign`, ASCII-only comments, and the
project Verilator suppression set. `generic` applies none of it. This
split has to exist before the first template is written; retrofitting
it means rewriting every template.

**verification_params leaves a few knobs live.** Fully baked output
cannot be shrunk, and a 32KB instance makes exhaustive array walks
intractable in simulation. When true, the emitter leaves sets and ways
as SystemVerilog parameters so a reduced instance can be built for the
testbench. Costs some of the specialization; buys tractable tests.

## 5. Checks the schema cannot express

JSON Schema validates shape and enumerations. It cannot do arithmetic
across fields. The tool performs these, and records each one in
`checks[]` with a pass, fail, or not_applicable result, so the
elaborated file shows what was checked rather than only that
generation succeeded.

| id | Check |
|---|---|
| `GEO-1` | capacity_bytes is a power of two |
| `GEO-2` | capacity_bytes / (line_bytes * associativity) is an integer >= 1 |
| `GEO-3` | sets is a power of two |
| `GEO-4` | banks divides sets |
| `ADDR-1` | offset_bits + index_bits + tag_bits == pa_bits |
| `ADDR-2` | tag_bits >= 1 |
| `VIPT-1` | index_bits + offset_bits <= log2(page_bytes), i.e. bytes_per_way <= page_bytes. Fails under alias_policy forbid |
| `VIPT-2` | va_bits >= index_bits + offset_bits |
| `IF-1` | core_read_width_bits <= line_bytes * 8 |
| `IF-2` | line_bytes * 8 is a multiple of refill_width_bits |
| `IF-3` | critical_word_first is meaningful only when refill_beats > 1; otherwise not_applicable |
| `MSHR-1` | mshrs <= sets * associativity |
| `RPL-1` | tree_plru requires associativity to be a power of two >= 2 |
| `TEST-1` | every entry in directed_tests applies to this cache_type |
| `EMIT-1` | rtl in emit implies pkg in emit |
| `EMIT-2` | tb in emit implies vectors in emit, or the testbench has no golden data |
| `EMIT-3` | sram_model bw_ram requires profile pacino |

`VIPT-1` is the one that most often changes a design. For 4KB pages
it reduces to bytes_per_way <= 4096, and it is why a 32KB VIPT cache
with 64B lines needs eight ways rather than four.

## 6. Generated, not transcribed

`derived.replacement_state.update_table` and `victim_table` are built
by the tool from the tree construction. They are not copied from a
reference and not hand-written.

This matters because those two tables are where hand-built
replacement logic is most often wrong, and because the RTL, the C++
model, the assertions and the datasheet all need the same table. The
predecessor kept the same 4-way PLRU truth table in four places -
`README.txt`, `lrurf.v`, `compare.v`, and `mdl.h::getLruWay()` - and
they happened to agree. Four hand-maintained copies that happen to
agree is not the same as one source.

The same argument applies to the address field decomposition. Every
field is emitted with bits, lsb, msb, mask and shift together,
redundantly, because the RTL wants slice bounds, the model wants mask
and shift, and the datasheet wants a diagram. Deriving them
separately is how they drift.

## 7. In scope

L1 instruction, L1 data, and L2 unified caches. PIPT and VIPT.
Write-back and write-through. Direct-mapped through 16-way. Tree
PLRU, true LRU, FIFO, random. Blocking and MSHR-based non-blocking
control. Optional victim and fill buffers. Single and multi-bank data
arrays.

## 8. Out of scope

Written down because these are what get argued for later.

- Coherence protocols. No MESI, MOESI, or directory participation.
  `coherence` is not a field. A cache that must participate in a
  protocol is a different generator.
- Sectored and sub-blocked lines.
- ECC and parity on tags or data.
- Compressed cache lines.
- NUCA, multi-bank interconnect, distributed tag directories.
- Synonym handling beyond rejection. `alias_policy` accepts only
  `forbid`. Way prediction, physical-tag recheck and OS page
  colouring are all real answers and none of them are here.
- Prefetch engines. A prefetch request port may be emitted; the
  engine behind it is not generated.
- Software-managed scratchpad or way-locking modes.
- Multi-port cores. One request per cycle per cache.

## 9. Open questions

1. Whether the emitted RTL is regenerated in place or hand-owned
   after first emission. This decides whether the manifest digests
   are used for diff review or ignored, and it is close to
   irreversible in practice. Recommendation: regenerate, treat the
   diff as the review artifact, and require any surviving hand edit
   to be pushed back into the generator within the same task.
2. Whether the C++ reference model is generated or written once and
   parameterized at runtime from the elaborated JSON. The second is
   less work and keeps one model under test; the first keeps the
   single-solve property.
3. Whether `banks` belongs in geometry or interfaces. It is a
   physical decision with an interface consequence.
4. Whether the ITLB interface is emitted for VIPT configurations or
   left to the integrator. Currently assumed left to the integrator,
   consistent with the icache being encapsulated behind the IFU.
