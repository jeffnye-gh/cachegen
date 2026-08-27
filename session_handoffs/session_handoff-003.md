<!-- SPDX-License-Identifier: Apache-2.0                       -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com -->
# CacheGen PA Session Handoff 003
```
 FILE:    session_handoff-003.md
 SOURCE:  the PA session of 2026-08-27
 STATUS:  starting context for the next cachegen session
 UPDATED: 2026.08.27
 CONTACT: Jeff Nye
```

Successor to handoff 002. That handoff described a tool with a
front half and no emitter, and listed five next actions of which
the first was said to block the next schema edit. All five are
done and none of them blocked what it was said to block.

THE PROJECT GOAL HAS CHANGED. The emitted RTL and the pacino
configuration are going to another project. The C++ functional
model is deferred. Usage documentation, the generator conventions
file and portability outside this tree are deferred, because cgen
runs here.

---

## 0. Where the work is

```
  DONE         five file input schema, node/interface/port model
  DONE         pacino worked example, five files, clean
  DONE         cgen front half: load, resolve, check, derive
  DONE         cgen emitter: RTL, testbenches, Make build
  DONE         generation logs, including field consumption
  DONE         planning/arch/cgen_decisions.md, current
  DONE         planning/PROJECT_STATUS.md, current
  DEFERRED     the C++ functional model
  DEFERRED     usage docs, conventions file, portability
  NOT STARTED  cgen-gui
```

CLI-002 through CLI-005 all ran and delivered in this window.

```
  cli suite       86 of 86
  emitted suite   76 of 76, through the generated Makefiles
  emitted tree    83 files, 0 lint errors, 0 lint warnings,
                  Verilator 5.048
```

Next free CLI number is CLI-006. Next free INFRA number is
INFRA-003.

---

## 1. What exists now -- do not re-derive

cgen reads five JSON files and emits a complete SystemVerilog
design, its testbenches, memory images and a Make build.

```
  output/<node>/rtl/     one module per file
  output/<node>/tb/      testbench, tasks, self checking tests
  output/<system>/       system top, shared packages, the build
  output/<system>/images/ memory images, written by the run
  output/logs/           emission, unconsumed, features, geometry
  output/Vars.mk         tool paths, included by every Makefile
```

Synthesizable: l1i, l1d, l2, each its own top. Not synthesizable:
mem, a behavioural model; ifu and lsu, testbench drivers; and
pacino_top, because it instantiates the agents.

Schema versions, and NOTHING CONSUMES THEM at this stage:

```
  system.schema.json     0.10.0
  ports.schema.json      0.11.0
  caches.schema.json     0.13.0  bank_select_position removed
  links.schema.json      0.12.0
  topology.schema.json   0.14.0
```

$schema in all five is DRAFT 7, matching the validator the tool
links. That question is closed; see section 4.

---

## 2. Context to load, in this order

```
  planning/PROJECT_STATUS.md       current through CLI-005
  planning/arch/cgen_decisions.md  current through CLI-005
  planning/schema/*.schema.json    the five above
  planning/tools/Vars.mk           the master tool path file
  testcases/pacino/*.json          the worked example
  prompts/CLI-005.md               the most recent task
```

prompts/CLI-004.md carries the 17 generator conventions and is
the only record of them. Load it when emitted RTL is the subject,
not otherwise: it is large and reading it whole was 42% of
CLI-005's message budget.

Do NOT load planning/tools/verilator_decisions.md. It carries
predecessor project specifics, bp_pkg and NUM_PRED_SLOTS, and was
excluded by name from CLI-004 and CLI-005 for that reason. TD-08.

Do NOT load testcases/1mb_l1. Two tasks report it is not in this
tree.

---

## 3. Rulings made this session -- do not relitigate

**One port, one edge.** Q-04 is closed. A slave port may not host
more than one edge. A shared bus is several ports on one
interface, which is what the interface level exists for. T-7
emits no diagnostic, so the ruling is not yet enforced.

**bank_interleave_granularity determines the bank position.**
bank_select_position is DELETED. line means consecutive lines
alternate banks, so the select is the bits immediately above the
offset. pacino declared line and above_index together, which is
one configuration carrying a contradiction. D-49.

**A position derivable from another field is a derived value.**
The general form of the above, and the reason the field was
deleted rather than chosen between. D-37, generalised.

**Addressing is byte based**, behind one named package parameter
per node, AddrUnitBytes. A reversal is one edit in one file.

**D-27 is upheld.** The emitter needed two arbiters and neither
contradicts it. Bank against bank is contention the EMITTER
created by giving two banks one downstream master. up_i against
up_d is contention for the arrays, which D-27 already classifies
as pipeline scheduling. WHERE THERE IS ONE PORT THERE IS NO
ARBITRATION.

**verilog_style.md's import rule STANDS.** The file-scope
wildcard import puts package members into $unit and node packages
collide at the system build. The emitter's answer, prefixing
every package member with the node name, is what is expected. The
module-header import form is rejected and stays rejected. R-12.

**With CGEN_ROOT unset, --vars is an error, not a search.** The
master Vars.mk already errors the same way, so the failure has
one message and one place. R-11.

**Schema versions do not matter at this stage.** Nothing consumes
them and no external reader exists. caches.schema.json stayed at
0.13.0 through a property deletion deliberately. Bumping would
have cost 18 file edits and bought nothing.

**Tool paths are simulation control.** D-41 already covered clock
periods and output directories; tool paths join them. They are
CLI flags and a copied Makefile fragment, never input fields.

---

## 4. Traps -- do not rediscover

```
  A GENERATED MAKEFILE THAT NAMES A BARE TOOL RUNS WHATEVER IS
      ON PATH. CLI-004's output was linted by Verilator 5.020,
      not the 5.048 in the tree, and ten warnings were
      attributed to the emitted RTL that 5.048 does not
      produce. Fixed by Vars.mk. Do not reintroduce a bare
      tool name in any generated file.

  TYPE'(var + 1) CASTS AFTER THE ADDITION. The literal is a 32
      bit int, so the add happens at 32 bits with the narrow
      operand expanded and the cast truncates afterward. Size
      the literal instead: var + TYPE'(1). Nine emitted sites
      from one helper.

  exists(), first() AND next() ON AN ASSOCIATIVE ARRAY RETURN
      int, not a bit. Compare them explicitly or the
      conditional draws a width warning.

  A TESTBENCH THAT SAMPLES ON THE DESIGN'S CLOCK EDGE RACES IT,
      and the failure ACCUSES THE DESIGN. A trace showed a
      cache working perfectly while the testbench reported no
      response. Everything drives and samples on the negedge.

  A TEST CAN MEASURE THE WRONG THING AND PASS. The first top
      level test gave both agents one address, so ifu's cold
      miss warmed the line in l2 and lsu's miss never reached
      memory. The test measured l2 and called it the memory.
      Second instance of this shape.

  A HEADER UNDER cli/inc MAY NOT SHARE A NAME WITH A SYSTEM
      HEADER. cli/inc/features.h shadowed glibc's
      <features.h> because -I./inc precedes the system path,
      and the build failed inside /usr/include/time.h.

  THE FIXTURE TREE CONTAINS DELIBERATELY BROKEN FILES.
      neg_include_parse/bad_ports.json is not valid JSON, and
      ten files under schemas_bad_parse and schemas_bad_build
      are named *.schema.json and are broken on purpose. A
      glob for **/*.schema.json will find them.

  TileLink has used two OPPOSITE vocabularies. Spec 0.3.3
      calls the memory side "manager". Specs 1.7 and 1.9.3
      use master/slave, where master is the requesting side.
      Carried from handoff 002 and still true.

  An emitter bug is one bug per output file. Carried from
      handoff 001 and still true. It is why the test is the
      whole build rather than a sample.
```

RESOLVED, remove from the trap list: the draft 7 versus 2020-12
mismatch. The five schemas use neither construct on which the two
drafts differ. unevaluatedProperties appears nowhere and all 24
uses of $ref are the sole key of their object, so the tool was
validating them correctly throughout. The declarations now read
draft 7 and a test asserts it.

---

## 5. What the four tasks found

The findings matter more than the code in every case.

**CLI-002, the node/interface/port model.** T-3 HAD NEVER FIRED.
The resolver read from_port_type off a link where the schema
declares master_port_type, so the check was silently dead through
the whole of CLI-001. T-4 compared roles against initiator and
target while the schemas carry master and slave. The measured
test baseline was 11 of 30, not the 29 of 30 handoff 002
recorded.

**CLI-003, diagnostic coverage.** 21 OF 37 DIAGNOSTIC CODES HAD
NO FIXTURE while the suite was green. The reported figure was
two. Eight codes are unreachable, six of them because the schema
rejects the input before the tool's check runs. T-8.field_sum is
dead by arithmetic: it tests an identity assigned on the line
above it.

**CLI-004, RTL emission.** Three decisions treated as gates did
not gate anything. Three RTL defects that lint could not see were
found only by pushing to a passing simulation, the sharpest being
an L2 answering a multi-beat line request with one beat, a
deadlock invisible to every unit test.

**CLI-005, build environment and top level tests.** 74 OF 287
CONFIGURATION FIELDS ARE READ BY NO STAGE. The estimate was
seven. The top level went from 7 checks to 22 and now emits four
memory images per run.

The pattern across all four is one finding: EVERY PROPERTY THIS
PROJECT ASSUMED RATHER THAN MEASURED WAS WRONG BY A LARGE FACTOR,
and in every case the fix was a mechanism rather than a list.

---

## 6. State of the emitted RTL

This is what goes to the other project. The design is correct for
what it implements, and the list of what it does not implement is
longer than the list of what it does.

WHAT THE EMITTED CACHE DOES NOT DO. Every item is a field the
configuration declares and no stage reads:

```
  the cache is BLOCKING. mshrs reaches a comment and sizes
    nothing. mshr_targets, victim_buffer_entries and
    fill_buffer_entries are inert.
  read_latency_cycles is IGNORED on every cache. Only the
    memory model honours it. Q-14.
  inclusion is not enforced. The probe path is tied off.
  no maintenance port is emitted at all.
  way_access and data_array_organization are inert. Every way's
    tag and data array is read on every access.
  TL-C's B, C and E channels are emitted and tied off.
```

Anyone editing one of those fields gets byte-identical output and
nothing tells them. The generated list is at
output/logs/unconsumed.log.

Every geometry and timing number in pacino is invented. The VIPT
alias was the first consequence and is fixed; l1i is now eight
way, matching l1d, and both L1s sit exactly at the budget. The
timing numbers are still arbitrary and the RTL propagates them.

---

## 7. What was verified this session -- do not re-run

```
  cli suite 86 of 86, clean rebuild
  emitted suite 76 of 76, through the generated Makefiles with
    nothing on the make command line
  every emitted file lints clean, Verilator 5.048, -Wall
  every unit testbench and the system top compile, elaborate
    and pass
  two emissions of one configuration are byte identical,
    checked with diff -r
  74 unconsumed fields, generated and asserted in both
    directions against what the tool read
  l2 and mem bank decomposition under the D-49 ruling
```

74 lint waivers remain, in four classes, all enumerated in
prompts/CLI-004.md: IMPORTSTAR 35, UNUSEDPARAM 11, UNUSEDSIGNAL
22, BLKSEQ 6. No new class and no new site since.

---

## 8. Open questions

None of these blocks the RTL going to the other project.

```
  Q-10  Is emitted RTL regenerated in place or hand-owned after
        the first emission? The de facto answer is regenerated:
        the emitter overwrites what it writes and deletes
        nothing. MORE URGENT NOW, because the tree holds RTL,
        testbenches, a build, four logs and memory images. Still
        free to rule, because adaptation has not started.

  Q-12  The check enumeration and the diagnostic code list
        disagree three ways. T-7 is enumerated, is ruled, and
        emits no code. topology.addressing is a code with no T
        number. T-8.field_sum is dead by arithmetic. Make the
        two one artifact.

  Q-13  NOTHING MEASURES CYCLES. Two findings need it and
        neither can be settled without it: a bank select in the
        wrong position is a performance defect no functional
        test can see, and read_latency_cycles cannot be
        verified. The functional model will not see either.

  Q-14  read_latency_cycles is ignored on every cache. Worse
        than the other inert fields because it is a number a
        user would believe, and among the most likely to be
        edited during exploration. Interim is one line in the
        emitted control header.

  Q-15  Should planning/schema/examples be validated by the
        suite? Two files there are schema invalid and nothing
        noticed, because no test covers that directory.

  TD-13 TEN [PA] DECISIONS HAVE NEVER BEEN RULED ON: D-09,
        D-10, D-19, D-21, D-23, D-25, D-26, D-27, D-30, D-35.
        Five are now load-bearing in SHIPPED RTL rather than
        only in the schema. Promote or reopen each.
```

---

## 9. Next actions, in order

```
  1. Fix the two invalid example files. Delete
     bank_select_position and the trailing comma above it at
     planning/schema/examples/caches_example.json:235 and
     planning/schema/examples/zen5_dside_0.6.1.json:197. One
     edit each.
  2. --vars with CGEN_ROOT unset is an error, not a search.
     R-11 is ruled and unimplemented.
  3. Disclose read_latency_cycles in the emitted control
     header, as mshrs already is. One emitted line.
  4. Then the other project. Items 1 through 3 are small and
     none of them blocks it.
```

Not on any path until the other project reports back: the
functional model, usage documentation, the generator conventions
file, portability outside this tree, the schema version
question, and every remaining Technical Debt item.

---

## 10. Working method

Carried from handoff 002 and reinforced by this session.

```
  - State findings plainly. No metaphors, no picturesque
    phrasing, no clever headings. Complete sentences.
  - Verify before asserting, and verify before CONCEDING.
  - BEFORE RAISING A TOPIC, CHECK WHETHER IT IS ALREADY
    CLOSED. The PA reopened the import rule and the
    arbitration scope in this session, both already settled,
    and recommended a change to the first without knowing why
    the rule existed. CLAUDE.md states this rule and it was
    not followed.
  - DO NOT REPEAT SOMETHING THE USER HAS JUST EXPLAINED as
    though it were a new finding.
  - Do not propose scope the user did not ask for. Several
    reviews in this session ended with a new small task, and
    the effect was a treadmill rather than convergence.
  - Do not report a self-imposed modeling choice as a
    constraint the model forces.
  - Boilerplate and mechanical work is IA work. The PA is for
    the model.
  - A REVIEW ENDS WITH WHAT THE USER MUST DECIDE, not with a
    list of everything that could be improved.
```

On task files, from four written this session:

```
  - An R-1 that asks what is ALREADY DONE, reported as
    ALREADY DONE / PARTIAL / ABSENT with evidence, has caught
    something every time.
  - An R-2 baseline before any change catches a stale
    reported figure. It found the 11 of 30.
  - A requirement to REPORT rather than write is how planning
    documents stay read-only while the IA still contributes.
  - Every waiver must name the files and the task ID, and it
    is JEFF'S statement, not the PA's. The PA drafts it and
    Jeff confirms.
  - Ask for a MECHANISM, not a list. Every list this project
    maintained went stale.
```

---

## 11. Document History

```
  2026-08-23  001  IA session. Parameter vocabulary and the two
                   flat JSON schemas. No generator code.
  2026-08-25  002  PA session. Flat schema retired, five-file
                   input schema, node/interface/port model,
                   CLI-001 delivered the tool's front half.
  2026-08-27  003  PA session. CLI-002 through CLI-005. RTL
                   emission delivered, lint clean, building and
                   passing. Q-04 and Q-09 ruled. Diagnostic
                   coverage, field consumption and feature
                   coverage all measured for the first time.
                   PROJECT_STATUS and cgen_decisions rewritten
                   as PA-direct corrections, session-003.
```

