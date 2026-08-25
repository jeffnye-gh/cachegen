<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->

```
 FILE:    cgen_decisions.md
 SOURCE:  PA session, schema derivation
 STATUS:  DRAFT
 UPDATED: 2026-08-25
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

---
# Terms

```
node        an instance in the topology graph
cache def   an entry in a caches file, instantiable more than once
link def    an entry in a links file, a connection type
port type   a named endpoint kind with a role
initiator   the end of a link that issues requests
target      the end that responds
```

---
# File set

Five file types. Each carries schema_version, file_type, and an
optional include list.

```
  system      root file, include list and nothing else
  caches      cache and memory definitions
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
     check so that it can produce a real message.

## Tool checks, enumerated

```
  T-1  undefined name: edge endpoint, cache ref, link ref, port type
  T-2  duplicate definition across files
  T-3  port type compatibility on both ends of every edge
  T-4  port role matches edge direction
  T-5  graph terminates, no cycles
  T-6  group completeness: a field group is wholly present or
       wholly absent; a partly populated group is an error
  T-7  port occupancy: whether one target port may host more than
       one edge. See Q-04.
  T-8  cross field arithmetic: capacity, line, associativity, sets,
       tag width, VIPT index budget
```

---
# Cache definitions

D-15 [J] cache_type is a declared field. Values icache, dcache,
     unified, memory.

D-16 [J] memory is a cache_type, not a separate file type or
     schema. Bulk memory is a cache with associativity 1.

     PA had argued this was false, on the grounds that a memory
     would have to declare replacement and miss policies it never
     runs and tag and status arrays it does not have. J rejected
     that reasoning twice: an enum can carry a none value, and the
     arrays are derived widths that fall to zero. The residual
     objection, that a memory smaller than the address space
     decodes rather than compares, is unresolved. See Q-05.

D-17 [J] Only cache_type and geometry are required. Every field
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

D-19 [PA] Three conditionals survive in the cache schema and are
     the only ones that catch a contradiction rather than an
     inapplicable field:
       icache forbids write_hit, write_miss, dirty_bits, and forces
         flush_line and flush_all false
       memory forbids indexing, policies, inclusion, miss_handling,
         fill, maintenance, the tag and status arrays, and pins
         associativity to 1
       cache types forbid init and range_check

D-20 [J] Ports are typed references on the cache node. What happens
     with those ports inside the cache is tool work for now.

D-21 [PA] No port bodies in the cache schema. Protocol, widths and
     handshake live on the link, because both ends must agree on
     them by construction. Stating them on the node wrote the same
     fact twice and gave a shared node one port definition for
     several different attached caches.

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
     Source: sifive tilelink_spec_1.9.3.pdf, verified this session.

D-25 [PA] The custom protocol body decomposes the handshake into
     three independent choices: request_strobes, accept, and
     read_data_return. The 1mb_l1 core port is separate read and
     write strobes with ready and a valid flag return, which is not
     valid_ready, not stall and not AXI-lite. The same object also
     describes the main memory port, where nothing throttles the
     request and mm_ready is a write completion rather than an
     accept.

D-26 [PA] A link definition is a type, not an instance. It carries
     no from or to. Topology edges reference it by name. Several
     edges naming one link definition are attachments to one bus.

D-27 [PA] Arbitration belongs on the link definition, since that is
     the bus.

---
# Port definitions

D-28 [J] A ports file declares port types. Simple enums are enough
     for now.

D-29 [PA] A port type carries a role, initiator or target. Without
     it nothing distinguishes the two ends and T-4 has no basis. If
     the role is to come from a naming convention instead, drop the
     field.

D-30 [PA] An edge names the port instance on each end rather than
     letting the tool infer it from the type. l3u has three target
     ports of the same type, so type alone cannot say which edge
     lands where.

---
# Topology

D-31 [J] A graph of named nodes with typed edges. Names resolve
     against the caches list, types against the links list.

D-32 [J] Typed edges are the departure from DOT that matters. DOT
     edge attributes are literals on the edge; here the type is a
     name resolved against a definition, so several edges naming
     one type are the same bus. DOT has no definition namespace to
     resolve against.

D-33 [J] A shared bus does not need a bus node. The edge type
     carries it.

D-34 [J] Direction and multiplicity are handled by any DAG
     representation.

D-35 [PA] Nodes are instances. The node key is the instance name
     and a cache field names the definition, so one cache
     definition can be instantiated on two cores. Naming a node the
     same as its cache gives the one to one case.

D-36 [PA, contested] Entry points are an attach object of core and
     port. J challenged core as a first class type: it is a magic
     string with no definition, and a core is only one kind of
     requester. DMA engines, accelerators, prefetchers, IO bridges
     and debug ports all issue requests. See Q-02.

---
# Standing conventions

Carried from earlier work. Not re-argued.

D-37 No derived value appears in the input. sets, tag width, the
     word offset field and the refill beat count are computed by
     the tool.

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

---
# Open questions

Q-02 Is a node kind needed, and what are the kinds? A core is
     currently a bare string inside attach. Requesters other than
     cores exist. A consistent model would make every graph
     participant a node with a kind, which would also remove the
     asymmetry of memory being a cache_type while an initiator is a
     magic string.

Q-03 Node and edge shape. Nodes are an object keyed by name, edges
     are an array of objects. Two shapes for two halves of one
     graph. DOT gives both the same shape. Whether the DOT
     correspondence should be structural or notational, that is,
     round trippable to a .dot file for viewing, was raised and not
     settled.

Q-04 May one target port host more than one edge? Two edges aimed
     at the same port currently type check cleanly. If a bus is
     expressed by several edges sharing a link type, this is legal;
     if a port is point to point, it is an error. T-7 needs a rule.

Q-05 A memory smaller than the address space decodes rather than
     compares. Whether "no tag compare" needs a field, or follows
     from something already present, was not worked through.

Q-06 The elaborated output schema is untouched and stale. It still
     carries the old artifact kind enum and has no home for fields
     added since. Nothing in this document has been reconciled
     against it.

Q-07 Version compatibility across files. Each file carries its own
     schema_version and the tool has to reject a mismatched set.

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

R-07 [PA] Blanket required lists on every field group. See D-17.

---
# Current state

Schemas at 0.10.0, all validating with python jsonschema 4.26.0,
draft 2020-12, with accept and reject cases for every conditional:

```
  cgen_ports_0.10.0.schema.json
  cgen_caches_0.10.0.schema.json
  cgen_links_0.10.0.schema.json
  cgen_topology_0.10.0.schema.json
  cgen_system_0.10.0.schema.json
```

Worked examples: an icache, a dcache, a unified L1, a memory, three
TileLink conformance levels, the two 1mb_l1 ad hoc interfaces, and a
two processor topology with split L2 on one core, unified on the
other, sharing an L3 into DRAM. The port type check has been
demonstrated catching a protocol mismatch and a role mismatch.

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
been reported so far.

