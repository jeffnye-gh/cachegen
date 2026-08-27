<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->
# Project Status -- cachegen

```
 FILE:    PROJECT_STATUS.md
 SOURCE:  various
 STATUS:  WORKING
 UPDATED: 2026.08.27
 CONTACT: Jeff Nye
```

Updated every session. Paste into IA or PA at session start, along
with the latest session_handoff-NNN.md and CLAUDE.md.

Paste PROJECT_CORE.md only when methodology is under discussion.

This file is the authoritative list of document status. The values
in use are DRAFT, CLOSED, NOT STARTED and DEPRECATED.

---

## Where the work is

cgen reads the pacino configuration and emits a complete
SystemVerilog design, its testbenches and a Make build. The
emitted design lints clean, builds, elaborates and passes.

```
  DONE         five file input schema, node/interface/port model
  DONE         pacino worked example, five files, clean
  DONE         cgen front half: load, resolve, check, derive
  DONE         cgen emitter: RTL, testbenches, Make build
  DONE         generation logs, including field consumption
  DONE         planning/arch/cgen_decisions.md, current to CLI-003
  NOT STARTED  the C++ functional model backend
  NOT STARTED  the elaborated output. TD-06, not a blocker
  NOT STARTED  cgen-gui
  DEFERRED     usage documentation, generator conventions file,
               portability outside this tree. Not needed while
               cgen runs here.
```

CLI-001 through CLI-005 have run and delivered.

```
  cli suite       86 of 86
  emitted suite   76 of 76, through the generated Makefiles
  emitted tree    83 files, 0 lint errors, 0 lint warnings
```

Next free CLI number is CLI-006. Next free INFRA number is
INFRA-003.

---

## State of the emitted RTL

This is what another project would consume. The design is
correct for what it implements and the list of what it does not
implement is longer than the list of what it does.

```
  synthesizable    l1i, l1d, l2. Each is its own top.
  not synthesizable
                   mem, a behavioural memory model
                   ifu, lsu, testbench drivers
                   pacino_top, because it instantiates the agents
```

Emitted per node under output/<node>/, plus output/pacino/ for
the system top, the shared packages and the build. Vars.mk sits
at the output root and every Makefile includes it.

WHAT THE EMITTED CACHE DOES NOT DO. A consumer will hit these
and none of them is reported by the tool as a limitation:

```
  the cache is BLOCKING. mshrs reaches a comment and sizes
    nothing. mshr_targets, victim_buffer_entries and
    fill_buffer_entries are read by nothing.
  read_latency_cycles is IGNORED on every cache. It is honoured
    only by the memory model. TD-19.
  inclusion is not enforced. Enforcing it needs the probe path
    that TL-C's B and C channels carry, and those are tied off.
  no maintenance port is emitted. invalidate_line,
    invalidate_all, flush_line and flush_all are inert.
  way_access and data_array_organization are inert. Every way's
    tag and data array is read on every access.
  TL-C's B, C and E channels are emitted and tied off.
```

74 of pacino's 287 configuration fields are read by no stage.
The full list is generated at output/logs/unconsumed.log.

216 features are enumerated from the configuration. 35 have a
behavioural test. 107 are consumed but shape emitted text or a
derived width rather than a behaviour, so the build verifies
them rather than a test. 74 are inert.

---

## Session-007: CLI-005. Build environment and top level tests.

Vars.mk is copied from planning/tools/Vars.mk to the output root
and every emitted Makefile includes it. No emitted recipe carries
a bare tool name. The option is --vars, defaulting to
$CGEN_ROOT/planning/tools/Vars.mk, and --tool VAR=PATH sets a
path. $CGEN_ROOT is special: a path inside the tree is written
back as $(CGEN_ROOT)/... so the emitted tree stays portable.

VERILATOR 5.048 PRODUCED NONE OF THE TEN WARNINGS CLI-004's
output showed. Those came from 5.020, which ran because no
emitted Makefile said which Verilator to use. Both halves were
one finding. All ten were fixed in the emitter regardless: nine
were one helper casting after the addition, TYPE'(x + 1), which
performs the add at 32 bits and truncates afterward. Sizing the
literal instead is correct under any version.

FIELD CONSUMPTION IS NOW MEASURED. The tool records every leaf at
load and every read at the point a stage takes a value. What is
left is the unconsumed report. 74 of 287. The CLI-005 hypothesis,
written from CLI-004's observations, named seven.

Two rules make the number meaningful and both belong in the
decision record:

```
  PRESENCE IS NOT A READ. T-6 asks whether a field exists and
    never looks at its value, so it does not mark. Otherwise the
    whole maintenance group counts as consumed.
  THE RECORD IS MADE IN THE ACCESSOR, not at extraction. NodeCtx
    extracts inclusion and nothing calls inclusion(). Four of
    fifteen accessors have no caller.
```

Four logs under output/logs: emission, unconsumed, features,
geometry.

The top level testbench went from 7 checks to 22. It emits four
memory images per run at named points, in a plain text format
with a fixed width hex address and data per line. The pair either
side of an eviction is the write_back evidence and no unit test
can produce it.

A defect found by running it: the first version gave both agents
one address, so ifu's cold miss warmed the line in l2 and lsu's
miss never reached memory. The test measured l2 and called it the
memory, and it passed. Second instance of a test measuring the
wrong thing and reporting success. TD-20.

---

## Session-006: CLI-004. RTL emission.

78 files, 10,329 lines of SystemVerilog, from the pacino
configuration. Every file linted clean under Verilator 5.048 with
-Wall. Every unit testbench and the system top compiled,
elaborated and ran, 61 checks passing where the bar was only that
they run and report.

THREE DECISIONS TREATED AS GATES DID NOT GATE ANYTHING. The
target dialect was already stated in CLAUDE.md, so the RTL was
written fresh and the 1mb_l1 question never arose. The bank field
was derived. The offset unit went behind one named parameter,
AddrUnitBytes, so reversing it is one edit in one file.

The emitter decided 17 generator conventions because D-40's file
does not exist. They are reported in the task file. TD-21.

THREE RTL DEFECTS LINT COULD NOT SEE, all found by pushing past
the stated bar to a passing simulation:

```
  the l2's TileLink slave answered a multi beat line request
    with one beat. A deadlock, invisible to every unit test,
    visible only when a real L1 talks to a real L2.
  a fill answering a write miss ORed the written word into the
    fetched line instead of replacing it.
  the fill merge then ran on reads, because the write strobe
    register held the previous request's mask.
```

Also found: verilog_style.md's file-scope wildcard import puts
package members into $unit, which is shared across the whole
compilation, so four node packages collide at the system build
while each node lints clean alone. The emitter's answer is to
prefix every package member with the node name. The style rule
stands. TD-22.

---

## Session-005: CLI-003. Diagnostic coverage measured.

One authoritative list of every diagnostic code the tool can
emit, cli/inc/diag_codes.h, 37 codes with the constants and the
run-time table generated from one X macro.

21 OF THE 37 CODES HAD NO FIXTURE. CLI-002 had reported two. The
suite was 31 of 31 green while more than half the diagnostics had
never fired in the life of the project.

```
  fixture added           13
  found unreachable        8
  suite                   31 of 31 -> 48 of 48
```

Six of the eight are unreachable because the schema rejects the
input before the tool's check runs, which is the D-46 pattern
appearing in geometry. T-8.field_sum is dead by arithmetic: it
tests an identity assigned on the line above it, and it is the
check that would have to catch the S-12 class.

The draft 7 question is closed. The five schemas use neither
construct on which draft 7 and 2020-12 differ, so the tool was
validating them correctly throughout. They now declare draft 7
and a test asserts it. Python jsonschema is not a validator of
record.

---

## Session-004: CLI-002. The node/interface/port model.

The input model moved from node.port to node.interface.port. The
link moved from the edge to the interface. Arbitration moved from
the link to the interface.

```
  node        node_type, and the type specialisation
    interface link, arbitration
      port    role, master or slave
```

TWO PRE-EXISTING DEFECTS WERE REPAIRED, NEITHER PART OF THE
INTERFACE WORK. T-4 compared port roles against initiator and
target while the schemas carry master and slave. The resolver
read from_port_type off a link where the schema declares
master_port_type, SO T-3 NEVER FIRED AT ALL.

The measured baseline was 11 of 30, not the 29 of 30 recorded in
handoff 002. That figure and handoff 002's schema version table
both described the intended shape rather than the tree. TD-14.

pacino's l1i went from four ways to eight, closing the VIPT
alias. A VIPT way must not exceed a page.

---

## Session-003: CLI-001. The tool's front half.

12 classes, gtest under cli/tb. cgen --cmd=check loads, resolves,
checks and derives. Nine schema gaps found. G-9 survives: a
definition no node instantiates gets no geometry derivation and
no arithmetic check.

---

## Sessions 001 and 002: INFRA-001 and INFRA-002.

Superseded by the five-file rebuild. Recorded because the defect
classes recur.

INFRA-001 found 64 findings over 39 files. Two were silent wrong
answers rather than gaps:

```
  M-03  options.cpp reads json["mm_capacity_value"] into
        mm_capacity. 8MB reloads as 8.
  S-12  tag width has three disagreeing sources. RTL hard-wires
        14, the model derives 5, the schema derives from
        pa_bits. They agree only at 4GB.
```

INFRA-002's hypothesis was NOT confirmed and that was the
deliverable. One defect it introduced is the pattern that has now
recurred four times: a claim that two things are one, recorded
without a read. Compare T-3 firing on nothing, handoff 002's
version table, and CLI-005's seven-field estimate against 74.

---

## Module Status

| Module / document                  | Status      | Notes                                                       |
|------------------------------------|-------------|-------------------------------------------------------------|
| cgen (cli)                         | DRAFT       | load, resolve, check, derive, emit. 86 of 86.                |
| cgen emitted RTL                   | DRAFT       | 83 files, lint clean, 76 of 76. See State of the emitted RTL.|
| cgen (cli), functional model       | NOT STARTED | Deferred. Not needed for the RTL handoff.                    |
| cli/inc, cli/src, cli/tb           | DRAFT       | 27 classes, 8 test files.                                    |
| cli/tb/fixtures                    | DRAFT       | 26 configurations plus 2 broken schema sets. TD-16.          |
| cgen-gui                           | NOT STARTED | No task written.                                             |
| planning/schema/system             | DRAFT       | 0.10.0. Include list only. Draft 7 declared.                 |
| planning/schema/ports              | DRAFT       | 0.11.0. Port type plus role, master/slave.                   |
| planning/schema/caches             | DRAFT       | 0.13.0. bank_select_position removed, CLI-005.               |
| planning/schema/links              | DRAFT       | 0.12.0. TileLink bounds from spec 1.9.3 Table 4.             |
| planning/schema/topology           | DRAFT       | 0.14.0. Edge names node.interface.port.                      |
| planning/schema/examples           | DRAFT       | TWO FILES INVALID. Carry bank_select_position. TD-23.        |
| planning/schema/elaborated         | DEPRECATED  | Predates the file split. Nothing reads it. TD-06.            |
| planning/arch/cgen_decisions.md    | DRAFT       | Current to CLI-003. CLI-004 and CLI-005 rulings not folded   |
|                                    |             | in. TD-21.                                                   |
| planning/tools/Vars.mk             | DRAFT       | New. The master copy the emitter reads and copies.           |
| planning/PROJECT_CORE.md           | DRAFT       | TD-03, TD-04, TD-05.                                         |
| planning/GLOSSARY.md               | DEPRECATED  | Retired by decision. Split pending. TD-02.                   |
| planning/ANTIPATTERNS.md           | NOT STARTED | Header only. Six candidates now exist. TD-07.                |
| planning/CLOSED_TECH_DEBT.md       | NOT STARTED | Header only. TD-07.                                          |
| planning/tools/tool_decisions.md   | DRAFT       | Validation section pending from CLI-003 R-7.                 |
| planning/tools/verilator_decisions | DRAFT       | Predecessor specifics: bp_pkg, NUM_PRED_SLOTS. TD-08.        |
| planning/tools/verilog_style.md    | DRAFT       | The import rule is a correctness trap, not a preference.     |
|                                    |             | TD-22.                                                       |
| testcases/pacino                   | DRAFT       | Five files, clean, emitting. Every number invented. TD-09.   |
| testcases/1mb_l1                   | UNKNOWN     | Not found in this tree by CLI-001 or CLI-003. No longer      |
|                                    |             | blocks anything: the emitter wrote fresh. TD-01.             |
| templates/TASK_TEMPLATE.md         | DRAFT       | TD-12. Shows modified in git status.                         |

---

## Technical Debt

Numbers are stable. A closed item keeps its number and is marked
CLOSED rather than removed, because other documents cite them.

| #  | Item                                    | Resolution path                                              |
|----|-----------------------------------------|--------------------------------------------------------------|
| 01 | testcases/1mb_l1 is not in this tree.   | DOWNGRADED. It was recorded as blocking the emitter and it    |
|    |                                         | did not: CLI-004 wrote fresh SystemVerilog-2023 per           |
|    |                                         | CLAUDE.md and the question never arose. What remains is that  |
|    |                                         | the module walk in the Method note was never done, so the     |
|    |                                         | field set is not known to be complete.                        |
| 02 | GLOSSARY.md retirement.                 | Standing rules have already moved to cgen_decisions.md. The   |
|    |                                         | check table and scope lists remain.                           |
| 03 | PROJECT_CORE cites CLAUDE.md content    | Names a "Fixed Constants" section that does not exist and a   |
|    | that does not exist.                    | suite-gating requirement CLAUDE.md does not state.            |
| 04 | PROJECT_CORE :121 and :123 contradict   | :121 makes the IA the sole modifier, :123 makes the user      |
|    | on who modifies a planning document.    | the modifier. CLAUDE.md agrees with :123. Fix :121.           |
| 05 | PROJECT_CORE names schema files that    | It still names cache_config.schema.json and output_json.md,   |
|    | do not exist.                           | neither of which is in the tree.                              |
| 06 | The elaborated output does not exist    | NOT A BLOCKER and no longer on any path. The emitter holds    |
|    | in the current shape.                   | derived values in memory and emits them into the packages.    |
|    |                                         | Decide whether the file is wanted at all.                     |
| 07 | ANTIPATTERNS.md and CLOSED_TECH_DEBT.md | Six candidates: a manifest naming a third-party               |
|    | are empty.                              | amalgamation; a prompt comparing a design to a schema while   |
|    |                                         | calling it the input json; state reported without a read;     |
|    |                                         | a green suite standing in for unmeasured coverage; a          |
|    |                                         | testbench that races the design; a test that measures the     |
|    |                                         | wrong thing and passes.                                       |
| 08 | planning/tools carries predecessor      | verilator_decisions imports bp_pkg and names NUM_PRED_SLOTS.  |
|    | specifics that will mislead a prompt.   | It was excluded from CLI-004 and CLI-005 by name for this     |
|    |                                         | reason. Generalise or mark illustrative.                      |
| 09 | Every geometry and timing number in     | JEFF EDIT. The VIPT alias was the first consequence and is    |
|    | the pacino config is invented.          | fixed. The timing numbers are still arbitrary and the         |
|    |                                         | emitted RTL now propagates them.                              |
| 10 | Ten RTL defects, S-17 a-j, in           | Gated on whether that directory exists. Not on any path.      |
|    | testcases/1mb_l1.                       |                                                               |
| 11 | Model defects, M-03 and others, in      | Same. Not on any path.                                        |
|    | testcases/1mb_l1.                       |                                                               |
| 12 | Task template defects.                  | Deliverables says prompts/CB-000.md, header says CG-000. No   |
|    |                                         | slot for granted permissions; five tasks have needed one and  |
|    |                                         | used Context Comments. No interactive mode. Resume sha and    |
|    |                                         | Model effort cannot be filled by the IA and are carried       |
|    |                                         | unfilled every task.                                          |
| 13 | Unruled [PA] decisions in               | TEN: D-09, D-10, D-19, D-21, D-23, D-25, D-26, D-27, D-30,    |
|    | cgen_decisions.md.                      | D-35. Five are now load-bearing in SHIPPED RTL rather than    |
|    |                                         | only in the schema. Promote or reopen each.                   |
| 14 | State reported without a read.          | Four instances: handoff 002's version table, its 29 of 30     |
|    |                                         | baseline, INFRA-002's claim that two enums were one, and      |
|    |                                         | CLI-005's seven-field estimate against a measured 74. Rule:   |
|    |                                         | a version, a count or a shape reported without a read is a    |
|    |                                         | plan and must be marked as one.                               |
| 15 | The check enumeration and the           | T-7 is in the enumeration and emits no code, so port          |
|    | diagnostic code list disagree.          | occupancy is unchecked now that one port one edge is ruled.   |
|    |                                         | topology.addressing is a code with no T number.               |
|    |                                         | T-8.field_sum is dead by arithmetic. Make the enumeration     |
|    |                                         | and the code list one artifact.                               |
| 16 | The fixture tree contains deliberately  | neg_include_parse/bad_ports.json is not valid JSON. Ten       |
|    | broken files.                           | files under schemas_bad_parse and schemas_bad_build are       |
|    |                                         | named *.schema.json and are broken on purpose.                |
| 17 | CGEN_SCHEMA_DIR is undocumented.        | An environment variable selects which schemas validate the    |
|    |                                         | configuration. It appears in no decision. D-41 covers CLI     |
|    |                                         | flags and rules on no environment variable.                   |
| 18 | C++20 or C++23.                         | D-48 and the Makefile say C++20. CLAUDE.md says C++23.        |
|    |                                         | CLAUDE.md is the file the IA reads every session.             |
| 19 | read_latency_cycles is IGNORED on       | The memory model honours it and no cache does. A              |
|    | every cache.                            | configuration asking for a 12 cycle L2 gets whatever the      |
|    |                                         | pipeline happens to be, and nothing says so. Worse than the   |
|    |                                         | other inert fields because it is a number a user would        |
|    |                                         | believe. Interim: one line in the emitted control header,     |
|    |                                         | as mshrs already has. Real fix: a configurable pipeline       |
|    |                                         | depth with tag_compare_stage selecting the compare stage.     |
| 20 | Nothing measures cycles.                | Two CLI-005 findings need it and neither can be settled       |
|    |                                         | without it: a bank select in the wrong position is a          |
|    |                                         | performance defect a functional test cannot see, and          |
|    |                                         | read_latency_cycles cannot be verified. The functional        |
|    |                                         | model will not see either.                                    |
| 21 | The generator conventions file does     | D-40 says conventions live in an xxx_decisions.md file.       |
|    | not exist.                              | CLI-004 decided 17 and CLI-005 added more, all reported in    |
|    |                                         | task files and nowhere else. Not needed while cgen runs       |
|    |                                         | here; needed before anyone else reads the emitted RTL.        |
| 22 | verilog_style.md's import rule is a     | A file-scope wildcard import lands in $unit, shared across    |
|    | correctness trap, not a preference.     | the compilation, so node packages collide at the system       |
|    |                                         | build while each node lints clean alone. The rule STANDS and  |
|    |                                         | the emitter prefixes package members to work around it. The   |
|    |                                         | file should say WHY, because a hand written module has no     |
|    |                                         | such workaround and no warning.                               |
| 23 | Two files under planning/schema/        | caches_example.json:235 and zen5_dside_0.6.1.json:197 carry   |
|    | examples are schema invalid.            | bank_select_position, deleted by CLI-005. Delete the line     |
|    |                                         | and the trailing comma above it. Nothing validates that       |
|    |                                         | directory, which is why it went unnoticed; adding it to the   |
|    |                                         | fixture sweep prevents the next occurrence.                   |
| 24 | --vars falls back to an upward          | With CGEN_ROOT unset the default cannot resolve, so the tool  |
|    | directory search.                       | walks the working directory upward. RULED: that is an error   |
|    |                                         | reported through emit.vars, not a search. The gtest suite     |
|    |                                         | sets CGEN_ROOT in its fixture.                                |
| 25 | A header under cli/inc may not share a  | cli/inc/features.h shadowed glibc's <features.h> because      |
|    | name with a system header.              | -I./inc precedes the system path, and the build failed        |
|    |                                         | inside /usr/include/time.h. Renamed to feature_table.h.       |
|    |                                         | Belongs in CLAUDE.md as a rule.                               |

---

## Open Items

| Priority | Item                                              | Status                        |
|----------|---------------------------------------------------|-------------------------------|
| 1        | TD-23. Fix the two invalid example files, and     | JEFF or one small task.       |
|          | decide whether the suite validates that dir.      |                               |
| 2        | TD-24. --vars: error, not a search.               | RULED. Unimplemented.         |
| 3        | TD-19. Disclose read_latency_cycles in the        | Interim is one emitted line.  |
|          | emitted control header.                           |                               |
| 4        | Q-10. Regenerate in place or hand-own after the   | JEFF. There is now emitted    |
|          | first emission.                                   | RTL, four logs and an images  |
|          |                                                   | directory in the tree.        |
| 5        | TD-13. The ten unruled [PA] decisions.            | JEFF. Five are load-bearing   |
|          |                                                   | in shipped RTL.               |
| 6        | TD-21. Fold the CLI-004 and CLI-005 rulings into  | PA-direct. Q-09 closed, the   |
|          | cgen_decisions.md.                                | D-37 generalisation, the two  |
|          |                                                   | field-consumption rules.      |
| 7        | Port occupancy. RULED one port, one edge.         | T-7 emits no diagnostic.      |
|          |                                                   | Q-04 still reads as open.     |
| 8        | TD-25. The system header name rule into           | One line.                     |
|          | CLAUDE.md. Also TD-18, C++20 versus C++23.        |                               |
| 9        | G-9. An uninstantiated definition gets no         | OPEN, low. Ruled an error in  |
|          | geometry derivation and no arithmetic check.      | session. Unimplemented.       |
| 10       | The six schema-shadowed guard checks and          | OPEN, low. field_sum is an    |
|          | T-8.field_sum.                                    | identity and should go.       |
| 11       | Q-07, version compatibility across a file set.    | OPEN, low.                    |
| 12       | Q-05. A memory smaller than the address space     | OPEN, low.                    |
|          | decodes rather than compares.                     |                               |
| 13       | Q-09, bank placement.                             | CLOSED. CLI-005. Granularity  |
|          |                                                   | determines position;          |
|          |                                                   | bank_select_position deleted. |
| 14       | The offset unit, word or byte.                    | CLOSED. Byte, behind          |
|          |                                                   | AddrUnitBytes. CLI-004.       |
| 15       | Whether schemas are read from disk or compiled    | CLOSED. On disk, selectable   |
|          | into the binary.                                  | at run time. TD-17.           |
| 16       | Tool paths in the emitted build.                  | CLOSED. Vars.mk, --vars,      |
|          |                                                   | --tool. CLI-005.              |
| 17       | The draft 7 versus 2020-12 mismatch.              | CLOSED. CLI-003.              |
| 18       | Include path base directory.                      | CLOSED by implementation.     |
|          |                                                   | Relative to the including     |
|          |                                                   | file. D-10, still [PA].       |

---

## Tool Decisions

| Area              | Decision                                          |
|-------------------|---------------------------------------------------|
| Language          | C++20, namespace cgen, one class per file. TD-18. |
| Build             | Make. Never CMake.                                |
| JSON              | nlohmann                                          |
| Schema validation | pboettch json-schema-validator, draft 7. The only |
|                   | validator of record. Python jsonschema is not.    |
| Command line      | Boost.ProgramOptions                              |
| Tests             | gtest, under cli/tb                               |
| JSON to C++       | Hand written. No code generation from the schema. |
| Layout            | cli/src, cli/inc, cli/tb                          |
| CLI form          | --cmd={check,emit}, no positional arguments       |
| Output            | --output, default ./output                        |
| Error behaviour   | --eoe exits on first error, default off           |
| Diagnostics       | An object carrying file, path, severity, message. |
|                   | Stages accumulate rather than throwing. Codes are |
|                   | declared once in cli/inc/diag_codes.h.            |
| Schema location   | On disk, selectable by CGEN_SCHEMA_DIR. TD-17.    |
| Tool paths        | Vars.mk, copied to the output root. --vars names  |
|                   | the master, --tool VAR=PATH sets one. No emitted  |
|                   | Makefile carries a bare tool name.                |
| Style             | 80 columns, 2 space indent, ASCII, per CLAUDE.md  |

---

## Tool Checks

The schema holds shape and vocabulary. These are tool work
because the diagnostic matters more than the check. The
authoritative list of emitted codes is cli/inc/diag_codes.h; this
table and that list do not agree. TD-15.

```
  T-1  undefined name: node ref, interface, port, interface link
       ref, port type, edge endpoint
  T-2  duplicate definition across files
  T-3  port type compatibility against the link's declared
       master_port_type and slave_port_type
  T-4  port role matches edge direction: from is master, to is
       slave
  T-5  graph terminates, no cycles
  T-6  field group completeness. See D-46 for what it still owns
  T-7  port occupancy. RULED one port, one edge. NO CODE, NO
       CHECK. TD-15
  T-8  cross field arithmetic: capacity, line, associativity,
       sets, tag width, bank divide, VIPT index budget
  T-9  link agreement: both ends of an edge name the same link
```

Unnumbered but emitted: topology.addressing, the load.* family,
the schema.* family, the emit.* family.

---

## Architectural Decisions

Full detail: planning/arch/cgen_decisions.md, which is current to
CLI-003. The CLI-004 and CLI-005 rulings below are NOT yet folded
into it. TD-21.

### Configuration

- Five file types. system ties files together and carries no
  graph. topology holds the graph and the addressing block.
- include appears in every file type. system is excluded from
  every include type enum.
- Full name enumeration at load, once. The tool is a linker.
- The hierarchy is node, then interface, then port. An interface
  carries exactly one link and an optional arbitration policy.
- An edge names node.interface.port at each end. An edge names
  no link; both ends carry one and must agree.
- node_type: icache, dcache, unified, memory, agent,
  interconnect. Only node_type and interfaces are required.
- Arbitration is on the interface. Not the link, which is point
  to point. Not the node, where contention is array port
  scheduling.
- Vocabulary is master and slave.
- No derived value in the input. GENERALISED BY CLI-005: a
  position derivable from another field is a derived value, which
  is why bank_select_position was deleted rather than chosen
  between.
- Simulation control is not configuration. Tool paths included.
- $schema carries the JSON Schema draft, which is 7.
  schema_version carries the project's version and is in $id.

### Emission, from CLI-004 and CLI-005

- The bank select position follows from
  bank_interleave_granularity. line means consecutive lines
  alternate banks, so the select is the bits immediately above
  the offset. word remains unresolved and reports why.
- Addressing is byte based, behind one named package parameter
  per node, AddrUnitBytes.
- Everything is a package localparam. No module has a parameter,
  because a derived value that can be overridden at instantiation
  is a second source that can disagree with the first.
- Package member names carry the node name, to work around the
  $unit collision of TD-22.
- Emission is deterministic. Vars.mk is the one emitted file
  whose contents may vary with the command line.
- 17 further conventions are reported in prompts/CLI-004.md and
  are not yet in a planning file. TD-21.

### Field consumption

- Presence is not a read. A check that asks whether a field
  exists does not mark it consumed.
- The record is made in the accessor, not where the value is
  extracted.

---

## Method note

The schema was derived by reacting to findings rather than by
walking the RTL. The derivation that was never performed: read a
reference design module by module and classify every parameter,
port and structural choice as a config field, a derived value or
a project convention.

CLI-004 did that classification FROM THE OTHER END, starting from
the schema and finding out what an emitter needs. The conventions
it reported are the residue and are exactly the list that
derivation was meant to produce. Whether the field set is
COMPLETE is still unknown, and only a second configuration unlike
pacino would show.

The recurring result across five tasks is narrower and more
useful: every property this project assumed rather than measured
was wrong by a large factor. Two uncovered diagnostic codes
against a measured 21. Seven inert fields against a measured 74.
A 29 of 30 baseline against a measured 11 of 30. In each case the
fix was a mechanism, not a list.

