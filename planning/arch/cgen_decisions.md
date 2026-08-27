<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->

```
 FILE:    cgen_decisions.md
 SOURCE:  PA session, schema derivation
 STATUS:  DRAFT
 UPDATED: 2026-08-27
 CONTACT: Jeff Nye
```

---
# Scope

Decisions governing the cgen input configuration and the tool that
reads it. Covers the file set, the load model, the division of work
between JSON Schema and tool code, the content of each schema, and
the conventions the emitter applies to generated output.

Does not cover the elaborated output. That schema has not been
revised in this line of work and is stale against everything below.

D-40 says generator conventions live in their own file. That file
still does not exist. The emission decisions recorded here are the
ones that constrain the CONFIGURATION or reverse an earlier entry.
The remaining 17, decided by CLI-004 and reported in
prompts/CLI-004.md, are not here and are not anywhere else.

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
     a ruling. It has since become load-bearing in shipped output.

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

D-51 [J] 2026-08-27. THE TOOL'S DIAGNOSTIC CODES ARE DECLARED ONCE.
     cli/inc/diag_codes.h holds every code the tool can emit, and
     every emission site draws its string from that list rather
     than carrying a literal.

     Each code carries how it is reached:

```
       Fixture  a configuration or prepared schema directory
                under cli/tb/fixtures produces it, and one must
       Guard    no schema valid configuration reaches it, because
                the schema or the loader rejects the input first
       Env      reachable only from a filesystem or permission
                failure a fixture cannot carry
```

     A test relates the list to what the fixtures produce, in both
     directions: a Fixture code that nothing produces fails, and a
     Guard or Env code that something produces fails.

     The rule this came from: BEFORE THE MECHANISM EXISTED, 21 OF
     37 CODES HAD NO FIXTURE while the suite was green, and T-3 had
     been dead since CLI-001. A check that fires on nothing and a
     check that passes are indistinguishable in the output, so the
     relationship has to be asserted rather than assumed.

     The check enumeration below and this list do not yet agree.
     See Q-12.

D-52 [J] 2026-08-27. FIELD CONSUMPTION IS RECORDED, and two rules
     decide whether the record means anything.

     PRESENCE IS NOT A READ. A check that asks whether a field
     exists and never looks at its value does not mark it consumed.
     T-6 group completeness is the case: without this rule the
     whole maintenance group and most of miss_handling count as
     read.

     THE RECORD IS MADE IN THE ACCESSOR, not where the value is
     extracted. NodeCtx::build extracts inclusion into a member and
     nothing calls NodeCtx::inclusion(). Marking at extraction
     would report that field consumed. Four of fifteen accessors
     have no caller.

     Either rule inverted produces a report that appears to work
     and measures nothing. A stage that starts consuming a field
     marks it in the same change that makes it live.

     The measured result for pacino is 74 unconsumed leaves of 287.
     The estimate before the mechanism existed was seven.

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
  T-7  port occupancy. RULED one port, one edge, Q-04. NO CODE AND
       NO CHECK. See Q-12
  T-8  cross field arithmetic: capacity, line, associativity, sets,
       tag width, bank divide, VIPT index budget
  T-9  link agreement: the interfaces at the two ends of an edge
       must name the same link definition. See D-43
```

Unnumbered but emitted: topology.addressing, and the load.*,
schema.* and emit.* families. See Q-12.

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
       banks >= 2 requires bank_interleave_granularity, and no
         longer requires bank_select_position. See D-49

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
     maintenance on every node type EXCEPT memory, which D-19
     forbids those groups on outright, for policies on an icache,
     and for storage and timing on a memory. Everywhere else the
     schema reaches the defect first and T-6 is redundant.

     CLI-003 found the same pattern in geometry: six T-8 and load
     codes are unreachable because the schema rejects the input
     before the tool's check runs. That is this boundary appearing
     somewhere it was not recorded. See Q-12.

D-49 [J] 2026-08-27. THE BANK FIELD POSITION FOLLOWS FROM
     bank_interleave_granularity. bank_select_position is DELETED
     from the schema and from every configuration.

     line granularity means consecutive lines alternate banks. That
     is the definition, and it puts the select at the bits
     IMMEDIATELY ABOVE THE OFFSET, which is the bottom bank_bits of
     the index. The set index is what is left, above it.

     The field was deleted rather than chosen between because one
     configuration can carry a contradiction: pacino declared line
     and above_index together, and line interleaving IS the bits
     above the offset while above_index is the other end of the
     index. One field cannot disagree with itself.

     word granularity remains UNRESOLVED and reports why. The
     select would be inside the offset, where a position field does
     not reach, and sets_per_bank derived as sets/banks is also
     wrong there because every bank then holds every set. geometry
     leaves bank_resolved false and carries the reason.

     Applied to pacino:

```
       l2   512 KB, 8 ways, 64 B, 2 banks, line
            offset [5:0]  index [15:6]  tag [31:16]
            bank   [6:6]  set index [15:7]
       mem  1 GB, 1 way, 64 B, 8 banks, line
            bank   [8:6]  set index [29:9]
```

     WHAT NO TEST CAN SHOW. Both readings pass every functional
     check. Two addresses differing in bit 6 are in different banks
     under this ruling and in different sets under the previous
     one, and a correct cache serves both. The CLI-004
     corroboration, that the set index width agrees with
     sets_per_bank, passes under both, because taking N bits from
     either end of an index leaves N bits. It checks the arithmetic
     and not the position. A bank select in the wrong place is a
     PERFORMANCE defect and nothing in this project measures
     cycles. See Q-13.

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
     weighted. Nothing in the tool consumes it. See Q-08.

     UPHELD 2026-08-27. CLI-004 needed two arbiters and neither
     contradicts this. l2's two banks contending for one downstream
     link is contention the EMITTER created by giving them one
     downstream master, which is a structural choice and not a
     property of the configuration. l2's up_i against up_d is
     contention for the arrays, which this decision already
     classifies as pipeline scheduling rather than arbitration with
     a policy. WHERE THERE IS ONE PORT THERE IS NO ARBITRATION.

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

     The emitter takes the INSTANCE name for the output directory
     and for every module name, so two instances of one definition
     cannot collide. This is now load-bearing in shipped output.

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
# Emission

Decisions the emitter forced that constrain the configuration or
reverse an earlier entry. The 17 conventions that constrain only
generated TEXT are reported in prompts/CLI-004.md and belong in
D-40's file, which does not exist.

D-50 [J] 2026-08-27. ADDRESSING IS BYTE BASED. The offset field is
     a byte offset within the line.

     Every word count and word index in generated output derives
     from one named package parameter per node, AddrUnitBytes, so a
     reversal to word based addressing is one edit in one generated
     file. Nothing else reads it and every consumer calls the field
     accessor rather than the parameter.

D-53 [J] 2026-08-27. NO GENERATED MODULE HAS A PARAMETER. Every
     value is a package localparam.

     A module parameter can be overridden at instantiation, and
     every one of these values was DERIVED by the tool from the
     configuration. An overridable derived value is a second source
     that can disagree with the first, which is the S-12 defect
     class. Package localparams make them un-overridable by
     construction. This is D-37 carried into the output.

     The one exception is a testbench clock half period, which is
     a `parameter` precisely so it can be overridden at build time.
     It is simulation control, D-41, and it is on a testbench and
     never on the design.

D-54 [J] 2026-08-27. EMISSION IS DETERMINISTIC. Two runs of one
     configuration produce byte identical output. No timestamp,
     host name, user name, absolute path or git sha appears in any
     emitted file.

     A generated file names its source by SYSTEM NAME and by the
     BASE NAME of the configuration file, never by a path, because
     --config accepts an absolute path and echoing it would make
     the output depend on the user's spelling.

     Vars.mk is the one emitted file whose contents may vary with
     the command line, D-55, and one log block records the tool
     paths it was given. Both are asserted by name rather than
     exempted, so a third file drifting into the command line's
     reach fails the test.

D-55 [J] 2026-08-27. NO EMITTED MAKEFILE NAMES A TOOL DIRECTLY.
     Every invocation goes through a variable defined in Vars.mk,
     which the tool copies from planning/tools/Vars.mk to the root
     of the output tree.

     A generated Makefile that says `verilator` runs whatever is on
     PATH. That is how CLI-004's output was linted by 5.020 rather
     than the 5.048 in the tree, and how ten warnings were
     attributed to the emitted RTL that 5.048 does not produce.

     --vars names the master copy, default
     $CGEN_ROOT/planning/tools/Vars.mk. --tool VAR=PATH sets one
     tool path and is repeatable. Every tool the emitted build uses
     gets a variable even where the value resolves to the system
     copy.

     $CGEN_ROOT IS SPECIAL. A supplied path beginning with the
     current expansion of $CGEN_ROOT is written back as
     $(CGEN_ROOT)/... so the emitted tree stays portable. A path
     outside the tree is written verbatim and the file says the
     tree is then machine specific by the user's choice.

     WITH CGEN_ROOT UNSET THE DEFAULT CANNOT RESOLVE AND THAT IS AN
     ERROR, not a search. The tool reports it and writes nothing.
     The master Vars.mk already errors the same way, so the failure
     has one message and one place. R-11.

D-56 [J] 2026-08-27. A header under cli/inc may not share a name
     with a system header. cli/inc/features.h shadowed glibc's
     <features.h>, because -I./inc precedes the system include
     path, and every system header that includes it received the
     project copy. The build failed inside /usr/include/time.h.

---
# Standing conventions

Carried from earlier work. Not re-argued.

D-37 No derived value appears in the input. sets, tag width, the
     word offset field, the bank field and the refill beat count
     are computed by the tool.

     GENERALISED 2026-08-27 by D-49. A POSITION derivable from
     another field is itself a derived value. bank_select_position
     stated where the bank select sits when
     bank_interleave_granularity already determined it, so it was a
     derived value that had escaped this rule and could contradict
     its own source. The general form: if two fields can disagree
     and one determines the other, the determined one does not
     belong in the input.

D-38 No value plus units pair anywhere in the input. Integers, in
     bytes and bits. A rendered string may appear in output for a
     datasheet and is never read back.

D-39 Generated, not transcribed. The PLRU update and victim tables
     and the address field decomposition are built once by the tool
     and emitted to every consumer. Four hand maintained copies
     that happen to agree is not one source.

     This rule also governs the TOOL's own artifacts. The
     diagnostic code list, D-51, the field consumption record,
     D-52, the tool variable table, D-55, and the feature table are
     each generated from one declaration rather than maintained by
     hand. Every property this project maintained as a list went
     stale; every one it generated did not.

D-40 [J] Generator conventions live in an xxx_decisions.md file in
     the repo, not in the JSON. This covers the tree PLRU bit
     encoding, the invalid way tie break direction, signal and port
     naming beyond clock and reset, and the non synthesizable reset
     clear loop. Any encoding works provided one generator emits
     all consumers from it.

     THE FILE STILL DOES NOT EXIST. CLI-004 decided 17 conventions
     and CLI-005 added more, all reported in task files and nowhere
     a reader of the emitted RTL would look. Not needed while cgen
     runs in this tree.

D-41 [J] Simulation control is not configuration. Clock period,
     cycle limits, reset cycle counts, waveform enables, test
     selection, output directories, module prefixes and macro names
     are tool requirements or CLI flags. TOOL PATHS are the same,
     D-55.

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

     $schema is a different field and carries the JSON SCHEMA
     DRAFT, which is 7 in all five, matching the validator the tool
     links. See Current state.

D-48 [J] Tool decisions, from planning/tools/tool_decisions.md and
     restated here because they bound everything above: C++20 not
     C++23, namespace cgen, one class per file, Make never CMake,
     nlohmann json, pboettch json-schema-validator, Boost
     ProgramOptions, gtest, --cmd={check,emit}, no positionals,
     --output defaults to ./output, --eoe off by default.

---
# Open questions

Q-05 A memory smaller than the address space decodes rather than
     compares. Whether "no tag compare" needs a field, or follows
     from something already present, was not worked through.

Q-06 The elaborated output schema is untouched and stale. It still
     carries the old artifact kind enum and has no home for fields
     added since. NOT ON ANY PATH: the emitter holds derived values
     in memory and emits them into the generated packages, so
     nothing waits on this. What a file would buy is provenance and
     a regeneration diff.

Q-07 Version compatibility across files. Each file carries its own
     schema_version and the tool has to reject a mismatched set. It
     does not yet; each file is validated against its own schema
     and a mismatch shows up as a schema_version const violation
     rather than as a set-level message.

Q-08 Nothing consumes arbitration. STRENGTHENED 2026-08-27: pacino
     declares arbitration on NONE of its eleven interfaces, so the
     field does not even appear on the unconsumed report. Every
     interface in the example carries exactly one port, and D-27
     rules that one port is no arbitration. The field becomes live
     when a configuration puts several ports on one interface.

Q-10 Is emitted RTL regenerated in place or hand-owned after the
     first emission? The de facto answer today is regenerated: the
     emitter overwrites what it writes and deletes nothing. More
     urgent than when it was raised, because the output tree now
     holds RTL, testbenches, a build, four logs and memory images.
     Still free to rule, because adaptation has not started.

Q-11 DOWNGRADED 2026-08-27. Is testcases/1mb_l1 a pacino template
     or a generic reference? It was recorded as blocking the
     emitter and it did not. CLAUDE.md already states the target is
     SystemVerilog-2023 synthesizable RTL, so CLI-004 wrote fresh
     and the question never arose. Two tasks report the directory
     is not in this tree.

     What remains is the Method note: the module walk was never
     done, so the field set is not known to be complete.

Q-12 The check enumeration and the diagnostic code list disagree,
     three ways. T-7 is enumerated, is now ruled by Q-04, and emits
     no code, so port occupancy is unchecked. topology.addressing
     is a code with no T number. T-8.field_sum is DEAD BY
     ARITHMETIC: it tests whether offset + index + tag equals
     pa_bits one line after tag is assigned the difference, so it
     is an identity, and it is also the check that would have to
     catch the S-12 class and cannot. Six further codes are
     unreachable because the schema rejects first, which is D-46
     appearing in geometry.

     The resolution is to make the enumeration and the code list
     one artifact, D-39 applied to this document.

Q-13 NOTHING MEASURES CYCLES, and two findings need it.

     A bank select in the wrong position is a performance defect
     that no functional test can see, D-49. read_latency_cycles is
     declared on every cache and honoured by none of them, Q-14,
     and cannot be verified without a cycle count. The functional
     model will not see either.

Q-14 read_latency_cycles IS IGNORED ON EVERY CACHE. The memory
     model honours it; no cache does. A configuration asking for a
     twelve cycle L2 gets whatever the emitted pipeline happens to
     be and nothing says so.

     This is worse than the other inert fields because it is a
     number a user would reasonably believe, and it is among the
     most likely fields to be edited during exploration.
     tag_compare_stage is in the same position and would decide
     part of it.

     Interim: the emitted control header states the declared
     latency is not honoured, as it already does for mshrs. The fix
     is a configurable read pipeline depth with tag_compare_stage
     selecting where the compare lands.

Q-15 Should planning/schema/examples be validated by the suite?
     Two files there carry bank_select_position and are schema
     invalid as of D-49. Nothing reads them and no test covers that
     directory, which is why it went unnoticed. An invalid example
     is worse than no example.

---
# Answered

Q-01, Q-02, Q-03 answered before this document was written.

Q-04 CLOSED 2026-08-27. RULED: ONE PORT, ONE EDGE. A slave port may
     not host more than one edge. A shared bus is expressed by
     several ports on one interface, which is what the interface
     level exists for.

     T-7 emits no diagnostic, so the ruling is not enforced. Q-12.

Q-09 CLOSED 2026-08-27 by D-49. bank_interleave_granularity alone
     determines the bank position, and bank_select_position is
     deleted because it could contradict its own source. word
     granularity remains unresolved and reports why.

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

R-09 [J] 2026-08-27. bank_select_position as a field alongside
     bank_interleave_granularity. Two fields describing one fact,
     able to disagree, and pacino carried the disagreement. Same
     shape as R-06. See D-49 and D-37.

R-10 [J] 2026-08-27. Module parameters with defaults for derived
     geometry, the conventional RTL idiom. An overridable derived
     value is a second source. See D-53.

R-11 [J] 2026-08-27. An upward directory search for Vars.mk when
     CGEN_ROOT is unset. A second way to locate a build input is
     how a tool silently picks up the wrong file. Unset is an
     error. See D-55.

R-12 [J] 2026-08-27. Allowing the module-header form of a wildcard
     import to avoid the $unit collision. verilog_style.md rejects
     that form and the rule stands. The emitter's answer, prefixing
     every package member with the node name, is what is expected.
     The style file should record WHY the placement rule exists,
     because a hand written module has the same collision and no
     warning.

---
# Current state

Schemas, all validating, with accept and reject cases for every
conditional. Versions are in $id and schema_version, D-47.

```
  system.schema.json     0.10.0   unchanged
  ports.schema.json      0.11.0   unchanged
  links.schema.json      0.12.0   arbitration removed, D-27
  caches.schema.json     0.13.0   node/interface/port, D-42;
                                  bank_select_position removed,
                                  D-49. Not bumped: nothing
                                  consumes the version.
  topology.schema.json   0.14.0   edge names node.interface.port
```

The tool loads, resolves, checks, derives and EMITS. cgen
--cmd=check on testcases/pacino produces no diagnostics.
cgen --cmd=emit produces 83 files: RTL, testbenches, a Make build,
Vars.mk and four logs.

```
  cli suite       86 of 86
  emitted suite   76 of 76, through the generated Makefiles
  emitted tree    0 lint errors, 0 lint warnings, Verilator 5.048
```

pacino's l1i was 32KB over four ways, which put 8192 bytes in a way
against a 4096 byte page and left index bit 12 above the page
offset, so one physical line could land in two sets. A VIPT way
must not exceed a page, so at a 4KB page a way caps at 4KB and 32KB
needs eight of them. l1i is now eight way, matching l1d, and both
L1s sit exactly at the budget with the index ending at bit 11.

THE DRAFT 7 QUESTION IS CLOSED. pboettch json-schema-validator
implements draft 7 and DISCARDS $schema rather than erroring on it,
so a wrong declaration is silent at run time and no tool diagnostic
can report one. The five schemas were searched for the two
constructs on which draft 7 and 2020-12 differ in a way that could
matter here: unevaluatedProperties appears nowhere, and all 24 uses
of $ref are the sole key of their object. The two drafts therefore
agree on every construct these files use, and the tool was
validating them correctly throughout. The declarations now read
draft 7 and SchemaFiles.DeclareDraft07 enumerates planning/schema
and asserts it. Python jsonschema is not a validator of record.

WHAT THE EMITTED CACHE DOES NOT DO. Every item is a field the
configuration declares and no stage reads:

```
  the cache is BLOCKING. mshrs reaches a comment and sizes
    nothing. mshr_targets, victim_buffer_entries and
    fill_buffer_entries are inert.
  read_latency_cycles is ignored on every cache, Q-14
  inclusion is not enforced; the probe path is tied off
  no maintenance port is emitted at all
  way_access and data_array_organization are inert
  TL-C's B, C and E channels are emitted and tied off
```

74 of pacino's 287 configuration leaves are read by no stage. The
generated list is at output/logs/unconsumed.log.


