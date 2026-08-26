<!-- SPDX-License-Identifier: Apache-2.0                       -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com -->
# CacheGen PA Session Handoff 002
```
 FILE:    cg_pa_handoff-002.md
 SOURCE:  the PA session of 2026-08-25
 STATUS:  starting context for the next cachegen session
 UPDATED: 2026.08.25
 CONTACT: Jeff Nye
```

Successor to cg_ia_handoff-001.md. That handoff described a single
flat schema and a repo with no generator. Both statements are now
out of date.

READ SECTION 8 FIRST. One decision is open and it blocks the next
schema edit.

---

## 0. Where the work is

```
  DONE        five-file input schema, all validating
  DONE        pacino worked example, five files, validating
  DONE        cgen front half: load, resolve, check, derive
  DONE        planning/arch/cgen_decisions.md
  NOT STARTED RTL emission
  NOT STARTED the C++ memory model backend
  NOT STARTED the elaborated output schema, see section 5
```

CLI-001 ran and delivered. 12 classes under cli/inc and cli/src,
gtest under cli/tb, 30 tests, 29 passing. The one failure was a stale
system schema, since fixed. cgen --cmd check runs the pacino
configuration end to end.

---

## 1. What the schema is now -- do not re-derive

THE SINGLE FLAT SCHEMA IS RETIRED. Handoff 001's
cache_config.schema.json and cache_elaborated.schema.json are both
superseded. The input is five files:

```
  system.schema.json     0.10.0  include list, nothing else
  ports.schema.json      0.12.0  port types, role only
  caches.schema.json     0.13.0  node definitions
  links.schema.json      0.12.0  connection types
  topology.schema.json   0.14.0  the graph plus addressing
```

Version lives in $id and schema_version, NEVER in the filename.
Installing a file whose name carried a version is how the stale
system schema reached the tree and cost a CLI-001 test failure.

The hierarchy, settled late in the session:

```
  node        node_type, and the type specialisation
    interface link, arbitration
      port    role, master or slave
```

An edge names node.interface.port at each end. The interface carries
the link, so an edge no longer names one. Both ends of an edge must
agree on the link; the check catches it.

node_type values: icache, dcache, unified, memory, agent,
interconnect. memory is a cache with associativity 1. agent is a
producer or consumer with interfaces and nothing else, and exists so
an edge into an L1 has a from. interconnect is the same shape.

---

## 2. Context to load, in this order

```
  planning/arch/cgen_decisions.md   41 decisions, marked [J] for
                                    Jeff, [PA] for unruled proposal,
                                    [D] for cited specification
  planning/PROJECT_STATUS.md        module status, TD, open items
  planning/schema/*.schema.json     the five above
  testcases/pacino/*.json           the worked example
  prompts/CLI-001.md                what the tool does and the nine
                                    schema gaps it found
```

Do NOT load testcases/1mb_l1. CLI-001 reports it is not in this
tree. PROJECT_STATUS carries three TD items that assume it is.

---

## 3. Decisions made this session -- do not relitigate

**Five file types, not one.** Each carries its own include list.
system is excluded from every include type enum, which is why no
shared definitions file is needed.

**The tool is a linker.** Full name enumeration across the whole
file tree at load, once. Undefined names, duplicates and type
mismatches resolve there. Interactive development is complicated by
this and that is accepted.

**Only node_type and interfaces are required.** Every field group is
optional and absence means the group does not apply. A partly
populated group is an error. The prior scheme required every field
and then needed six conditionals to relax it again.

**Group membership is in the schema, not in prose.** Conditional
required sets per node_type. Considered and rejected: putting the
table in cgen_decisions.md and having the checker implement it.
Required-field sets are what JSON Schema is for.

**Arbitration is not a link property.** A link is point to point in
both AXI and TileLink, so it cannot see another link. D-27 in
cgen_decisions.md says otherwise and is WRONG; correct it.

**A shared bus needs a node.** AXI: an interconnect is equivalent to
another device with symmetrical Manager and Subordinate ports.
TileLink's own figure shows the crossbar as an agent with several
interfaces. D-33 says a shared bus needs no node and is WRONG;
correct it.

**Vocabulary is master and slave.** Not initiator/target, not
manager/subordinate. A cache is SLAVE on its core side and MASTER on
its memory side. TileLink 1.9.3: the agent with the master interface
requests the agent with the slave interface to perform memory
operations, and its topology figure puts the slave interface on the
cache's left and the master on its right. AXI agrees: an Arm
processor is a manager, a memory controller is a subordinate.

**Simulation control is not configuration.** Clock period, cycle
limits, test selection, output directories, module prefixes and
macro names are CLI flags or tool requirements. A generation block
was proposed, rejected, reintroduced by the IA and removed again.

**Derived values never appear in the input.** sets, tag width,
offset widths, refill beat count and byte-enable widths are computed.

**Tool decisions.** C++20 not C++23, namespace cgen, one class per
file, Make never CMake, nlohmann, pboettch json-schema-validator,
Boost.ProgramOptions, gtest, --cmd={check,emit}, no positionals,
--output default ./output, --eoe off by default.

---

## 4. Traps -- do not rediscover

```
  TileLink has used two OPPOSITE vocabularies. Spec 0.3.3 calls
      the memory side "manager". Specs 1.7 and 1.9.3 use
      master/slave, where master is the requesting side. AXI's
      "manager" is the requesting side. Same word, opposite ends.
      This cost several turns this session.
  pboettch json-schema-validator implements DRAFT 7 and erases
      $schema. The schemas declare 2020-12. Every reject case in
      this session was run under a 2020-12 validator in Python,
      NOT under the tool's own validator. Conditionals without a
      fixture are unverified under draft 7. See section 8.
  The version-in-filename trap of section 1.
  An emitter bug is one bug per output. Carried from handoff 001
      and still true.
```

---

## 5. What CLI-001 found -- nine gaps, G-1 through G-9

Closed this session:

```
  G-1  group membership undefined -> now conditional required
       sets in caches.schema.json
  G-2  page_bytes optional -> now required in addressing
  G-5  edge from_port/to_port optional -> now required
  G-6  system schema stale -> corrected, 0.10.0 installed
  G-7  fixture workaround forced by G-6 -> gone with G-6
```

Still open:

```
  G-3  BANK PLACEMENT. Nothing states whether index_bits spans
       the whole set space or one bank. R-7's
       offset+index+tag == pa_bits forces the whole-set reading,
       which is what the tool computes. The bank field lsb and
       msb are not emitted because the schema does not determine
       them. NEEDS A DECISION FROM JEFF.
  G-4  initiator-end occupancy. Superseded by the node/interface
       /port model; re-check rather than answer as written.
  G-8  refill beat count with several downstream links. Also
       superseded: beat count is per interface.
  G-9  a cache definition no node instantiates gets no geometry
       derivation and no arithmetic check. An uninstantiated
       definition with impossible geometry passes. Tool fix.
```

The elaborated output schema does not exist in the new shape.
Handoff 001's cache_elaborated.schema.json predates everything. The
emitter needs derived values from somewhere; deciding whether that
is a file or in-memory state is section 8 work.

---

## 6. State of the pacino example

Validates against all five schemas. Every geometry and timing number
in it is invented and is there to be edited.

```
  ifu, lsu   agents
  l1i        icache 32KB 4-way 64B VIPT
  l1d        dcache 32KB 8-way 64B VIPT
  l2         unified 512KB 8-way 64B PIPT, 2 banks
  mem        memory 1GB
```

THE VIPT CHECK FIRES ON l1i AND THAT IS CORRECT. 32KB 4-way is 8192
bytes per way against a 4096 byte page, so the index reaches above
the page offset and the configuration has synonyms. Fixes are 16KB
4-way, 32KB 8-way, or PIPT on the I side. Handoff 001 section 6
already derived this for the pacino icache and concluded eight ways:
bytes_per_way must not exceed the page size, so a way caps at 4KB and
32KB needs eight of them. That solve is still right and the current
example contradicts it.

CAUTION ON THE L2 UPSTREAM SHAPE. The example currently gives l2 ONE
upstream interface with two slave ports, i and d, sharing one link
type. That was a modeling choice made while rewriting the example,
not a constraint, and it forces the I side and D side to share a
TileLink conformance level. Two separate upstream interfaces, each
with its own link, is equally legal and preserves TL-UH on the I
side. See section 8.

---

## 7. What was verified this session -- do not re-run

```
  five schemas well formed, draft 2020-12
  pacino: all five files valid
  five edges: role correct at both ends, both ends agree on link
  caches.schema.json group membership, 15 accept/reject cases
  topology, links, ports, system: reject cases per conditional
  the derived geometry for every pacino node
```

All of it under a Python 2020-12 validator. See the draft 7 trap.

---

## 8. Open questions -- these gate the next task

```
  1. ARBITRATION PLACEMENT. Node or interface? The session
     settled that it is NOT the link, because links cannot see
     each other. It did not settle node versus interface.

     The argument for the node: contention is for the resource
     BEHIND the interfaces, the tag and data arrays. Two
     upstream interfaces contend for that array exactly as two
     ports on one interface do. STBus puts arbitration on the
     Node, which manages up to 32 initiators and 32 targets and
     supports six arbitration types.

     The argument for the interface: it is the smallest scope
     that sees all contenders on one link.

     Current state: arbitration is on the INTERFACE in
     caches.schema.json 0.13.0. If it moves to the node, the
     example's l2 should also go back to two upstream
     interfaces with their own link types.

  2. G-3, bank placement. Section 5.

  3. Is there an elaborated output file at all, or does the
     emitter hold derived values in memory? It buys provenance
     and a regeneration diff, neither of which blocks emission.

  4. Handoff 001 question 3, still open: is emitted RTL
     regenerated in place or hand-owned after first emission?
     Close to irreversible once adaptation starts.

  5. DW-8, still open: is testcases/1mb_l1 a pacino template or
     a generic reference? Decides whether the emitter targets
     SystemVerilog-2023 with a translation first, or writes
     fresh. CLI-001 reports the directory is not in this tree.
```

Handoff 001 questions 1, 2, 5 and 6 are ANSWERED. Continue in
jeffnye-gh/cachegen. C++. banks is in geometry. The ITLB is left to
the integrator.

---

## 9. Next actions, in order

```
  1. Answer question 1 of section 8. It is a one-line schema
     move either way and it should not sit open.
  2. Correct D-27 and D-33 in cgen_decisions.md. Both are
     recorded WRONG in section 3 above and neither has been
     edited.
  3. Update CLI-001, or write CLI-002 against it. CLI-001 still
     describes ports as a flat name-to-type map. That is stale
     against the node/interface/port model.
  4. Run the existing reject cases through the TOOL's validator
     rather than through Python. The draft 7 trap. One task.
  5. Edit the pacino geometry. The l1i VIPT alias is the first
     thing to fix and handoff 001 already has the answer.
  6. Then RTL emission.
```

---

## 10. Working method

Carried from handoff 001 and reinforced by this session.

```
  - State findings plainly. No metaphors, no picturesque
    phrasing.
  - Verify before asserting, and verify before CONCEDING. The
    PA stated the correct master/slave orientation, was pushed
    back on, abandoned it, and spent several turns recovering
    a position the specifications confirmed. Agreeing to end
    friction is worse than being wrong, because the reader
    cannot tell which one they are getting.
  - Search the specifications early. Four questions this
    session were settled by AXI, TileLink, IP-XACT and STBus
    documents, and all four were argued from first principles
    for several turns before anyone looked.
  - Do not report a self-imposed modeling choice as a
    constraint the model forces. The l2 shared-interface case
    in section 6 is an instance of this.
  - Boilerplate and mechanical schema work is IA work. The PA
    is for the model.
```

---

## 11. Document History

```
  2026-08-23  001  IA session. Parameter vocabulary and the two
                   flat JSON schemas. No generator code.
  2026-08-25  002  PA session. Flat schema retired, five-file
                   input schema, node/interface/port model,
                   CLI-001 delivered the tool's front half.
```

