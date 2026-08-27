<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->

```
 FILE:    cgen_decisions.md
 SOURCE:  PA session, schema derivation
 STATUS:  DRAFT
 UPDATED: 2026-08-26
 CONTACT: Jeff Nye
```

---
# Scope

Decisions governing the cgen input configuration and the tool that
reads it. Covers the file set, the load model, the division of work
between JSON Schema and tool code, and the content of each schema.

Does not cover the elaborated output. That schema has not been
revised in this line of work and is stale against everything below.

Provenance is marked on every decision. [J] is Jeff's call. [PA] is
a proposal that has not been ruled on. [D] is derived from an
external specification and is cited.

A decision that was reversed keeps its number and is marked
CORRECTED, with the date and the reason. Other documents cite these
numbers, so they do not get renumbered or deleted.

---
# Terms

```
node        an instance in the topology graph
node def    an entry in a caches file, instantiable more than once
interface   a named group of ports on a node def. Carries exactly
            one link and, optionally, an arbitration policy
port        a named endpoint inside an interface, carrying a type
link def    an entry in a links file, a connection type
port type   a named endpoint kind with a role
master      the end of a link that issues requests
slave       the end that responds
```

Vocabulary is master and slave. Not initiator and target, not
manager and subordinate. TileLink 1.9.3: the agent with the master
interface requests the agent with the slave interface to perform
memory operations, and the topology figure puts the slave interface
on the cache's core side and the master on its memory side. AXI
agrees, an Arm processor is a manager and a memory controller is a
subordinate.

TRAP. TileLink 0.3.3 calls the memory side "manager", the opposite
of AXI's use of the same word. Specs 1.7 and 1.9.3 use master and
slave and are the ones followed here.

---
# File set

Five file types. Each carries schema_version, file_type, and an
optional include list.

```
  system      root file, include list and nothing else
  caches      node definitions, caches and memories and agents
  links       connection type definitions
  ports       port type definitions
  topology    the graph: node instances and typed edges
```

D-01 [J] Five separate schemas rather than one. Splitting cache from
     link from topology lets each be worked independently.

D-02 [J] include appears in every file type, not only the root. No
     reason to special case the top level.

D-03 [J] system is excluded from every include type enum. A system
     file cannot include another system file. This removes the only
     inconsistency between the schemas and needs no shared
     definitions file.

D-04 [J] The system file ties files together and nothing else. It
     carries no graph. Raised and withdrawn in session.

D-05 [J] topology is a separate file type and holds the graph. The
     system schema as written is correct: include only.

---
# Load model

D-06 [J] The tool performs a full name enumeration across the whole
     file tree at load, once. This is the linking step: it resolves
     references, detects undefined names, and detects duplicates.

D-07 [J] Doing this once complicates interactive development. That
     limitation is accepted at this stage.

D-08 [J] File name participates in the namespace. Form is
     mysystem@myl1.

D-09 [PA] include cycles must be detected. Not discussed; stated
     here because include is now recursive.

D-10 [PA] Include paths need a stated base directory. Relative to
     the including file and relative to the system root are both
     defensible and they differ once includes nest.

     The tool resolves relative to the including file. That is an
     assumption recorded in CLI-001 and exercised by the
     Loader.NestedIncludeResolvesAgainstTheIncludingFile test, not
     a ruling.

---
# Schema versus tool

D-11 [J] The schema holds data and expresses syntax. It does not
     validate data fields. Cross-field arithmetic, name resolution
     and type compatibility are tool work.

D-12 [J] The schema should do as much as it can, to save tool code.
     Qualified by D-13.

D-13 [PA, accepted by J] Shape and vocabulary in the schema, since
     that is what it is for and its errors are self explanatory.
     Anything where the diagnostic matters more than the check goes
     in the tool. jsonschema reports a failed oneOf as "not valid
     under any of the given schemas", which is not a usable message.

D-14 [J] dependentRequired is not used. Group completeness is a tool
     check so that it can produce a real message. See D-46 for the
     boundary as it actually stands in caches.schema.json 0.13.0.

D-45 [J] 2026-08-26. A check the tool can perform by reasoning over
     what is already in the file does not become a schema
     requirement. Three cases were raised and all three were ruled
     tool work:

       arbitration on an interface with one port is meaningless
       an interface holding ports of both roles is legal but has
         no use, and needs no schema marker
       which role an interface arbitrates does not need naming

     The rule behind all three: if stating it in the schema only
     protects against a configuration nobody would write, leave it
     out and let the tool reason.

## Tool checks, enumerated

```
  T-1  undefined name: edge endpoint, edge interface, edge port,
       node def ref, interface link ref, port type
  T-2  duplicate definition across files
  T-3  port type compatibility on both ends of every edge, against
       the master_port_type and slave_port_type the link declares
  T-4  port role matches edge direction, from is master, to is slave
  T-5  graph terminates, no cycles
  T-6  group completeness: a field group is wholly present or
       wholly absent; a partly populated group is an error. See
       D-46 for which groups this still owns
  T-7  port occupancy: whether one slave port may host more than
       one edge. See Q-04
  T-8  cross field arithmetic: capacity, line, associativity, sets,
       tag width, bank divide, VIPT index budget
  T-9  link agreement: the interfaces at the two ends of an edge
       must name the same link definition. See D-43
```

---
# Node definitions

D-15 [J] CORRECTED 2026-08-26. Was: cache_type is a declared field,
     values icache, dcache, unified, memory.

     Now: node_type is a declared field. Values icache, dcache,
     unified, memory, agent, interconnect. The field was renamed
     because the caches file holds more than caches. agent is a
     producer or consumer with interfaces and nothing else, and
     exists so an edge into an L1 has a from. interconnect is the
     same shape and exists for D-33.

D-16 [J] memory is a node_type, not a separate file type or
     schema. Bulk memory is a cache with associativity 1.

     PA had argued this was false, on the grounds that a memory
     would have to declare replacement and miss policies it never
     runs and tag and status arrays it does not have. J rejected
     that reasoning twice: an enum can carry a none value, and the
     arrays are derived widths that fall to zero. The residual
     objection, that a memory smaller than the address space
     decodes rather than compares, is unresolved. See Q-05.

D-17 [J] CORRECTED 2026-08-26. Was: only cache_type and geometry
     are required.

     Now: only node_type and interfaces are required. geometry is
     required by conditional for the four node types that have one,
     and forbidden for agent and interconnect. Every other field
     group is optional and absence means the group does not apply.

     This replaced a scheme in which every field was required. The
     required-everything scheme was PA's and it generated six
     conditionals whose only job was to relax it again. Omission
     handles mshr_targets at zero MSHRs, bank interleave at one
     bank, beat order at one beat, and inclusion at a terminal node
     without any of them.

D-18 [J] "Doesn't apply" versus "forgot to say" is a non problem.
     If everything necessary for a cache is absent or none, it is a
     memory. If only part is specified it is an error. T-6.

D-19 [PA] The conditionals that survive in the cache schema are the
     ones that catch a contradiction rather than an inapplicable
     field:

       icache forbids write_hit, write_miss, dirty_bits, and forces
         flush_line and flush_all false
       memory forbids indexing, policies, inclusion, miss_handling,
         fill, maintenance, the tag and status arrays, and pins
         associativity to 1
       cache types forbid init and range_check
       agent and interconnect forbid every group except interfaces
       write_hit write_back requires dirty_bits

D-20 [J] CORRECTED 2026-08-26. Was: ports are typed references on
     the cache node.

     Now: interfaces are named groups on the node definition, and
     ports are typed references inside an interface. What happens
     with those ports inside the cache is tool work for now.

D-21 [PA] No port bodies in the cache schema. Protocol, widths and
     handshake live on the link, because both ends must agree on
     them by construction. Stating them on the node wrote the same
     fact twice and gave a shared node one port definition for
     several different attached caches.

D-42 [J] 2026-08-26. The hierarchy is node, then interface, then
     port.

```
       node        node_type, and the type specialisation
         interface link, arbitration
           port    role, master or slave
```

     An interface is the smallest scope that sees every contender
     on one link, which is why arbitration lives there, D-27. It is
     also what the emitter needs, since an arbiter is sized by the
     port list it serves and that list is exactly one interface.

D-46 [J] 2026-08-26. Group completeness is the tool's, T-6. The six
     unconditional required lists that had accumulated inside the
     group objects were removed from caches.schema.json, since each
     one duplicated a T-6 member and produced a second, worse
     diagnostic for the same defect. R-07 already rejected blanket
     required lists on every field group.

     The per-node_type conditionals of D-19 stay, so the boundary
     is not clean and is stated here rather than left to be
     rediscovered. The schema still carries required member lists
     inside three conditional branches:

```
       icache, dcache, unified   storage and timing members
       dcache, unified           policies and timing members
       write_hit == write_back   storage dirty_bits
```

     T-6 is therefore the only check for miss_handling, fill and
     maintenance on every node type, for policies on an icache, and
     for storage and timing on a memory. Everywhere else the schema
     reaches the defect first and T-6 is redundant.

---
# Link definitions

D-22 [J] Links must express TileLink and the ad hoc interfaces in
     the current example RTL.

D-23 [PA] Discriminated on protocol. A named standard needs only
     its parameters; an ad hoc interface needs a structural
     description. This mirrors the IP-XACT split between a
     busDefinition and an instance of it.

D-24 [D] TileLink per-link parameters, spec 1.9.3 Table 4:
```
       w  1-4096   data bus width in bytes, power of two
       a  0-128    address field width in bits
       z  1-4      size field width in bits
       o  0-64     master source disambiguation bits
       i  0-64     slave sink disambiguation bits
```
     Conformance levels TL-UL, TL-UH, TL-C. TL-UL is read and write
     only. TL-UH adds multibeat, atomics and hints. TL-C adds cache
     block transfers and channels B, C and E.
     Source: sifive tilelink_spec_1.9.3.pdf, verified in session.

D-25 [PA] The custom protocol body decomposes the handshake into
     three independent choices: request_strobes, accept, and
     read_data_return. The 1mb_l1 core port is separate read and
     write strobes with ready and a valid flag return, which is not
     valid_ready, not stall and not AXI-lite. The same object also
     describes the main memory port, where nothing throttles the
     request and mm_ready is a write completion rather than an
     accept.

D-26 [PA] A link definition is a type, not an instance. It carries
     no from or to. It declares master_port_type and
     slave_port_type. Interfaces reference it by name. Several
     interfaces naming one link definition are attachments to one
     bus.

D-27 [PA] CORRECTED 2026-08-26. Was: arbitration belongs on the
     link definition, since that is the bus. That is WRONG.

     A link is point to point in both AXI and TileLink, so a link
     cannot see another link and cannot arbitrate between them.
     Arbitration is on the INTERFACE, which is the smallest scope
     that aggregates every port contending for one link.

     Node was the other candidate and was rejected. The argument
     for it was that contention is really for the tag and data
     arrays behind the interfaces. That is a different mechanism:
     array port scheduling, resolved by a fixed pipeline priority
     or by the array read port count, not by an arbiter with a
     policy. One field on the node would make two unlike things
     look like one knob.

     STBus was cited for the node. It does not support it. An STBus
     Node is a standalone crossbar module whose entire content is
     its attachments, which in this model is an interconnect node
     whose only members are interfaces. STBus places arbitration in
     the thing that aggregates the contending ports, and here that
     thing is the interface.

     The field is optional, enum none, fixed_priority, round_robin,
     weighted. Nothing in the tool consumes it yet. See Q-08.

---
# Port definitions

D-28 [J] A ports file declares port types. Simple enums are enough
     for now.

D-29 [J] CORRECTED 2026-08-26. Was: a port type carries a role,
     initiator or target.

     Now: a port type carries a role, master or slave. The field
     stays, T-4 has no basis without it. Only the vocabulary
     changed, see Terms.

D-30 [PA] CORRECTED 2026-08-26. Was: an edge names the port
     instance on each end rather than letting the tool infer it
     from the type.

     Now: an edge names node.interface.port on each end. Type alone
     cannot say which edge lands where when a node has three slave
     ports of one type, and interface alone cannot either once an
     interface holds more than one port.

---
# Topology

D-31 [J] A graph of named nodes with typed edges. Names resolve
     against the caches list, types against the links list.

D-32 [J] Typed edges are the departure from DOT that matters. DOT
     edge attributes are literals on the edge; here the type is a
     name resolved against a definition, so several edges naming
     one type are the same bus. DOT has no definition namespace to
     resolve against.

D-33 [J] CORRECTED 2026-08-26. Was: a shared bus does not need a
     bus node, the edge type carries it. That is WRONG.

     A shared bus needs a node. AXI treats an interconnect as
     another device with symmetrical Manager and Subordinate ports.
     TileLink's own topology figure draws the crossbar as an agent
     with several interfaces. interconnect was added to the
     node_type enum by D-15 for this.

D-34 [J] Direction and multiplicity are handled by any DAG
     representation.

D-35 [PA] Nodes are instances. The node key is the instance name
     and a cache field names the definition, so one node definition
     can be instantiated on two cores. Naming a node the same as
     its definition gives the one to one case.

D-36 [PA] SUPERSEDED 2026-08-26 by D-15. The attach object of core
     and port is gone. A requester is an agent node like any other,
     so there is no magic string and no first class core type. This
     also answers Q-02.

D-43 [J] 2026-08-26. The link is carried by the interface, not by
     the edge. An edge therefore names no link. Both ends of an
     edge carry one, and they must name the same definition, which
     is T-9.

     The alternative, keeping link on the edge as well, writes the
     same fact in three places and lets two of them disagree
     silently.

D-44 [J] 2026-08-26. An edge is
     from, from_interface, from_port, to, to_interface, to_port,
     all required, plus an optional name. Six required fields is
     the price of naming both ends unambiguously.

---
# Standing conventions

Carried from earlier work. Not re-argued.

D-37 No derived value appears in the input. sets, tag width, the
     word offset field, the bank field and the refill beat count
     are computed by the tool.

D-38 No value plus units pair anywhere in the input. Integers, in
     bytes and bits. A rendered string may appear in output for a
     datasheet and is never read back.

D-39 Generated, not transcribed. The PLRU update and victim tables
     and the address field decomposition are built once by the tool
     and emitted to every consumer. Four hand maintained copies
     that happen to agree is not one source.

D-40 [J] Generator conventions live in an xxx_decisions.md file in
     the repo, not in the JSON. This covers the tree PLRU bit
     encoding, the invalid way tie break direction, signal and port
     naming beyond clock and reset, and the non synthesizable reset
     clear loop. Any encoding works provided one generator emits
     all consumers from it.

D-41 [J] Simulation control is not configuration. Clock period,
     cycle limits, reset cycle counts, waveform enables, test
     selection, output directories, module prefixes and macro names
     are tool requirements or CLI flags.

D-47 [J] The schema version lives in $id and in schema_version, and
     NEVER in a file name. A versioned file name is how a stale
     system schema reached the tree and cost a CLI-001 test
     failure. The five schemas are

```
       planning/schema/system.schema.json
       planning/schema/ports.schema.json
       planning/schema/caches.schema.json
       planning/schema/links.schema.json
       planning/schema/topology.schema.json
```

D-48 [J] Tool decisions, from planning/tools/tool_decisions.md and
     restated here because they bound everything above: C++20 not
     C++23, namespace cgen, one class per file, Make never CMake,
     nlohmann json, pboettch json-schema-validator, Boost
     ProgramOptions, gtest, --cmd={check,emit}, no positionals,
     --output defaults to ./output, --eoe off by default.

---
# Open questions

Q-04 May one slave port host more than one edge? Two edges aimed at
     the same node.interface.port currently type check cleanly. The
     tool records occupancy and reports nothing. If a bus is
     expressed by several edges naming one link, this is legal; if
     a port is point to point, it is an error and the shared case
     is expressed by several ports on one interface instead.

     The interface model makes the second reading the more likely
     one, since an interface with several ports is now the natural
     way to write a shared bus. Not ruled on.

Q-05 A memory smaller than the address space decodes rather than
     compares. Whether "no tag compare" needs a field, or follows
     from something already present, was not worked through.

Q-06 The elaborated output schema is untouched and stale. It still
     carries the old artifact kind enum and has no home for fields
     added since. Nothing in this document has been reconciled
     against it. Whether there is an elaborated output FILE at all,
     or the emitter holds derived values in memory, is also open.
     A file buys provenance and a regeneration diff, neither of
     which blocks emission.

Q-07 Version compatibility across files. Each file carries its own
     schema_version and the tool has to reject a mismatched set. It
     does not yet; each file is validated against its own schema
     and a mismatch shows up as a schema_version const violation
     rather than as a set-level message.

Q-08 Nothing consumes arbitration. The field is accepted and
     carried, and no stage reads it. It becomes live when the
     emitter needs to size an arbiter, and until then a wrong value
     is not detected.

Q-09 G-3, bank placement. Nothing states whether index_bits spans
     the whole set space or one bank. The R-7 rule that
     offset + index + tag == pa_bits forces the whole-set reading,
     which is what the tool computes. bank_select_position says
     above_index or below_index but the schema does not determine
     the field lsb and msb, so they are not emitted. Needs a
     ruling.

Q-10 Is emitted RTL regenerated in place or hand-owned after the
     first emission? Close to irreversible once adaptation starts.

Q-11 DW-8. Is testcases/1mb_l1 a pacino template or a generic
     reference? Decides whether the emitter targets
     SystemVerilog-2023 with a translation first, or writes fresh.
     CLI-001 reports the directory is not in this tree.

---
# Rejected

R-01 [J] A generation block in the config, holding testbench and
     emission control. Withdrawn once, reintroduced by the IA, and
     removed again. See D-41.

R-02 [J] GLOSSARY.md as a document. Design decisions do not belong
     in a file named glossary. Its check table becomes machine
     readable data, its scope lists become their own document, its
     standing rules move here, and the rest goes.

R-03 [J] A shared definitions file plus cross file $ref, proposed
     to keep one enum consistent across two schemas. Three values
     that will not change do not need infrastructure. D-03 solved
     it by deleting one value.

R-04 [PA] inclusion as a link parameter. No precedent, invented and
     withdrawn in consecutive messages. Inclusion is a property of
     the cache.

R-05 [PA] single_beat as a fill beat order value. Beat count is
     derived from line size and the downstream read width. The tool
     records the field not applicable.

R-06 [PA] protocol as an enum alongside a structural handshake
     description. The two decided the same thing and could
     contradict each other.

R-07 [PA] Blanket required lists on every field group. See D-17 and
     D-46.

R-08 [PA] Keeping link on the edge as well as on the interface, as
     a redundant cross check. See D-43.

---
# Current state

Schemas, all validating, with accept and reject cases for every
conditional. Versions are in $id and schema_version, D-47.

```
  system.schema.json     0.10.0   unchanged
  ports.schema.json      0.11.0   unchanged
  links.schema.json      0.12.0   arbitration removed, D-27
  caches.schema.json     0.13.0   node/interface/port, D-42
  topology.schema.json   0.14.0   edge names node.interface.port
```

The tool front half loads, resolves, checks and derives against
these. cgen --cmd=check on testcases/pacino runs end to end and
produces no diagnostics. The gtest suite is 31 of 31, one negative
fixture per diagnostic plus the pacino positive fixture.

pacino's l1i was 32KB over four ways, which put 8192 bytes in a way
against a 4096 byte page and left index bit 12 above the page
offset, so one physical line could land in two sets. A VIPT way
must not exceed a page, so at a 4KB page a way caps at 4KB and 32KB
needs eight of them. l1i is now eight way, matching l1d, and both
L1s sit exactly at the budget with the index ending at bit 11.

TRAP. pboettch json-schema-validator implements DRAFT 7 and erases
$schema. The schemas declare 2020-12. Everything the suite exercises
runs under the tool's draft 7 validator, so the reject cases are
verified there. Any conditional without a fixture is verified only
under a 2020-12 validator in Python and is not known to behave the
same way in the tool.

---
# Method note

The schema was derived by reacting to findings rather than by
walking the RTL. Six versions were produced, several of them
repairing regressions introduced by the previous one, and
cache_type was silently deleted and later restored.

The derivation that was never performed: read examples/1mb_l1/rtl
module by module and classify every parameter, port and structural
choice as a config field, a derived value, or a project convention.
That produces the field list in one pass. Until it is done, the
field set is not known to be complete, only known to cover what has
been reported so far. See Q-11.
