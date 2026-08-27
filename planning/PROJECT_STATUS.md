<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->
# Project Status -- cachegen

```
 FILE:    PROJECT_STATUS.md
 SOURCE:  various
 STATUS:  WORKING
 UPDATED: 2026.08.26
 CONTACT: Jeff Nye
```

Updated every session. Paste into IA or PA at session start, along
with the latest session_handoff-NNN.md and CLAUDE.md.

Paste PROJECT_CORE.md only when methodology is under discussion.

This file is the authoritative list of document status. The values
in use are DRAFT, CLOSED, NOT STARTED and DEPRECATED.

---

## Where the work is

The front half of cgen is built and runs. The back half does not
exist.

```
  DONE         five file input schema, node/interface/port model
  DONE         pacino worked example, five files, clean
  DONE         cgen front half: load, resolve, check, derive
  DONE         planning/arch/cgen_decisions.md, current
  NOT STARTED  RTL emission
  NOT STARTED  the C++ functional model backend
  NOT STARTED  the elaborated output, see TD-06
  NOT STARTED  cgen-gui
```

CLI-001, CLI-002 and CLI-003 have all run and delivered. The gtest
suite is 48 of 48. cgen --cmd=check on testcases/pacino runs end to
end and produces no diagnostics.

Next free CLI number is CLI-004. Next free INFRA number is
INFRA-003.

---

## What blocks RTL emission

Three items. All are decisions, not work. Everything else in this
file can be done in parallel with the emitter or not at all.

```
  1  TD-01, the 1mb_l1 question. Does the emitter target
     SystemVerilog-2023 and write fresh, or is the existing
     example a template to be translated first? CLI-003 could
     not confirm the directory is in the tree at all. Until
     this is answered the emitter has no target dialect and no
     reference. THIS IS THE ONE THAT MATTERS.

  2  Q-09, bank placement. The schema does not determine the
     bank field lsb and msb, so they are not derived and cannot
     be emitted. pacino's l2 has two banks, so the first
     multi-bank design cannot have its address decomposition
     written.

  3  Open Item 12, the offset unit. Word count or byte count.
     It changes a value, not a name, and it feeds the read mux,
     the model and the datasheet. Cheapest of the three to
     settle and the most expensive to change afterwards.
```

Two more are near-irreversible once emission starts and should be
answered early rather than first:

```
  Q-10  is emitted RTL regenerated in place or hand-owned after
        the first emission?
  D-40  the generator conventions file. PLRU bit encoding, tie
        break direction, signal naming beyond clk and rstn, the
        reset clear loop. Decided as the emitter needs them is
        acceptable; recorded nowhere is not.
```

Not blocking, contrary to earlier readings: the elaborated output
schema, version compatibility across files, the guard diagnostics,
and every record item in the Technical Debt table.

---

## Session-005: CLI-003. Diagnostic coverage measured.

The tool now carries one authoritative list of every diagnostic
code it can emit, cli/inc/diag_codes.h, 37 codes generated with the
run-time table from one X macro. 42 emission sites draw their
string from it. A test relates the list to what the fixtures
produce, in both directions.

21 OF THE 37 CODES HAD NO FIXTURE. CLI-002 had reported two. The
suite was 31 of 31 green while more than half the tool's
diagnostics had never fired in the life of the project.

```
  fixture added           13
  found unreachable        8
  suite                   31 of 31 -> 48 of 48
```

The eight unreachable are the finding. Six are unreachable because
the schema rejects the input before the tool's check runs, which is
the D-46 pattern appearing in geometry rather than in group
completeness. One, schema.open, needs a file mode a fixture cannot
carry.

T-8.field_sum IS DEAD BY ARITHMETIC. tag_bits is assigned
pa_bits - offset - index on the line above a test asking whether
the three sum to pa_bits. It is an identity. It is also the check
that would have to catch the S-12 class, three disagreeing sources
for tag width, and it cannot, because by the time it runs there is
only one source.

The draft 7 question is closed. The five schemas use neither
construct on which draft 7 and 2020-12 differ. unevaluatedProperties
appears nowhere and all 24 uses of $ref are the sole key of their
object, so the tool was validating them correctly throughout. The
declarations now read draft 7 and SchemaFiles.DeclareDraft07
enumerates planning/schema and asserts it. Python jsonschema is no
longer a validator of record.

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

An edge names node.interface.port at each end and no longer names a
link. Both ends must agree on the link, which is T-9.

node_type replaced cache_type and gained interconnect. Values are
icache, dcache, unified, memory, agent, interconnect. memory is a
cache with associativity 1. agent and interconnect carry interfaces
and nothing else.

TWO PRE-EXISTING DEFECTS WERE REPAIRED, NEITHER PART OF THE
INTERFACE WORK:

```
  T-4 compared port roles against initiator and target while the
      schemas and the example carry master and slave
  the resolver read from_port_type and to_port_type off a link
      while the schema declares master_port_type and
      slave_port_type, so T-3 NEVER FIRED AT ALL and was
      silently dead
```

The measured baseline was 11 of 30, not the 29 of 30 recorded in
handoff 002. That figure and handoff 002's schema version table
both described the intended shape rather than the tree. See TD-14.

Group completeness was settled toward the tool. The six
unconditional required lists inside the group objects were removed
so T-6 owns completeness and produces the readable message. The
per-node_type conditionals stay, so the boundary is uneven by
decision and is recorded as D-46.

pacino's l1i went from four ways to eight, which closes the VIPT
alias. A way must not exceed a page, so at a 4096 byte page a way
caps at 4096 bytes and 32KB needs eight of them. Both L1s now sit
exactly at the budget. The negative side of that check moved to the
neg_vipt_alias fixture so coverage was not lost with the fix.

---

## Session-003: CLI-001. The tool's front half.

12 classes under cli/inc and cli/src, gtest under cli/tb. cgen
--cmd=check loads, resolves, checks and derives. No emission.

Nine schema gaps found, G-1 through G-9. G-1, G-2, G-5, G-6 and G-7
closed in the same line of work. G-4 and G-8 were superseded by the
interface model. G-3 survives as Q-09 and is a blocker above. G-9
survives: a definition no node instantiates gets no geometry
derivation and no arithmetic check.

---

## Session-002: INFRA-002. Schema 2.0.0 audited.

Superseded by the rebuild. Recorded because the defect classes
recur. Read-only assessment against the 64 INFRA-001 findings: 7
CLOSED, 6 PARTIAL, 1 WITHDRAWN, 50 OPEN.

The hypothesis was NOT confirmed and that was the deliverable. The
real defect was elsewhere: 2.0.0 applied the
reject-when-inapplicable principle in one branch of four.

One defect it introduced is the pattern that has now recurred three
times: $defs.artifact_kind asserted two enums were one definition
referenced twice while leaving the second unrevised. A false claim
of unity. Compare T-3 firing on nothing, and handoff 002's version
table describing the target rather than the disk.

---

## Session-001: INFRA-001. First assessment.

39 files, 14,727 lines. 64 findings: 20 planning, 19
schema-vs-RTL, 25 schema-vs-model.

Two silent-wrong-answer defects rather than gaps:

```
  M-03  options.cpp:321 reads json["mm_capacity_value"] into
        mm_capacity. 8MB reloads as 8. Open, gated on TD-01.
  S-12  tag width has three disagreeing sources. RTL hard-wires
        14, the model derives 5 from its default 8MB
        mm_capacity, the schema derives it from pa_bits. They
        agree only at 4GB. See T-8.field_sum, session-005.
```

---

## Module Status

| Module / document                  | Status      | Notes                                                       |
|------------------------------------|-------------|-------------------------------------------------------------|
| cgen (cli), front half             | DRAFT       | load, resolve, check, derive. 48 of 48. No emission.         |
| cgen (cli), emitter                | NOT STARTED | Gated on TD-01. No task written.                             |
| cgen (cli), functional model       | NOT STARTED | No task written.                                             |
| cli/inc, cli/src, cli/tb           | DRAFT       | 15 classes, 3 test files added by CLI-003.                   |
| cli/tb/fixtures                    | DRAFT       | 26 configurations plus 2 broken schema sets. TD-16.          |
| cgen-gui                           | NOT STARTED | No task written.                                             |
| planning/schema/system             | DRAFT       | 0.10.0. Include list only. Draft 7 declared.                 |
| planning/schema/ports              | DRAFT       | 0.11.0. Port type plus role, master/slave.                   |
| planning/schema/caches             | DRAFT       | 0.13.0. node_type, six values. interfaces required.          |
| planning/schema/links              | DRAFT       | 0.12.0. Arbitration removed. TileLink bounds from 1.9.3.     |
| planning/schema/topology           | DRAFT       | 0.14.0. Edge names node.interface.port, six required fields. |
| planning/schema/elaborated         | DEPRECATED  | Predates the file split. Stale against everything. TD-06.    |
| planning/arch/cgen_decisions.md    | DRAFT       | 48 decisions, 8 open questions, 8 rejected. Current.         |
| planning/PROJECT_CORE.md           | DRAFT       | TD-03, TD-04, TD-05.                                         |
| planning/GLOSSARY.md               | DEPRECATED  | Retired by decision. Split pending. TD-02.                   |
| planning/ANTIPATTERNS.md           | NOT STARTED | Header only. Four candidates now exist. TD-07.               |
| planning/CLOSED_TECH_DEBT.md       | NOT STARTED | Header only. TD-07.                                          |
| planning/tools/tool_decisions.md   | DRAFT       | Validation section pending from CLI-003 R-7.                 |
| planning/tools/verilator_decisions | DRAFT       | Predecessor specifics: bp_pkg, NUM_PRED_SLOTS. TD-08.        |
| planning/tools/verilog_style.md    | DRAFT       | Unchanged since INFRA-001. Feeds the emitter.                |
| testcases/pacino                   | DRAFT       | Five files, clean. Every number still invented. TD-09.       |
| testcases/1mb_l1                   | UNKNOWN     | CLI-001 and CLI-003 could not find it in this tree. TD-01.   |
| templates/TASK_TEMPLATE.md         | DRAFT       | TD-12. Shows modified in git status.                         |

---

## Technical Debt

Numbers are stable. A closed item keeps its number and is marked
CLOSED rather than removed, because other documents cite them.

| #  | Item                                    | Resolution path                                              |
|----|-----------------------------------------|--------------------------------------------------------------|
| 01 | Is testcases/1mb_l1 a pacino template   | JEFF DECISION. BLOCKS THE EMITTER. Decides whether the       |
|    | or a generic reference? Formerly DW-8.  | generator targets SystemVerilog-2023 and writes fresh, or    |
|    |                                         | the example is translated first. Two tasks report the        |
|    |                                         | directory is not in this tree, so the first question is      |
|    |                                         | whether it exists. Every file in it was .v Verilog-2001.     |
| 02 | GLOSSARY.md retirement.                 | Four documents in one. Check table becomes machine readable  |
|    |                                         | data; scope lists become their own document; standing rules  |
|    |                                         | have already moved to cgen_decisions.md; the rest goes.      |
| 03 | PROJECT_CORE cites CLAUDE.md content    | Names a "Fixed Constants" section that does not exist, and   |
|    | that does not exist.                    | a suite-gating requirement CLAUDE.md does not state, which   |
|    |                                         | makes the waiver paragraph unenforceable.                    |
| 04 | PROJECT_CORE :121 and :123 contradict   | :121 makes the IA the sole modifier, :123 makes the user     |
|    | on who modifies a planning document.    | the modifier. CLAUDE.md agrees with :123. Fix :121.          |
| 05 | Schema file naming.                     | PARTIAL. D-47 settles the rule: version in $id and           |
|    |                                         | schema_version, never in a filename. PROJECT_CORE still      |
|    |                                         | names cache_config.schema.json and output_json.md, neither   |
|    |                                         | of which exists. This file's old cgen_ prefixes are gone.    |
| 06 | The elaborated output does not exist in | Decide whether there is a FILE at all. It buys provenance    |
|    | the current shape.                      | and a regeneration diff, neither of which blocks emission.   |
|    |                                         | Derived values in memory is the alternative and is what the  |
|    |                                         | tool does today. NOT A BLOCKER.                              |
| 07 | ANTIPATTERNS.md and CLOSED_TECH_DEBT.md | Four candidates now: a manifest naming a 7400-line           |
|    | are empty.                              | third-party amalgamation; a prompt comparing a design to a   |
|    |                                         | schema while calling it the input json; a version table      |
|    |                                         | describing the target rather than the disk; a green suite    |
|    |                                         | standing in for coverage that was never measured.            |
| 08 | planning/tools carries predecessor      | verilator_decisions imports bp_pkg and names NUM_PRED_SLOTS, |
|    | specifics that will mislead a prompt.   | a branch predictor parameter. Generalise or mark             |
|    |                                         | illustrative. Feeds the emitter, so do it before RTL work.   |
| 09 | Every geometry and timing number in     | JEFF EDIT. Sourced from nothing. The VIPT alias was the      |
|    | the pacino config is invented.          | first consequence and is fixed. The timing numbers are       |
|    |                                         | still arbitrary and the emitter will propagate them.         |
| 10 | Ten RTL defects, S-17 a-j.              | Gated on TD-01 and on whether the directory exists.          |
| 11 | Model defects, M-03, M-09, M-10, M-12,  | Gated on TD-01. mdl always exits 1, so any task gating on    |
|    | M-15.                                   | "model tests pass" is blocked.                               |
| 12 | Task template defects.                  | Deliverables says prompts/CB-000.md, header says CG-000. No  |
|    |                                         | slot for granted permissions; every task so far has needed   |
|    |                                         | one and used Context Comments instead. No interactive mode,  |
|    |                                         | though CLI-002 was given interactively with an empty prompt  |
|    |                                         | section and no requirements to check deliverables against.   |
| 13 | Unruled [PA] decisions in               | TEN, not six. D-09, D-10, D-19, D-21, D-23, D-25, D-26,      |
|    | cgen_decisions.md.                      | D-27, D-30, D-35. Five of them are load-bearing for the      |
|    |                                         | model CLI-002 implemented across 48 files. D-27 is recorded  |
|    |                                         | as settled in CLI-002 and still marked [PA]. Promote or      |
|    |                                         | reopen each. NOT A BLOCKER.                                  |
| 14 | Reported state has three times差         | A version table, a pass count and a schema shape have each   |
|    | described the target, not the disk.     | been recorded from intent rather than from a read. Costs so  |
|    |                                         | far: one CLI-001 test failure, one false 29 of 30 baseline,  |
|    |                                         | one session opened on a wrong shape. Rule for               |
|    |                                         | ANTIPATTERNS: a version, a count or a shape reported         |
|    |                                         | without a read is a plan and must be marked as one.          |
| 15 | The check enumeration and the           | T-7 appears in the enumeration and emits no code at all,     |
|    | diagnostic code list disagree.          | so port occupancy is unchecked now that one port one edge    |
|    |                                         | is ruled. topology.addressing is a code with no T number.    |
|    |                                         | T-8.field_sum is dead by arithmetic and protects nothing.    |
|    |                                         | Six more T-8 and load codes are unreachable because the      |
|    |                                         | schema rejects first, which is D-46 again. Make the          |
|    |                                         | enumeration and the code list one artifact.                  |
| 16 | The fixture tree contains deliberately  | neg_include_parse/bad_ports.json is not valid JSON. Ten      |
|    | broken files.                           | files under schemas_bad_parse and schemas_bad_build are      |
|    |                                         | named *.schema.json and are broken on purpose. Any future    |
|    |                                         | glob for **/*.schema.json or tree walk parsing every .json   |
|    |                                         | will trip. ANTIPATTERNS candidate.                           |
| 17 | CGEN_SCHEMA_DIR is undocumented.        | An environment variable selects which schemas validate the   |
|    |                                         | configuration. Three CLI-003 fixtures depend on it. It       |
|    |                                         | appears in no decision. D-41 covers CLI flags and tool       |
|    |                                         | requirements and rules on no environment variable.           |
| 18 | C++20 or C++23.                         | D-48 and the Makefile say C++20. CLAUDE.md and this file's   |
|    |                                         | Tool Decisions said C++23. CLAUDE.md is the file the IA      |
|    |                                         | reads every session. Corrected below to C++20; correct       |
|    |                                         | CLAUDE.md or rule the other way.                             |

---

## Open Items

| Priority | Item                                              | Status                        |
|----------|---------------------------------------------------|-------------------------------|
| 1        | TD-01. The 1mb_l1 question.                       | JEFF. BLOCKS THE EMITTER.     |
| 2        | Q-09. Bank placement. index_bits spans the whole  | JEFF. BLOCKS a banked         |
|          | set space; bank lsb and msb are undetermined.     | design. pacino l2 has two.    |
| 3        | The offset unit. Word count or byte count.        | JEFF. BLOCKS the read mux.    |
|          | Changes a value, not a name.                      | Cheapest of the three.        |
| 4        | Q-10. Regenerate in place or hand-own after the   | JEFF. Near-irreversible       |
|          | first emission.                                   | once adaptation starts.       |
| 5        | D-40. The generator conventions file. PLRU bit    | OPEN. Decide as the emitter   |
|          | encoding, tie break, signal naming, reset loop.   | needs them. Record them.      |
| 6        | Port occupancy. Ruled: one port, one edge.        | RULED, NOT IMPLEMENTED. T-7   |
|          |                                                   | emits no diagnostic. Q-04 in  |
|          |                                                   | cgen_decisions still reads    |
|          |                                                   | as open and leans the other   |
|          |                                                   | way. Close it. TD-15.         |
| 7        | The six schema-shadowed guard checks and          | OPEN, low. Keep as defence    |
|          | T-8.field_sum.                                    | in depth or delete. field_sum |
|          |                                                   | is an identity and should go. |
| 8        | G-9. An uninstantiated definition gets no         | OPEN, low. Ruled as an error  |
|          | geometry derivation and no arithmetic check.      | in session. Unimplemented.    |
| 9        | Q-07. Version compatibility across a file set.    | OPEN, low. A mismatch shows   |
|          |                                                   | as a schema_version const     |
|          |                                                   | violation on one file.        |
| 10       | Node and edge shape versus DOT.                   | OPEN, low. Nodes keyed by     |
|          |                                                   | name, edges an array.         |
| 11       | Q-05. A memory smaller than the address space     | OPEN, low. Whether "no tag    |
|          | decodes rather than compares.                     | compare" needs a field.       |
| 12       | Whether schemas are read from disk or compiled    | CLOSED. On disk, and          |
|          | into the binary.                                  | selectable at run time. See   |
|          |                                                   | TD-17.                        |
| 13       | Include path base directory.                      | CLOSED by implementation.     |
|          |                                                   | Relative to the including     |
|          |                                                   | file. D-10, still [PA].       |
| 14       | Install the schemas and the pacino files.         | CLOSED. Done and running.     |
| 15       | The pacino L1I VIPT alias.                        | CLOSED. Eight ways, CLI-002.  |
| 16       | The draft 7 versus 2020-12 mismatch.              | CLOSED. CLI-003. The schemas  |
|          |                                                   | use no differing construct.   |
| 17       | Node kinds beyond agent.                          | CLOSED. interconnect added.   |

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
| Style             | 80 columns, 2 space indent, ASCII, per CLAUDE.md  |

---

## Tool Checks

The schema holds shape and vocabulary. These are tool work because
the diagnostic matters more than the check. The authoritative list
of emitted codes is cli/inc/diag_codes.h; this table and that list
do not yet agree. TD-15.

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
the schema.* family.

---

## Architectural Decisions

Full detail: planning/arch/cgen_decisions.md, which is current.
Key decisions for quick reference only.

### Configuration

- Five file types. system ties files together and carries no
  graph. topology holds the graph and the addressing block.
- include appears in every file type. system is excluded from
  every include type enum.
- File name participates in the namespace: mysystem@myl1.
- Full name enumeration at load, once. The tool is a linker.
- The hierarchy is node, then interface, then port. An interface
  carries exactly one link and an optional arbitration policy. A
  port carries a role.
- An edge names node.interface.port at each end. Six required
  fields. An edge names no link; both ends carry one and must
  agree.
- node_type: icache, dcache, unified, memory, agent,
  interconnect. Only node_type and interfaces are required.
- Arbitration is on the interface. Not the link, which is point
  to point and cannot see another link. Not the node, where
  contention is array port scheduling, a different mechanism.
- A shared bus needs a node. That is what interconnect is for.
- Vocabulary is master and slave.
- A link definition is a type, not an instance. Links are
  discriminated on protocol. TileLink parameters are spec 1.9.3
  Table 4.
- No derived value in the input. No value plus units pair.
- Simulation control is not configuration.
- The schema version lives in $id and schema_version, never in a
  filename. $schema carries the JSON Schema draft, which is 7.

### Generator conventions

These live in an xxx_decisions.md file, not in the JSON, and that
file does not exist yet. Any encoding works provided one generator
emits all consumers from it.

- The tree PLRU bit encoding and the invalid-way tie break
  direction.
- Signal and port naming beyond clk and rstn.
- The non-synthesizable reset clear loop.
- Generated, not transcribed: the PLRU update and victim tables
  and the address field decomposition are built once and emitted
  to every consumer.

---

## Method note

The schema was derived by reacting to findings rather than by
walking the RTL. That method produced six versions, several
repairing regressions from their predecessor, and one silent
deletion of a working field.

The derivation that was never performed: read the 1mb_l1 RTL
module by module and classify every parameter, port and structural
choice as a config field, a derived value, or a project
convention. Until it is done the field set is not known to be
complete, only known to cover what has been reported so far.

That derivation and TD-01 are the same task. It is the one piece
of preparatory work that pays for itself before emission, because
it produces both the target dialect answer and the field list in
one pass. Everything else in the Technical Debt table can wait.

