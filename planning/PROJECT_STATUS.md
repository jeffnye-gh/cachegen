<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->
# Project Status -- cachegen

```
 FILE:    PROJECT_STATUS.md
 SOURCE:  various
 STATUS:  WORKING
 UPDATED: 2026.08.25
 CONTACT: Jeff Nye
```

Updated every session. Paste into IA or PA at session start, along
with the latest session_handoff-NNN.md and CLAUDE.md.

Paste PROJECT_CORE.md only when methodology is under discussion.

This file is the authoritative list of document status. The values
in use are DRAFT, CLOSED, NOT STARTED and DEPRECATED.

---

## PA session: input schema rebuilt. Five files, no code yet.

NO CODE EXISTS. cgen has not been started. What exists is the input
configuration schema, a worked example, and a task file to build the
front half of the tool.

The schema was rebuilt from scratch during this session, not
patched. The line runs 1.0.0 (audited by INFRA-001), 2.0.0 (audited
by INFRA-002), then a restart at 0.1.0 scoped to one RTL example,
and from there to 0.12.0. Six intermediate versions were produced.
Several repaired defects introduced by their own predecessor.

THE SINGLE FLAT SCHEMA IS RETIRED. The input is now five file types.

```
  system      root, an include list and nothing else
  ports       port type declarations
  caches      cache and memory definitions
  links       connection type definitions
  topology    the graph, plus the addressing block
```

Every file type carries its own include list. The tool performs a
full name enumeration across the whole tree at load. This is a
linking step: undefined names, duplicate definitions and type
mismatches are all resolved there, once.

MEMORY IS A cache_type, not a separate schema. So is `agent`, the
producer/consumer abstraction that lets an edge into an L1 have a
`from`. Neither is synthesizable and both are in the graph because
the emitter needs their ports.

`cache_type` WAS DELETED AND RESTORED. It existed and worked in
1.0.0, with the icache conditional already written. The PA removed
it when scoping a minimal schema to a single D-cache example and did
not report the removal. Four versions were built on the deletion.
The project targets an I-side; a D-cache-only configuration schema
was the wrong artifact for two sessions.

RULE: a field is required only if it applies. Absence means the
group does not apply, and a partly populated group is a tool error.
The prior scheme required every field and then needed six
conditionals to relax it again.

RULE: derived values never appear in the input. sets, tag width,
offset widths, refill beat count and byte-enable widths are computed
by the tool. Nothing currently stores them; see TD-06.

Validation state, all with python jsonschema 4.26.0, draft 2020-12,
each with accept and reject cases per conditional:

```
  cgen_system.schema.json      0.10.0   5 reject cases
  cgen_ports.schema.json       0.10.0
  cgen_caches.schema.json      0.11.0  17 reject cases
  cgen_links.schema.json       0.10.0  13 cases, TileLink bounds
  cgen_topology.schema.json    0.12.0   5 reject cases
```

The version spread is real, not an error. A file was bumped only
when its content changed.

A pacino configuration exists and validates: L1I, L1D, unified L2,
two TileLink links into L2, TileLink to memory, plus IFU and LSU
agents. Every geometry and timing number in it is invented and is
there to be edited.

THE VIPT CHECK FIRES ON THE PACINO L1I AS WRITTEN. 32KB 4-way is
8192 bytes per way against a 4096 byte page, so the index reaches
one bit above the page offset and the design has synonyms. L1D at
32KB 8-way is exactly 4096 and is clean. Fixes are 16KB 4-way, 32KB
8-way, or PIPT on the I side. This is the first check the schema
work has caught mechanically rather than by eye.

CLI-001 is written and NOT RUN. It builds everything up to and
including derivation and emits no RTL.

GLOSSARY.md IS RETIRED by decision. Design decisions do not belong
in a file named glossary. Its check table becomes machine readable
data, its scope lists become their own document, its standing rules
move to cgen_decisions.md. See TD-02.

New: planning/cgen_decisions.md. 41 decisions, 6 open questions, 7
rejected proposals, each marked [J], [PA] or [D] for provenance.

Next free INFRA number is INFRA-003. Next free CLI number is
CLI-002. CLI-001 is written and unrun.

---

## Session-002: INFRA-002. Schema 2.0.0 audited.

Read-only assessment of the flat schema 2.0.0 against the 64
INFRA-001 findings. 7 CLOSED, 6 PARTIAL, 1 WITHDRAWN, 50 OPEN.

The 50 open are open by construction: 2.0.0 touched only the input
schema, so all 25 model findings and 19 of 20 planning findings
could not close.

THREE DEFECTS INTRODUCED BY 2.0.0 ITSELF, none previously reported:

```
  golden data had two mechanisms with no precedence, emit
    "golden" and verification.golden_vectors
  the clock port name fell out of the pacino profile description
    when reset was promoted to its own fields, and landed nowhere
  $defs.artifact_kind asserted the input and elaborated enums were
    "one definition referenced twice" while leaving the elaborated
    enum unrevised -- a false claim of unity where there had been
    visible duplication
```

The hypothesis was NOT confirmed and that is recorded as a
deliverable: no allOf branch in 2.0.0 could be bypassed by omitting
an optional object. The real mechanical defect was elsewhere --
2.0.0 applied the reject-when-inapplicable principle in one branch
of four, so PIPT with va_bits, mshrs 0 with mshr_targets 8, and
sram_model inferred with macro_name all validated.

All of this is superseded by the five-file rebuild. It is recorded
because the defect classes recur.

---

## Session-001: INFRA-001. First assessment.

Read-only assessment of the repo against schema 1.0.0. 39 files,
14,727 lines. 64 findings: 20 planning, 19 schema-vs-RTL, 25
schema-vs-model.

Two silent-wrong-answer defects rather than gaps:

```
  M-03  options.cpp:321 reads json["mm_capacity_value"] into
        mm_capacity. 8MB reloads as 8. Still open.
  S-12  tag width has three disagreeing sources. RTL hard-wires 14,
        the model derives 5 from its default 8MB mm_capacity, the
        schema derives it from pa_bits. They agree only at 4GB.
```

DW-7 applied from INFRA-002 onward: the two jsoncpp amalgamation
files were 7,401 of the 14,727 manifest lines and produced no
finding. Dropped from every manifest since. Closed.

---

## Module Status

| Module / document                  | Status      | Notes                                                      |
|------------------------------------|-------------|------------------------------------------------------------|
| cgen (cli)                         | NOT STARTED | CLI-001 written, unrun. No source exists.                  |
| cli/src, cli/inc, cli/tb           | NOT STARTED | Directories do not exist.                                  |
| cgen-gui                           | NOT STARTED | No task written.                                           |
| planning/schema/system             | DRAFT       | 0.10.0. Include list only. 5 reject cases pass.            |
| planning/schema/ports              | DRAFT       | 0.10.0. Port type plus role. Role is a [PA] call.          |
| planning/schema/caches             | DRAFT       | 0.11.0. Five cache_type values. 17 reject cases pass.      |
| planning/schema/links              | DRAFT       | 0.10.0. TileLink bounds from spec 1.9.3 Table 4, verified. |
| planning/schema/topology           | DRAFT       | 0.12.0. Nodes, typed edges, addressing block.              |
| planning/schema/elaborated         | DEPRECATED  | Predates the file split. Stale against everything. TD-06.  |
| planning/cgen_decisions.md         | DRAFT       | New this session. 41 decisions with provenance marks.      |
| planning/PROJECT_CORE.md           | DRAFT       | 20 findings open from INFRA-001. TD-03, TD-04, TD-05.      |
| planning/GLOSSARY.md               | DEPRECATED  | Retired by decision. Split pending. TD-02.                 |
| planning/ANTIPATTERNS.md           | NOT STARTED | SPDX and header block only. TD-07.                         |
| planning/CLOSED_TECH_DEBT.md       | NOT STARTED | SPDX and header block only. TD-07.                         |
| planning/tools/tool_decisions.md   | DRAFT       | A bare list of eight tool names. No decisions recorded.    |
| planning/tools/verilator_decisions | DRAFT       | Carries predecessor-project specifics: bp_pkg,             |
|                                    |             | NUM_PRED_SLOTS. TD-08.                                     |
| planning/tools/verilog_style.md    | DRAFT       | Unchanged since INFRA-001.                                 |
| testcases/pacino                   | DRAFT       | Five files, validating. Every number invented. TD-09.      |
| testcases/1mb_l1 (rtl)             | DRAFT       | Ten RTL defects open, S-17 a-j. Will not pass Verilator.   |
| testcases/1mb_l1 (model)           | DRAFT       | M-03, M-09, M-10, M-12, M-15 open. mdl always exits 1.     |
| templates/TASK_TEMPLATE.md         | DRAFT       | Deliverables prefix mismatch, CB vs CG.                    |

---

## Technical Debt

| #  | Item                                    | Resolution path                                              |
|----|-----------------------------------------|--------------------------------------------------------------|
| 01 | DW-8. Is testcases/1mb_l1 a pacino      | JEFF DECISION. Blocks the emitter: it decides whether the    |
|    | template or a generic reference?        | generator targets SystemVerilog-2023 and the example needs   |
|    |                                         | translating first, or writes fresh and the example is only   |
|    |                                         | evidence. Every file in it is .v Verilog-2001.               |
| 02 | GLOSSARY.md retirement.                 | Four documents in one. Check table (17 ids) becomes machine  |
|    |                                         | readable data; scope lists become their own document;        |
|    |                                         | standing rules move to cgen_decisions.md; the rest goes.     |
|    |                                         | Two rows are already stale: IF-3 names critical_word_first   |
|    |                                         | and EMIT-2 names vectors, both deleted.                      |
| 03 | PROJECT_CORE cites CLAUDE.md content    | :136-138 names a "Fixed Constants" section that does not     |
|    | that does not exist.                    | exist. :206-208 names a suite-gating requirement CLAUDE.md   |
|    |                                         | does not state, which makes the waiver paragraph             |
|    |                                         | unenforceable. A prompt satisfying it satisfies nothing.     |
| 04 | PROJECT_CORE :121 and :123 contradict   | :121 makes the IA the sole modifier, :123 makes the user     |
|    | on who modifies a planning document.    | the modifier. CLAUDE.md agrees with :123. Fix :121.          |
| 05 | Three names for two schema files.       | PROJECT_CORE :283-284, GLOSSARY :15-16 and the actual paths  |
|    |                                         | disagree. Cheapest R1 item; removes the most confusion per   |
|    |                                         | edit. Now compounded: there are five schema files.           |
| 06 | The elaborated output schema is stale   | Decide whether it exists at all. It buys provenance and a    |
|    | and predates the file split.            | regeneration diff, neither of which blocks the emitter.      |
|    |                                         | Derived values in memory is the alternative. If it stays,    |
|    |                                         | checks[].id needs an enumerated id set.                      |
| 07 | ANTIPATTERNS.md and CLOSED_TECH_DEBT.md | Populate or mark NOT STARTED and drop from manifests.        |
|    | are empty.                              | Candidate entries exist: a manifest naming a 7400-line       |
|    |                                         | third-party amalgamation, and a prompt asking the IA to      |
|    |                                         | compare a design against a schema while calling it "the      |
|    |                                         | input configuration json".                                   |
| 08 | planning/tools carries predecessor      | verilator_decisions :29-31 imports bp_pkg, :33-35 names      |
|    | specifics that will mislead a prompt.   | NUM_PRED_SLOTS, a branch predictor parameter. Generalise or  |
|    |                                         | mark illustrative.                                           |
| 09 | Every geometry and timing number in     | JEFF EDIT. Sourced from nothing. The L1I VIPT aliasing is    |
|    | the pacino config is invented.          | the first consequence; see Open Items 1.                     |
| 10 | Ten RTL defects in testcases/1mb_l1.    | S-17 a-j from INFRA-001, all verified still present by       |
|    |                                         | INFRA-002. Three block Verilator: continuous assign to a reg |
|    |                                         | output in bitrf.v:35 and lrurf.v:35, and cache.v multiply-   |
|    |                                         | driving mm_write_d at :156 and :400. probes.v will not       |
|    |                                         | elaborate against top.v. Gated on TD-01.                     |
| 11 | Model defects in testcases/1mb_l1.      | Severity order: M-03 silent 8MB to 8 on reload, M-15         |
|    |                                         | undefined behaviour on construction, M-09 and M-10 stale and |
|    |                                         | unchecked iterators, M-12 inverted pass criterion. mdl always|
|    |                                         | exits 1, so any task gating on "model tests pass" is blocked.|
| 12 | Task template defects.                  | Deliverables says prompts/CB-000.md, header says CG-000. No  |
|    |                                         | slot for granted permissions; every task so far has needed   |
|    |                                         | one and used Context Comments instead.                       |
| 13 | Six [PA] decisions in cgen_decisions.md | Calls made by the PA that Jeff has not ruled on. Most likely |
|    | have never been ruled on.               | wrong: D-35 nodes as instances, D-29 role on the port type,  |
|    |                                         | D-30 edges naming ports.                                     |

---

## Open Items

| Priority | Item                                              | Status                        |
|----------|---------------------------------------------------|-------------------------------|
| 1        | Edit the pacino config. Fix the L1I VIPT alias.   | Jeff. Blocks nothing;         |
|          | 32KB 4-way is 8192 B/way against a 4096 B page.   | CLI-001 should report it.     |
| 2        | Install the five schemas at planning/schema and   | Required before CLI-001 runs. |
|          | the pacino files at testcases/pacino.             | Paths are in its manifest.    |
| 3        | Confirm the jnutils subdirectory names.           | CLI-001 grants read on        |
|          |                                                   | idioms, program_options, msg. |
| 4        | Delete the stale pa_bits sentence from CLI-001    | Resolved: addressing now      |
|          | requirement R6.                                   | lives in the topology file.   |
| 5        | Port occupancy. May a target port host more than  | DECIDED: yes. Recorded as     |
|          | one edge?                                         | T-7 in cgen_decisions.        |
| 6        | Version compatibility across files.               | DECIDED: not an issue. The    |
|          |                                                   | tool parses the format or     |
|          |                                                   | errors out.                   |
| 7        | Node kinds beyond agent.                          | CLOSED. attach deleted, core  |
|          |                                                   | as a first class type         |
|          |                                                   | rejected. agent covers it.    |
| 8        | Node and edge shape versus DOT.                   | OPEN, low. Nodes are keyed by |
|          |                                                   | name, edges are an array.     |
|          |                                                   | Two shapes for one graph.     |
| 9        | Include path base directory.                      | OPEN. Relative to the         |
|          |                                                   | including file or to the      |
|          |                                                   | system root. Differs once     |
|          |                                                   | includes nest.                |
| 10       | Include cycle detection.                          | Stated in CLI-001 R2. No      |
|          |                                                   | decision needed.              |
| 11       | Whether schemas are read from disk at runtime or  | CLI-001 asks the IA to state  |
|          | compiled into the binary.                         | which it did.                 |
| 12       | The offset unit question. Word count or byte      | OPEN. S-02 and R3-B1. It      |
|          | count.                                            | changes a VALUE, not a name,  |
|          |                                                   | and feeds the read mux, the   |
|          |                                                   | model and the datasheet.      |
|          |                                                   | Settle before regeneration.   |

---

## Tool Decisions (settled this session)

| Area              | Decision                                          |
|-------------------|---------------------------------------------------|
| Language          | C++23, namespace cgen, one class per file         |
| Build             | Make. Never CMake.                                |
| JSON              | nlohmann                                          |
| Schema validation | pboettch json-schema-validator, header only       |
| Command line      | Boost.ProgramOptions                              |
| Tests             | gtest, under cli/tb                               |
| JSON to C++       | Hand written. No code generation from the schema. |
| Layout            | cli/src, cli/inc, cli/tb                          |
| CLI form          | --cmd={check,emit}, no positional arguments       |
| Output            | --output, default ./output                        |
| Error behaviour   | --eoe exits on first error, default off           |
| Diagnostics       | An object carrying file, path, severity, message. |
|                   | Stages accumulate rather than throwing.           |
| Style             | 80 columns, 2 space indent, ASCII, per CLAUDE.md  |

---

## Tool Checks

The schema holds shape and vocabulary. These are tool work, because
the diagnostic matters more than the check.

```
  T-1  undefined name: edge endpoint, cache ref, link ref, port type
  T-2  duplicate definition across files
  T-3  port type compatibility on both ends of every edge
  T-4  port role matches edge direction
  T-5  graph terminates, no cycles
  T-6  field group completeness: wholly present or wholly absent
  T-7  port occupancy. DECIDED: a target port may host many edges
  T-8  cross field arithmetic: sets, tag width, VIPT index budget
```

dependentRequired is deliberately not used. T-6 is a tool check so
that it can produce a real message.

---

## Architectural Decisions

Full detail: planning/cgen_decisions.md. Key decisions for quick
reference.

### Configuration

- Five file types. The system file ties files together and carries
  no graph. The topology file holds the graph and the addressing
  block.
- include appears in every file type. system is excluded from every
  include type enum.
- File name participates in the namespace: mysystem@myl1.
- Full name enumeration at load, once. Interactive development is
  complicated by this and that is accepted.
- Only cache_type and geometry are required on a cache. Absence of
  a group means it does not apply.
- cache_type: icache, dcache, unified, memory, agent. memory is a
  cache with associativity 1. agent is a producer/consumer with a
  name and ports and nothing else.
- No derived value in the input. No value plus units pair anywhere.
- Ports are typed references on the cache node. Protocol, widths and
  handshake live on the link, because both ends must agree by
  construction.
- A link definition is a type, not an instance. Several edges naming
  one link definition are attachments to one bus. Arbitration
  belongs on the link.
- Links are discriminated on protocol: tilelink by parameters,
  custom by structure. TileLink parameters are spec 1.9.3 Table 4.
- Topology nodes are instances. The node key is the instance name
  and a cache field names the definition.
- Typed edges are the departure from DOT that matters. A DOT edge
  attribute is a literal; here the type is a name resolved against a
  definition.
- Simulation control is not configuration. Clock period, cycle
  limits, test selection, output directories and macro names are
  tool requirements or CLI flags.

### Generator conventions

These live in an xxx_decisions.md file, not in the JSON. Any
encoding works provided one generator emits all consumers from it.

- The tree PLRU bit encoding and the invalid-way tie break
  direction.
- Signal and port naming beyond clock and reset.
- The non-synthesizable reset clear loop.
- Generated, not transcribed: the PLRU update and victim tables and
  the address field decomposition are built once and emitted to
  every consumer.

---

## Method note

The schema was derived by reacting to findings rather than by
walking the RTL. That method produced six versions, several
repairing regressions from their predecessor, and one silent
deletion of a working field.

The IA did perform a module walk against 0.4.0 and its findings are
in the schema. That pass predates the file split, so the
classification exists but has not been re-checked against the
current shape. The judgement recorded here is that a re-check is not
worth a session: what changed is where a field lives, not whether it
exists.

If the emitter finds a structural choice with no field behind it,
that judgement was wrong and this note is the record of it.

