<!-- SPDX-License-Identifier: Apache-2.0                       -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com -->
# CacheGen IA Session Handoff 001
```
 FILE:    cg_ia_handoff-001.md
 SOURCE:  the IA interactive session of 2026-08-23, run in pacino
 STATUS:  starting context for the first cachegen IA session
 UPDATED: 2026-08-23
 CONTACT: Jeff Nye
```

FIRST handoff of a NEW project. There is no predecessor. This session
ran inside the pacino tree and reviewed `tools/cachegen`; the work
moves to `jeffnye-gh/cachegen` and continues there.

Read this for state, decisions and traps. The four spec files listed
in section 2 are the content.

NOTE ON SCOPE: this is a TOOL project. It is C++ or Python that emits
SystemVerilog. Pacino's CLAUDE.md rules apply to what the tool EMITS
under the pacino profile, and NOT to the tool's own source. Do not
apply SystemVerilog style rules to the generator's code.

---

## 0. Where the work is

NOTHING IS BUILT. The output of this session is a specification: an
input JSON schema, an elaborated output schema, a parameter
vocabulary, and a worked example. No generator code exists.

```
  DONE        parameter vocabulary and both JSON schemas
  DONE        worked pacino icache example, input and elaborated
  DONE        reference geometry + PLRU solve (gen_tables.py)
  NOT STARTED the generator itself
  NOT STARTED emission templates
  NOT STARTED the C++ reference model retarget
```

Two questions were asked and NOT answered before the session ended.
They gate the first real task. See section 8.

---

## 1. What the existing repo actually contains -- do not re-derive

`jeffnye-gh/cachegen` at sha 6770694. 173 files.

THE GENERATOR IS NOT IN THE REPO. The README says so directly: the
cmd-line and Qt5 tool is "in progress but not in this repo yet." What
is present is ONE worked example, `examples/1mb_l1`: a 1MB 4-way
write-back D-cache.

```
  rtl/    Verilog-2001, iverilog -g2012, ~4500 lines
  model/  C++, ~9300 lines, of which 5262 is vendored jsoncpp
  fpga/   Quartus Cyclone V project
```

Two binaries:

```
  bin/cgen   writes .memh/.memb images, tc_1.json, a datasheet
  bin/cmdl   reads tc_1.json via --load_json, runs the model
```

`cgen` DOES NOT EMIT RTL. The RTL in the example was hand-written.
`scripts/hacks.py` is a nine-line generator that emitted the 32
byte-lane assignments in `dsram.v` -- the only generated RTL in the
repo, and proof the approach works on the easy axis.

VERIFIED BY GREP, do not re-check: the RTL side NEVER reads the JSON.
Zero hits across `rtl/Makefile`, `rtl/inc/*.h`, `rtl/src/*.v`. The
RTL gets geometry from hardcoded localparams and data from
`$readmemh`. `tc_1.json` is purely a cgen -> cmdl parameter handoff
plus a file manifest.

---

## 2. Context to load, in this order

The four spec files. These ARE the deliverable of session 001.

```
  VOCABULARY.md                          START HERE. Prose
                                         vocabulary, the checks
                                         JSON Schema cannot do,
                                         scope and exclusions.
  cache_config.schema.json               input, JSON Schema 2020-12
  cache_elaborated.schema.json           output
  example_pacino_icache.json             worked input
  example_pacino_icache.elaborated.json  its generated solve
  gen_tables.py                          reference solve that
                                         produced the elaborated file
```

From the old repo, read ONLY these:

```
  model/inc/options.h        the parameter set and derived geometry
  model/src/options.cpp      loadFromJson, and the bug in section 5
  model/src/gen.cpp          createDataSheet, createJsonFile
  examples/1mb_l1/rtl/README.txt   the PLRU truth tables and the
                                   address decomposition. Best
                                   single document in the old repo.
```

Do NOT load `cache.v` (531 lines), `fsm.v` (406) or `tests.h` (1512)
unless the task is specifically about the old datapath. They are
D-cache specific and largely not reusable.

---

## 3. Decisions made this session -- do not relitigate

**Workflow is generate-and-adapt.** A deterministic tool emits the
bulk; the IA reviews, adapts and integrates. Rationale: context is
the binding constraint. Generating thousands of lines of RTL through
the IA burns context linearly and fails by drift between artifacts
that must agree. Jeff's framing, and it is correct.

**`generated` becomes a third task Mode** alongside automated and
manual. It is a task mode, NOT a new agent role. Do not add a role to
any Roles section; nothing about the agent changed.

**Input JSON lives fenced inside the planning document** that
justifies it, labelled ```` ```json cachegen ````. The prose around it
is the rationale and stays the source of truth for why. A separate
config file elsewhere in the tree would be a second thing that must
agree with the prose, with nothing making it agree. The tool also
accepts a standalone .json for non-planning use.

**Input and elaborated output are separate artifacts.** Input is
sparse intent, no derived values, `sets` in the input is rejected as
an unknown key. Elaborated output is the full solve plus provenance,
manifest and check results, machine-written, never hand-edited.

**No `value` + `units` pairs anywhere in the input.** Bytes and bits
as integers. This kills a whole bug class; see section 5.

**`profile` is an emission backend, not a flag.** `pacino` versus
`generic`. This split must exist before the first template is
written; retrofitting means rewriting every template.

**Ownership line: generic core, project-specific edges.** The
generator owns geometry, arrays, PLRU construction, tag compare, way
select, mechanical MSHR bookkeeping, Makefile, golden vectors,
datasheet. The IA owns the IFU and ITLB boundary, fence.i semantics,
the assertion set, corner-case stimulus, and anything a pacino
decision document constrains specifically.

**Replacement tables are constructed, never transcribed.** The old
repo keeps the same 4-way PLRU truth table in four places --
`README.txt`, `lrurf.v`, `compare.v`, `mdl.h::getLruWay()` -- and
they happen to agree. Four hand-maintained copies that happen to
agree is not one source.

**Sequencing advice was WITHDRAWN.** The IA first recommended
hand-building the pacino icache before writing the generator, to get
two reference designs. That argument optimises for generator
autonomy and weakens under generate-and-adapt, where the IA covers
the gap. Do not re-raise it. What survives: the old D-cache is a free
second data point, worth reading, not worth rebuilding.

---

## 4. What is reusable, and what is not

REUSABLE, and the reason to keep the old repo at all:

```
  options.h / options.cpp / gen.cpp geometry solve
      capacity + line + assoc -> every tag/index/offset
      msb, lsb, mask, shift. Plus the datasheet and JSON
      emission. Strongest piece in the repo.
  the golden-file verification methodology
      C++ model produces reference array images and an expected
      capture stream; the TB loads them and compares array by
      array at test end. Transfers to Verilator directly.
      utils.h: check_data_arrays, check_tb_tags_bits,
      check_main_memory, check_tb_capture_info
  rtl/README.txt PLRU spec and address decomposition
  compare.v victim select: leftmost invalid, else LRU
  valid bits in flops, not SRAM, so invalidate-all is one cycle
  rtl/docs/*.json wavedrom timing diagrams
```

NOT REUSABLE:

```
  the entire RTL datapath. It is a D-cache: write allocate, dirty
      bits, byte-enable merge, evict/writeback, 32-bit word port.
  fsm.v. Blocking, one outstanding miss. Does not evolve into
      MSHR control; it is a different machine.
  32-bit addresses throughout the C++ (uint32_t). RVA23 needs
      up to 56-bit PA. Pervasive type change.
  l1_tag_type accepts only PHYSICAL. No TLB, no VIPT anywhere.
  sram.v is the only cleanly parameterized module, 26 lines,
      and it has a sensitivity bug.
```

---

## 5. Defects found in the old code -- do not rediscover

```
  options.cpp loadFromJson
      mm_capacity = json["mm_capacity_value"].asUInt64();
      Reads the WRONG key. Shipped config has capacity
      4294967296 and capacity_value 4, so a round trip turns
      4GB into 4. Inert only because mm_entries sizes the
      memory. THIS is why the input schema forbids value+units
      pairs and why a round-trip test is required.
  bitrf.v, lrurf.v
      reg [3:0] regs[0:8192] is 8193 entries. Off by one. As a
      template this is [0:SETS] where it must be [0:SETS-1],
      so every emitted geometry would carry it.
  cache.v:234
      "default  mm_writedata = 256'bx;" -- missing colon.
  dut.v
      drives clk, reset, mm_cc_ready as implicit undeclared
      nets. Verilator errors under -Wall.
  sram.v
      always @(a) rd = ram[a]; incomplete sensitivity, and it is
      the stl_sequent pattern pacino's CLAUDE.md warns about.
```

GENERAL LESSON, worth carrying: an emitter bug is one bug per
output. Template defects propagate to every configuration.

---

## 6. Pacino constraints the tool must satisfy

Pacino state as of its session 067, for context only:

```
  bpu   22 rtl modules, 47 targets green (18 lint, 22 sim, 7 cov)
  ftq   11 rtl modules, 22 targets, 670 checks, complete at BP-107
  decode 4 rtl modules
  lib   bw_ram.sv, sram_init.sv, dual_lm1.sv, sat_alu.sv
  NOT STARTED: IFU, icache, ITLB, all of midcore and backend
  docs/frontend/icache/*.md are NINE EMPTY STUBS
```

What binds the emitter:

```
  rtl/lib/rtl/bw_ram.sv    Emit arrays against THIS port
                           interface and its documented SRAM
                           SUBSTITUTION BOUNDARY. Do not invent
                           a new SRAM abstraction.
  rtl/lib/rtl/sram_init.sv the reset/init FSM to wire up
  planning/arch/sram_init.md  the policy behind it
  FTB_BLOCK_BYTES 32       the PREDICTION block
  FETCH_BLOCK_BYTES 64     the fetch width
                           bp_defines_pkg.sv:81 says these must
                           not be collapsed
  ftq_ifu_interfaces.md 8  DECIDED 2026-08-19: pacino defines NO
                           logical ftq_icache interface. The
                           icache is encapsulated behind the IFU.
                           Do not emit an FTQ-facing port.
  every Makefile target runs on every task; make all is
      explicitly insufficient
  planning/, docs/, pa_handoffs/ are IA-READ-ONLY in pacino
```

The icache solve, verified this session:

```
  32KB, 8-way, 64B lines, VIPT, Sv39, 4KB pages
  sets 64   tag 44b   index 6b   offset 6b   bytes/way 4096
  VIPT budget 12/12 bits, HEADROOM ZERO
  PLRU 7 bits, depth 3   refill 2 beats at 256b
  overhead 23488 bits over 262144 data = 8.96%
```

Eight ways is not a preference. It follows from VIPT with 4KB pages
and 64B lines: bytes_per_way must not exceed the page size, so a way
caps at 4KB and 32KB needs eight of them.

---

## 7. What was verified this session -- do not re-run

Schema conditionals, 10 negative cases correctly rejected and 1
positive accepted:

```
  icache + write_hit                 rejected
  icache + flush_all                 rejected
  icache + core_write_width_bits     rejected
  VIPT without va_bits               rejected
  mshrs>0 without mshr_targets       rejected
  associativity 6                    rejected
  name "I-Cache"                     rejected
  derived key in input               rejected
  sram_model macro without name      rejected
  dcache missing write policies      rejected
  dcache fully specified             ACCEPTED
```

8-way tree PLRU, four invariants over all 128 states:

```
  a just-accessed way is never the next victim
  touching ways 0..7 in order leaves way 0 as victim
  all 8 ways reachable as victim
  no starvation over 64 successive evictions
```

---

## 8. Open questions -- these gate the first task

TWO WERE ASKED AND NOT ANSWERED. Ask Jeff before starting.

```
  1. Continue jeffnye-gh/cachegen, or start a new repo?
     IA recommended CONTINUE. The existing README already
     promises this tool, and examples/1mb_l1 stays useful as
     the D-cache reference point. Jeff said "I think" he will
     restart in jeffnye-gh/cachegen. Confirm before assuming.
  2. Tool implementation language: C++ to reuse options.cpp
     geometry math, or Python for a faster start? This changes
     the project CLAUDE.md substantially and is not decided.
```

From VOCABULARY.md section 9, all still open:

```
  3. Is emitted RTL regenerated in place, or hand-owned after
     first emission? Decides whether manifest digests drive a
     diff review or sit unused. Close to irreversible once
     adaptation starts. IA recommendation: regenerate, treat
     the diff as the review artifact, require any surviving
     hand edit to be pushed back into the generator in the
     same task.
  4. Is the C++ reference model generated, or written once and
     parameterized at runtime from the elaborated JSON?
  5. Does `banks` belong in geometry or interfaces?
  6. Is the ITLB interface emitted for VIPT, or left to the
     integrator? Currently assumed left to the integrator,
     consistent with the icache being behind the IFU.
```

---

## 9. Next actions, in order

```
  1. Answer questions 1 and 2 of section 8.
  2. Move the repo out of pacino. It is untracked there
     (?? tools/cachegen/), nothing references it, and it
     carries its own .git with the correct remote:
       mv pacino/tools/cachegen jeffnye-gh/cachegen
     History and remote come along. Do not re-clone.
  3. Copy the six spec files into spec/.
  4. Write the project CLAUDE.md. It needs TWO rule sets:
     standards for the tool's own source, and standards for
     what it emits under profile pacino. Without this a fresh
     session will apply SystemVerilog rules to Python.
  5. Answer question 3 before any emission code is written.
  6. First code task: the geometry solver and elaborated-JSON
     writer, ported from options.cpp, widened to 56-bit PA,
     with a round-trip test that writes the config, reads it
     back, and asserts equality. That test is the direct
     answer to the mm_capacity bug and catches the whole
     class.
  7. Second: PLRU tree construction emitting update and victim
     tables, with the four invariants of section 7 as its
     test. gen_tables.py already does this in Python and can
     be the reference.
```

Do NOT start emission templates before 4 and 5 are settled.

---

## 10. Working method

Jeff's stated preferences, carried from pacino CLAUDE.md and this
session:

```
  - State findings plainly. No metaphors, no picturesque
    phrasing, no jargon.
  - Match length to document class. A status entry points at
    something; it does not repeat it.
  - Before raising a topic Jeff did not ask about, check
    whether it is already closed. If closed, do not raise it.
    If you think a closed item is wrong, say so in ONE
    sentence and wait.
  - ASCII only in files and, by preference, in console output.
  - Verify before asserting. This session twice stated
    something about the old tool that turned out to be wrong
    on reading the code -- the JSON's purpose, and the value
    of parameterized output. Read first.
```

---

## 11. Document History

```
  2026-08-23  001  Created. Captures the pacino interactive
                   session that reviewed tools/cachegen and
                   produced the parameter vocabulary and both
                   JSON schemas. No generator code exists yet.
```
