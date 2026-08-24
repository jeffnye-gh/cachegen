<!-- SPDX-License-Identifier: Apache-2.0                       -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com -->
=============================================================
# Task Header 
=============================================================
:: HEADER:START ::

| Field        | Value                   | Notes                    |
|--------------|-------------------------|--------------------------|
| Task ID      | INFRA-001               |                          |
| Date         | 2026.08.24              |                          |
| Module       | n/a                     | assessment task          |
| Run time     | 13m.56s                 |                          |
| Ctx %        | 33%                     |                          |
| Model        | claude-opus-5[1m]       | effort not exposed       |
| Resume sha   | <sha>                   |                          |
| IA session   | 001                     |                          |

Task:   [ ] experiment  [ ] implementation  [ ] debug
        [ ] cleanup     [ ] testbench       [ ] verification
Mode:   [ ] automated   [ ] manual          [ ] interactive
Status: [ ] in-progress [ ] complete        [ ] abandoned

# Task Overview

This task performs an assessment of the state of the repo and
associated design files. 

The question to answer is can the design in examples/1mb_l1/rtl
and examples/1mb_l1/model be represented by the json schema found
in planning/schema/cgen_config.schema.json

This assessment looks for inconsistency, accuracy, completeness,
feasibility.

The output is a list of corrections necessary to accomplish this
task.

:: HEADER:END ::

=============================================================
:: DISCUSSION:START ::
=============================================================

# Results Discussion 

## Claude.code Console Output

INFRA-001 executed. Results Capture written to prompts/INFRA-001.md between
  the RESULTS markers; validation re-passes; no context file modified.

  Answer to the task's question: the RTL can be represented by
  cgen_config.schema.json for geometry, policy and replacement. It cannot for
  byte-enable width, next-level model, hit timing, per-array storage kind, reset
  polarity, or the probes artifact. The C++ model cannot be represented as it
  stands — different vocabulary, missing emission/maintenance/miss_handling
  groups, four fields GLOSSARY puts out of scope, and its emitted JSON shares no
  structure with the elaborated schema.

  64 findings, each with file:line — 20 planning (P), 19 schema-vs-RTL (S), 25
  schema-vs-model (M).

  Two are silent-wrong-answer defects rather than gaps:

  - M-03 — the exact round-trip bug GLOSSARY section 1 was written to justify is
    live: options.cpp:321 reads json["mm_capacity_value"] into mm_capacity. 8MB
    reloads as 8. GLOSSARY describes it almost verbatim as a predecessor
    defect; it was never fixed.
  - S-12 / M-24 — tag width has three disagreeing sources. RTL hard-wires 14
    bits (cache.v:51); the model derives 5 from its default 8MB mm_capacity
    (gen.cpp:347,363); the schema derives it from pa_bits. They agree only at
    mm_capacity = 4GB.

  Also worth your attention before the next task:

  - mdl always exits 1 — mdl_tests.cpp:171 and the basicWrEvictTest stub both do
    an unconditional ++errs, and mdl_run.cpp:55-68 scores every disabled test
    as 1000 errors. Skipping is counted as failing.
  - The RTL will not pass Verilator — continuous assign to a reg output
    (bitrf.v:35, lrurf.v:35) is illegal, and cache.v multiply-drives mm_write_d
    (lines 156 and 400). probes.v also elaborates against a hierarchy that
    exists in neither top.v nor dut.v.
  - CacheModel::mm is uninitialised (mdl.h:112) and deleted on the constructor
    path (mdl.cpp:33,244).
  - The example is Verilog-2001 .v, not the SystemVerilog-2023 the pacino
    profile describes. Deciding whether it is a template or a reference (DW-8)
    gates a lot of downstream work.

  Three notes on how I ran it:

  - R2 says compare against "the input configuration json,
    planning/schema/cgen_config.schema.json" — that path is a schema, not an
    instance. I read it as "can the schema express this design" and did not open
    misc/example_pacino_icache.json, which is not in the manifest. Recorded as
    assumption A-1.
  - I ran two directory listings (names only, no reads outside the manifest) to
    check PROJECT_CORE's path claims, rather than assert missing files
    unverified. Recorded as A-2; if you read the scope rule more strictly, P-06
    and P-12 become unverified and nothing else changes.
  - The manifest's 39 files total 14,727 lines, of which 7,401 (50.3%) are stock
    jsoncpp 1.7.4. I read them as required; they produced no finding. DW-7
    recommends dropping them from future manifests.

  planning/PROJECT_STATUS.md and templates/TASK_TEMPLATE.md exist but are not in
  the manifest, so I left them closed — P-02 reports the STATUS vocabulary
  mismatch it can see in the file headers and stops short of claiming what
  PROJECT_STATUS.md says.

## Results Assessment

IA points out a flaw in my task creationg. "the input configuration json" when
it should have been focused on the schema. The IA made the correction
adjustment. 

## Follow-on Actions
- [ ] Additional fields will be necessary in the input json, 
- or Nothing required

:: DISCUSSION:END ::

=============================================================
:: PROMPT:START ::
=============================================================

## Task ID
INFRA-001

## Context Loaded
@planning/ANTIPATTERNS.md
@planning/CLOSED_TECH_DEBT.md
@planning/GLOSSARY.md
@planning/PROJECT_CORE.md
@planning/schema/cgen_config.schema.json
@planning/schema/cgen_elaborated_config.schema.json

@planning/tools/tool_decisions.md
@planning/tools/verilator_decisions.md
@planning/tools/verilog_style.md

@examples/1mb_l1/rtl/src/bitrf.v
@examples/1mb_l1/rtl/src/cache.v
@examples/1mb_l1/rtl/src/compare.v
@examples/1mb_l1/rtl/src/dsram.v
@examples/1mb_l1/rtl/src/dut.v
@examples/1mb_l1/rtl/src/fsm.v
@examples/1mb_l1/rtl/src/lrurf.v
@examples/1mb_l1/rtl/src/mainmemory.v
@examples/1mb_l1/rtl/src/merge.v
@examples/1mb_l1/rtl/src/parts.v
@examples/1mb_l1/rtl/src/probes.v
@examples/1mb_l1/rtl/src/sram.v
@examples/1mb_l1/rtl/src/top.v

@examples/1mb_l1/model/inc/addresspacket.h
@examples/1mb_l1/model/inc/gen.h
@examples/1mb_l1/model/inc/json/json.h
@examples/1mb_l1/model/inc/mdl.h
@examples/1mb_l1/model/inc/msg.h
@examples/1mb_l1/model/inc/options.h
@examples/1mb_l1/model/inc/ram.h
@examples/1mb_l1/model/inc/utils.h

@examples/1mb_l1/model/src/gen.cpp
@examples/1mb_l1/model/src/gen_main.cpp
@examples/1mb_l1/model/src/jsoncpp.cpp
@examples/1mb_l1/model/src/mdl.cpp
@examples/1mb_l1/model/src/mdl_main.cpp
@examples/1mb_l1/model/src/mdl_run.cpp
@examples/1mb_l1/model/src/mdl_tests.cpp
@examples/1mb_l1/model/src/options.cpp
@examples/1mb_l1/model/src/ram.cpp
@examples/1mb_l1/model/src/utils.cpp

## Context Comments
All files in the context are read only.

## Hypothesis

Test whether the support and planning files are consistent.

Test whether the json configuration schema is complete.

There are two components to this task. Analysis of the RTL output
and analysis of the C++ model. The fpga design is not part of this
effort.



## Background

This is a first run in a previous repo that is being migrated from
manual generation to an automated flow using LLMs.

## Binding Previous Decisions

None.

## Specific Requirements

R1 -- assess the planning documents and enumerate your assessment of the
      necessary corrections to improve usage, clarity and consistency.

R2 -- assess the design found in examples/1mb_l1/rtl/src as compared to
      the input configuration json, planning/schema/cgen_config.schema.json.
      Enumerate issues in the schema, missing fields, extra fields, 
      ambiguity.

R3 -- perform the similar analysis for the design found in 
      examples/1mb_l1/model.

## Constraints

Do not modify any files in the context.

You may modify this task file for results only between the markers in the
results section.

Report and halt when you encounter any inconsistent or ambigous 
instructions.

## Deliverables

Task's expected deliverables 

- Results Capture filled in below, in this file: prompts/INFRA-001.md

Fill in every section. Test Matrix may be omitted: this task adds no
test cases. 

:: PROMPT:END ::

=============================================================
:: RESULTS:START ::
=============================================================


## Summary

Read-only assessment. All 39 manifest files were read completely,
including the two jsoncpp amalgamation files.

Answer to the Task Overview question: the RTL in examples/1mb_l1/rtl
CAN be represented by cgen_config.schema.json for geometry, policy and
replacement. It CANNOT be fully represented for byte-enable width,
next-level model, hit timing, per-array storage kind, reset polarity,
or the probes artifact. The C++ model in examples/1mb_l1/model CANNOT
be represented as it stands: its option set uses a different
vocabulary, omits the entire emission and maintenance groups, carries
four fields a planning document puts out of scope, and its emitted
JSON shares no structure with cgen_elaborated_config.schema.json.

Counts: 20 planning findings (P-01..P-20), 19 schema-vs-RTL findings
(S-01..S-19), 25 schema-vs-model findings (M-01..M-25). Every finding
cites a file and line. No file in the context was modified.

Two findings are the highest value in the set:

- M-03. The exact round-trip defect GLOSSARY section 1 was written to
  justify is live in the code: options.cpp:321 reads
  json["mm_capacity_value"] into mm_capacity. 8MB reloads as 8.
- S-12 / M-24. Tag width has three disagreeing sources. The RTL is
  hard-wired to 14 bits, the model derives 5 bits from its default
  main memory capacity, and the schema derives it from pa_bits. The
  three only agree when mm_capacity is 4GB.

## Test Matrix (testbench sessions only, omit otherwise)

Omitted. This task adds no test cases.

## What was delivered

### R1 -- Planning document assessment

P-01  planning/ANTIPATTERNS.md and planning/CLOSED_TECH_DEBT.md
      contain only the SPDX block and the header block. No content.
      PROJECT_CORE.md:273-274 describes them as "known prompt failure
      modes" and "closure entries, historical". Neither has an entry.
      Fix: populate, or mark NOT STARTED and drop from the context of
      tasks that do not need them. Loading two empty files in every
      manifest costs context and signals content that is not there.

P-02  Status vocabulary and status location are both inconsistent.
      PROJECT_CORE.md:261 declares the value set as DRAFT, CLOSED,
      NOT STARTED, DEPRECATED. Six of the seven planning files in
      this context carry STATUS: WORKING, which is not in that set
      (GLOSSARY.md:8, ANTIPATTERNS.md:8, CLOSED_TECH_DEBT.md:8,
      tool_decisions.md:8, verilator_decisions.md:8,
      verilog_style.md:8). PROJECT_CORE.md:8 carries DRAFT.
      Separately, PROJECT_CORE.md:259-261 says status "is recorded in
      exactly one place: the PROJECT_STATUS Module Status table", yet
      every file header carries a STATUS field. That is a second
      place. GLOSSARY.md:15 adds a third by opening with "Draft."
      Fix: pick one. Either delete STATUS from the file headers, or
      delete the exactly-one-place rule. Add WORKING to the set or
      retire it.

P-03  PROJECT_CORE.md:283-284 inventory names schema files that do
      not exist:
        listed  planning/schema/cache_config.schema.json
        actual  planning/schema/cgen_config.schema.json
        listed  planning/schema/output_json.md
        actual  planning/schema/cgen_elaborated_config.schema.json
      The second is also a format change, .md to .json.

P-04  GLOSSARY.md:15-16 names a third pair of filenames:
      "cache_config.schema.json (input) and
      cache_elaborated.schema.json (output)". Neither exists. Three
      documents now use three names for two files.

P-05  Both schemas point at a document that does not exist.
      cgen_config.schema.json:5 and :106 say "See VOCABULARY.md".
      planning/VOCABULARY.md is absent; the content is in
      GLOSSARY.md. Two references.

P-06  PROJECT_CORE.md:234-252 repository layout does not match the
      tree. Verified by listing only, no file reads:
        listed and absent    gui/, cli/, output/
        listed and empty     docs/, planning/arch/
        present, not listed  examples/, misc/, src/, versions/,
                             planning/tools/, .claude/
      planning/tools/ holds three files that are in this task's own
      context manifest and appear nowhere in the inventory.

P-07  Prompt authorship names three authors with no precedence rule,
      all governing the same artifact:
        PROJECT_CORE.md:66-67  "The IA writes the experiment prompts."
        PROJECT_CORE.md:106    User "Supplies session prompts to the
                               IA for execution"
        PROJECT_CORE.md:72-73  "At times Claude.ai is used to create
                               planning files and emitting task files."
      Fix: state who owns a task file by default and who may override.

P-08  The abbreviation IA has three expansions across two documents,
      same scope:
        CLAUDE.md Terms         "interactive assistant, Claude Code"
        PROJECT_CORE.md:26      "Implementation Assistant"
        PROJECT_CORE.md:61      "Interactive Assistant"
      Fix: one expansion, stated once, referenced elsewhere.

P-09  PROJECT_CORE.md cites CLAUDE.md content that CLAUDE.md does not
      contain:
        :136-138  "CLAUDE.md carries the enforcing copy of this rule,
                  in Fixed Constants." CLAUDE.md has no Fixed
                  Constants section.
        :206-208  "CLAUDE.md requires the IA to run each in-scope
                  module's complete suite and blocks completion on any
                  non-waived failure." CLAUDE.md states no such
                  requirement.
      The second matters: it makes the suite-gating waiver paragraph
      unenforceable, so a prompt written to satisfy it is satisfying
      nothing.

P-10  PROJECT_CORE.md:121 and :123 contradict each other on the single
      question of who edits a planning document.
        :121  "PLANNING DOCUMENTS ARE ONLY MODIFIED BY THE IA under
              restricted permission."
        :123  "The IA reports what a document should say; the user
              drafts and applies the change"
      As written, :121 makes the IA the sole modifier and :123 makes
      the user the modifier. CLAUDE.md agrees with :123. Fix :121 to
      read "PLANNING DOCUMENTS ARE NEVER MODIFIED BY THE IA EXCEPT
      UNDER AN EXPLICIT WAIVER."

P-11  The task ID prefix list at PROJECT_CORE.md:80-86 omits BP, while
      BP-NNN appears at :133-134 ("BP-098 through BP-105") and at :237
      ("BP-NNN.md / INFRA-NNN.md"). Either add BP with its meaning or
      convert the two citations.

P-12  PROJECT_CORE.md:295 gives the status line path as
      ./claude/statusline.sh. The file is .claude/statusline.sh.
      Leading dot missing.

P-13  Header field hygiene. UPDATED is "n/a" in tool_decisions.md:9
      and verilog_style.md:9. verilator_decisions.md:9 uses
      2026.08.24; every other file uses 2026-08-24. The task file
      header uses 2026.08.24 as well. Fix: one date format, and
      require UPDATED.

P-14  planning/tools/ carries predecessor-project specifics that do
      not apply to cachegen and will mislead a generator prompt:
        verilator_decisions.md:29-31  "import bp_pkg::*;"
        verilator_decisions.md:34-35  "NUM_PRED_SLOTS"
      cachegen names its package <name>_pkg.sv
      (cgen_config.schema.json:28). NUM_PRED_SLOTS is a branch
      predictor parameter. Fix: generalise both to the cachegen
      naming, or state that these are illustrative.

P-15  planning/tools/tool_decisions.md records no decisions. It is a
      bare list of eight tool names under "# Tools" plus a dangling
      empty "# " heading at line 26. Verible, Clang-Format and
      Checkmake are named with no invocation, version, or config.
      Contrast verilator_decisions.md, which is what a decisions file
      should look like.

P-16  Two of the four GLOSSARY open questions are already answered by
      other documents in this same context:
        Q2 (is the C++ model generated or parameterized) is answered
        "generated" by PROJECT_CORE.md:50 and by the "model" value in
        cgen_config.schema.json:238 emit enum.
        Q3 (does banks belong in geometry or interfaces) is answered
        "geometry" by cgen_config.schema.json:71.
      Fix: close both, or state that the schema is provisional on
      them. An open question that the schema has already decided is a
      trap for the next prompt author.

P-17  GLOSSARY.md:37-52 states the input JSON lives in a fenced
      "```json cachegen" block inside the planning document that
      justifies it, and that this "keeps one file, reviewed once,
      under PA ownership". No planning document names which document
      that is, and the repository layout has no slot for it. The
      schema's $id and description instead describe a standalone
      file. Fix: name the host document, or demote the fenced-block
      path to the secondary form.

P-18  Three different output layouts are named in three places:
        PROJECT_CORE.md:247-250   output/<design>/rtl, /tb
        cgen_config.schema.json:242  rtl/, tb/, tests/
        examples/1mb_l1 on disk   rtl/src, rtl/data, rtl/golden,
                                  model/inc, model/src
      Fix: one layout, stated in the schema, with PROJECT_CORE
      pointing at it.

P-19  No planning document states the source language level or file
      extension for emitted RTL. CLAUDE.md says SystemVerilog-2023
      and cgen_config.schema.json:229 says the pacino profile applies
      "always_comb over chained assign". Every file in the example is
      .v Verilog-2001 using always @*, reg and casez. A planning
      statement of ".sv, SystemVerilog-2023, no .v" would have caught
      this. See S-18.

P-20  Editorial pass needed on PROJECT_CORE.md. Typographic errors in
      normative text: "drivend" (:101), "occiasionally" (:107),
      "priviledges" (:111,:114), "SystemVerlog" (:33), "testins"
      (:48), "elborated" (:45), "There templates" (:69),
      "explicity" is not present but "Cgen" (:37) vs "cgen"
      elsewhere is. These are cosmetic individually; collectively
      they reduce confidence in a document that is cited as the
      authority for the flow.

### R2 -- examples/1mb_l1/rtl/src vs cgen_config.schema.json

The design as read from the RTL:

  capacity      1MB      cache.v:3, and 2^13 sets * 4 ways * 32B
  line_bytes    32       cache.v:50 CACHELINE_BITS = 32*8
  associativity 4        cache.v:53 WAYS = 4
  sets          8192     cache.v:52 IDX_BITS = 13
  tag           14 bits  cache.v:51, a[31:18] at cache.v:108
  index         13 bits  a[17:5]  at cache.v:109
  word offset   3 bits   a[4:2]   at cache.v:110
  byte          2 bits   a[1:0]   implicit
  banks         1        four dsram instances, one per way,
                         cache.v:491-529
  indexing      PIPT     no virtual address anywhere
  read_miss     allocate fsm.v:208-211 RD_ALLOC
  write_miss    allocate fsm.v:224-227 WR_ALLOC
  write_hit     write_back  fsm.v:186-195 sets dirty, defers
  replacement   tree_plru   lrurf.v 3 bits for 4 ways
  mshrs         0        blocking FSM, fsm.v single outstanding miss
  victim buf    0        none present
  fill buf      0        none present
  core read     32 bits  cache.v:23 rd
  core write    32 bits  cache.v:33 wd
  refill        256 bits cache.v:41 mm_readdata, one beat

S-01  NEGATIVE RESULT, recorded as a deliverable. Every field above
      is expressible in cgen_config.schema.json with a legal value.
      capacity_bytes 1048576 is inside [256, 8388608]; line_bytes 32,
      associativity 4, refill_width_bits 256, core widths 32 are all
      in their enums; pa_bits 32 is at the minimum. GEO-1..4,
      ADDR-1..2, IF-1..2, MSHR-1 and RPL-1 all pass for this
      geometry. The geometry half of the schema is sound.

S-02  The offset field cannot express the split the design uses, and
      its units are unstated. cgen_elaborated_config.schema.json:69
      emits one offset field. The RTL uses two: a 3-bit word offset
      at a[4:2] driving the read and write muxes (cache.v:201-221,
      cache.v:299-319) and a 2-bit byte position that only the byte
      enables see. The C++ model uses the same split
      (options.h:131-138, gen.cpp:334). Nothing in either schema says
      whether "offset" means bytes within the line or words within
      the line. Fix: state the unit explicitly, and add a
      sub-granule field (word offset bits, or access_bytes) so the
      emitter does not have to re-derive it.

S-03  No byte-enable width. The RTL's be is 4 bits, one per byte of
      the 32-bit word (cache.v:30). That width fans out to 32 bits
      across a 256-bit line in dsram.v:14 and drives merge.v:9. There
      is no input field for write granularity and no derived be_bits
      in the elaborated schema. core_write_width_bits implies it but
      never states it. Fix: add derived.byte_enable_bits, or a
      write_granularity_bytes input.

S-04  critical_word_first conflates two different properties.
      cgen_config.schema.json:134-137 defines it as forwarding the
      requested beat as it arrives, and GLOSSARY IF-3 makes it
      not_applicable when refill_beats is 1. This design has
      refill_beats == 1, and it still forwards the critical word from
      the in-flight fill bus rather than waiting for the array write
      (fsm.v:29-31 comment, cache.v:310-321 frd path selected by
      fsm_cc_rd_fill). Those are two separate decisions: beat
      ordering on the fill bus, and fill-bus bypass to the core. The
      schema has one field for both, and IF-3 would mark this design
      not_applicable for a feature it demonstrably implements. Fix:
      split into refill_beat_order and fill_bypass_to_core.

S-05  No description of the next level. interfaces.next_level_name
      (cgen_config.schema.json:193) is a string marked
      "Informational". The example emits mainmemory.v with ENTRIES,
      READ_LAT and WRITE_TPUT parameters (mainmemory.v:10-14), and
      top.v:266-269 supplies all three. If the tool emits tb and
      makefile, it must emit or bind a next-level model, and no field
      describes its size, read latency, or write throughput. Fix: add
      an interfaces.next_level object with capacity_bytes,
      read_latency_cycles, write_throughput_cycles, and a model kind.

S-06  No hit timing fields, and the RTL's timing parameters are dead.
      cache.v:18-19 declares READ_HIT_LAT and WRITE_HIT_TPUT.
      READ_HIT_LAT is passed into fsm (cache.v:371) and fsm.v never
      reads it. WRITE_HIT_TPUT is never read anywhere in cache.v.
      top.v:17-18 sets both. So the design states a timing contract
      that the RTL does not implement and the schema cannot express.
      The golden capture depends on that contract. Fix: add a timing
      group to the input schema, or delete the parameters.

S-07  sram_model is a single enum for arrays that need different
      models. cgen_config.schema.json:246-250 offers bw_ram,
      inferred, macro, once. The example needs at least three
      distinct storage models simultaneously:
        tag array     sram.v      no byte enables, no reset,
                                  address-sensitive read
        data array    dsram.v     32 byte enables, registered read
                                  qualified by read_q
        status bits   bitrf.v,    flop register file with a
                      lrurf.v     synchronous clear-all
      GLOSSARY.md:206-208 makes the third one load-bearing:
      invalidate_all is "Cheap only because valid bits are flops
      rather than SRAM". So storage kind for status bits is a
      decision, and there is no field for it. Fix: make sram_model an
      object keyed by array (tag, data, status), or add
      status_storage.

S-08  The probes artifact has no home in either schema.
      examples/1mb_l1/rtl/src/probes.v is 238 lines of real emitted
      output. emission.emit (cgen_config.schema.json:238) and
      manifest[].kind (cgen_elaborated_config.schema.json:155) share
      an eight-value enum with no probes entry, so the file can
      neither be requested nor recorded. Fix: add "probes" to both
      enums, which are duplicated and should be a single $ref.

S-09  emit cannot distinguish preload images from golden images. The
      flow needs two sets and keeps them in two directories:
      mdl_tests.cpp reads ../rtl/data/ for preload (lines 15-35) and
      ../rtl/golden/ for expect (lines 116-146). The schema has one
      "vectors" value plus one verification.golden_vectors boolean
      (cgen_config.schema.json:267-270). GLOSSARY EMIT-2 describes
      vectors only as golden data. Fix: split into "preload" and
      "golden", or make vectors an object with both flags.

S-10  TEST-1 is under-specified. GLOSSARY.md:136 checks directed
      tests against cache_type only. Four of the thirteen enum values
      can be inapplicable for reasons other than cache_type, and the
      schema would accept all four for this design:
        mshr_merge, mshr_full    meaningless when mshrs is 0
        victim_hit               meaningless when
                                 victim_buffer_entries is 0
        refill_beat_order        meaningless when refill_beats is 1
      All four conditions hold for this design. Fix: extend TEST-1 to
      test against miss_handling and the derived refill_beats, and
      record each as not_applicable rather than rejecting.

S-11  maintenance defaults claim capability the RTL does not have.
      invalidate_all defaults to true (cgen_config.schema.json:208).
      The example declares the interface and does not implement it:
      cache.v:93-96 declares pe_flush, pe_flush_all, pe_invalidate,
      pe_invalidate_all and never drives or reads any of them.
      fsm.v:63-82 documents Flush, Invalidate, Flush all and
      Invalidate all in the header comment and has no corresponding
      states. probes.v:33 decodes an INVAL_ALL state that fsm.v never
      enters. Fix: default invalidate_all to false, and make the tool
      verify emission rather than trusting the default.

S-12  Three disagreeing sources for tag width. This is the single
      most consequential finding in R2.
        RTL      TAG_BITS = 14, hard-wired, cache.v:51
        schema   tag_bits = pa_bits - index_bits - offset_bits,
                 GLOSSARY ADDR-1
        model    l1_tagBits = mm_address_bits - l1_setBits
                 - l1_offBits - 2, gen.cpp:363-364, where
                 mm_address_bits = log2(mm_capacity), gen.cpp:347
      With the model's default mm_capacity of 8388608 (options.cpp:
      183), mm_address_bits is 23 and l1_tagBits is 5. The RTL needs
      14, which requires mm_address_bits 32, that is mm_capacity 4GB.
      So the shipped model default and the shipped RTL disagree by
      nine bits of tag. The schema's pa_bits is the correct fix and
      this is the evidence for why the field must exist and must be
      an input, not a derivation from main memory size.

S-13  No reset polarity field, and profile implies one silently.
      cgen_config.schema.json:229 says profile pacino applies
      "clk/rstn". CLAUDE.md states active low reset is rstn. Every
      module in the example uses an active-high reset named reset
      (cache.v:45, bitrf.v:21, lrurf.v:21, fsm.v:123). So the example
      does not satisfy the profile the schema names, and nothing in
      the elaborated output would record the discrepancy. Fix: record
      reset polarity and name in derived, so the manifest states what
      was actually emitted.

S-14  additionalProperties: false at every level with no extension
      point (cgen_config.schema.json:17, 55, 84, 116, 144, 179, 204,
      226, 265). Any project knob such as the READ_HIT_LAT of S-06
      has nowhere to go and would be rejected outright. Fix: allow a
      single reserved object, for example "x", or a patternProperties
      "^x_" escape at the top level.

S-15  output_dir subdirectory convention. See P-18. The schema's own
      statement (cgen_config.schema.json:242) is one of three
      competing layouts and does not match the example on disk.

S-16  verification.assertions is one boolean covering four unrelated
      assertions (cgen_config.schema.json:295). For this design two
      of the four are inapplicable: "MSHR index in range" is
      meaningless with mshrs 0, and "no fill into a valid way without
      eviction" needs restating for the invalid-way-preferred path.
      None of the four exists in the example RTL. The one that
      matters most here is one-hot way hit, and it is not guaranteed
      by construction: fill_way_sel (cache.v:262-269) ORs way_hit_d
      with fill_or_victim_way_d, so a fill concurrent with a hit in
      another way sets two bits, and bitrf.v:38-44 then writes only
      the first matching way. Fix: make assertions a list, and mark
      inapplicable members not_applicable in checks[].

S-17  RTL defects found while performing this assessment. Recorded
      because FIX WHAT YOU FIND applies and because each one shapes
      what a generator must emit. This task has no write permission
      for these files, so they are reported, not repaired.

      a. compare.v:3 declares parameter WIDTH = 14 and every port is
         hard-coded [14-1:0] with the parameterized forms commented
         out at lines 15-18 and 26. cache.v:341 instantiates it with
         .WIDTH(TAG_BITS). The parameter has no effect. This is the
         direct opposite of what emission.verification_params
         promises (cgen_config.schema.json:255-258).

      b. compare.v:95 assigns 2'bx to lru_selected_way_d, which is
         declared 4 bits at line 12. Width mismatch in the default
         arm.

      c. cache.v drives mm_write_d from two sources. Line 156 is
         "assign mm_write_d = fsm_mm_write_d;" and line 400 connects
         the fsm instance output port to the same net with
         ".fsm_mm_write_d(mm_write_d)". The wire fsm_mm_write_d
         declared at line 76 is therefore never driven. The net
         survives only because an undriven wire resolves to z and
         loses to the instance driver. Verilator will flag the
         multiple driver.

      d. top.v:258 connects .mm_ready(mm_cc_ready) and mm_cc_ready is
         never declared in top.v. Implicit net.

      e. dut.v:56 has the same undeclared mm_cc_ready, and dut.v:58-59
         reference reset and clk which are neither ports nor
         declarations. dut.v is dead code, commented out of top.v at
         lines 214-230.

      f. probes.v hierarchical paths resolve against neither top.v nor
         dut.v. top.v instantiates the cache as dut0 and mainmemory
         as its sibling mm0, so probes.v:181-189 "top.dut0.mm0.ram"
         does not exist. dut.v does contain mm0 but names the cache
         l1, so probes.v:7-15 "top.dut0.dsram0" would need
         "top.dut0.l1.dsram0" there. probes.v was written against a
         hierarchy that no longer exists in either file.

      g. mainmemory.v:40 bounds check is "a > ENTRIES". Valid indices
         are 0..ENTRIES-1, so the check is off by one and lets the
         last-plus-one access through. Precedence makes the bitwise &
         work as intended, but && is the correct operator.

      h. bitrf.v:32 and lrurf.v:32 declare regs[0:8192], which is
         8193 entries, while ENTRIES is 8192 at line 25 and the clear
         task loops i<ENTRIES at line 29. The last entry is never
         cleared. sram.v:19 and dsram.v:23 correctly use
         [0:ENTRIES-1]. Asymmetric and wrong.

      i. bitrf.v:14 declares "output reg [3:0] q" and line 35 drives
         it with a continuous assign. lrurf.v:13 and :35 do the same.
         A continuous assign to a variable is illegal in Verilog.
         Icarus accepts it; Verilator will not. This blocks the
         project's stated Verilator flow
         (planning/tools/tool_decisions.md:21).

      j. sram.v:25 uses "always @(a) rd = ram[a];". Sensitivity on
         the address only means a write to the currently addressed
         location does not update rd. The read port is also unused
         (line 13). dsram.v uses a registered read qualified by
         read_q instead. Two different read models for the tag and
         data arrays with no stated reason.

S-18  The example is not in the language the project targets. Every
      file in examples/1mb_l1/rtl/src is .v and uses Verilog-2001
      constructs: always @*, reg, casez, no packages, no always_comb,
      no interfaces. CLAUDE.md specifies SystemVerilog-2023.
      cgen_config.schema.json:229 says the pacino profile applies
      "always_comb over chained assign" and "file-scope package
      imports". planning/tools/verilog_style.md:15-19 gives the
      always_comb rationale. The example therefore cannot serve as a
      pacino-profile template without translation. It can serve as a
      generic-profile reference. Fix: decide which, and state it in
      the example's own README or in PROJECT_STATUS.

S-19  The schema permits configurations neither the RTL nor the model
      supports. IF-2 allows refill_width_bits below line_bytes*8,
      that is a multi-beat fill. The RTL fills a whole 256-bit line
      in one cycle (cache.v:253-255) and the model hard-rejects any
      other case: options.cpp:245 requires mm_fetch_size ==
      l1_line_size. Not a schema defect. Recorded so the first
      multi-beat config is not mistaken for a regression.

### R3 -- examples/1mb_l1/model vs the two schemas

M-01  Field-by-field mapping, input schema to Options. Legend:
      OK = present and compatible, NAME = present under a different
      name or vocabulary, ABSENT = no counterpart, EXTRA = model has
      a field the schema does not.

      schema_version         NAME    json_format_version,
                                     options.h:61, "0.0.1"
      name                   ABSENT  tc_prefix (options.h:100) drives
                                     filenames instead. The schema
                                     says name does
                                     (cgen_config.schema.json:28).
      description            ABSENT
      cache_type             ABSENT  model is unconditionally a
                                     write-back write-allocate dcache
      level                  ABSENT  hard-coded l1_ prefixes;
                                     options.h:52 comment says
                                     "Future supports multi-level"
      geometry.capacity_bytes  OK    l1_capacity
      geometry.line_bytes      OK    l1_line_size
      geometry.associativity   OK    l1_associativity
      geometry.banks         ABSENT
      addressing.indexing    NAME    l1_tag_type "PHYSICAL"
                                     (options.cpp:157). Tag
                                     physicality is not the same
                                     decision as index source. PIPT
                                     vs VIPT is not expressible.
      addressing.va_bits     ABSENT  l1_mmu_present bool is the
                                     nearest thing
      addressing.pa_bits     NAME    mm_address_bits, and it is
                                     derived, not stated. See S-12.
      addressing.page_bytes  ABSENT
      addressing.alias_policy ABSENT
      policies.read_miss       OK    "ALLOCATE" vs schema "allocate"
      policies.write_miss      OK    "ALLOCATE" vs schema "allocate"
      policies.write_hit     NAME    default "NO-WRITE-THRU"
                                     (options.cpp:137). Schema enum
                                     is write_back | write_through.
                                     A third spelling of write_back.
      policies.replacement   NAME    "PLRU" vs schema "tree_plru"
      policies.critical_word_first OK  l1_critical_word_first
      miss_handling.mshrs    ABSENT
      miss_handling.mshr_targets ABSENT
      miss_handling.victim_buffer_entries OK  l1_victim_buffer_size,
                                     forced to 0 at options.cpp:237
      miss_handling.fill_buffer_entries ABSENT
      interfaces.core_read_width_bits  NAME  l1_word_size
                                     (options.h:108), one field for
                                     both directions, fixed at 32,
                                     carrying its own FIXME
      interfaces.core_write_width_bits NAME  same field
      interfaces.refill_width_bits NAME  mm_fetch_size, in BYTES
                                     (options.cpp:187) where the
                                     schema is in BITS. Unit mismatch
                                     on a field that feeds a width.
      interfaces.next_level_name ABSENT
      maintenance.*          ABSENT  entire group
      emission.*             ABSENT  entire group except a loose
                                     mapping of output_dir to
                                     data_dir (options.h:101)
      verification.golden_vectors ABSENT
      verification.directed_tests NAME  seven bools, options.h:
                                     150-157, set in code not JSON,
                                     hard-coded at mdl_main.cpp:10-17
      verification.assertions ABSENT

      EXTRA fields with no schema counterpart:
        l1_store_buffer_size   options.h:77. Not in the schema and
                               not in GLOSSARY.
        l1_coherency_protocol  options.h:75, default "NONE",
                               printed in the datasheet at
                               gen.cpp:311 and written to JSON at
                               gen.cpp:323. GLOSSARY.md:177-179
                               explicitly excludes this: "Coherence
                               protocols. No MESI, MOESI, or
                               directory participation. `coherence`
                               is not a field." The model has the
                               field a planning document forbids.
        l1_mmu_present         options.h:81
        l1_mpu_present         options.h:82
      All four must be deleted or given a schema home and a rationale.

M-02  The model's emitted JSON shares no structure with
      cgen_elaborated_config.schema.json. gen.cpp:278-559 writes a
      flat object keyed l1_tagMsb, mm_lineMask, dary_files and so on.
      The elaborated schema requires exactly five top-level members,
      provenance, input, derived, manifest, checks, with
      additionalProperties false
      (cgen_elaborated_config.schema.json:7-8). Nothing overlaps.
      Specifically absent from the model's output:
        provenance   all of it. No tool_version, no tool_git_sha,
                     no input_sha256, no generated_utc, no host.
                     Nothing reproducible.
        input        the model does not preserve its input verbatim
        manifest     the model writes bits_file, tags_file, mm_file
                     and a dary_files array of bare paths
                     (gen.cpp:549-558). No sha256, no bytes, no kind,
                     no module. Regeneration cannot diff.
        checks       none. See M-17.
        make_targets none.
      This is the largest single gap in R3. The elaborated file is
      described at cgen_elaborated_config.schema.json:5 as "the file
      a task's Results Capture cites", and the model produces
      nothing that could be cited.

M-03  The value-plus-units duplication that GLOSSARY section 1 exists
      to forbid is present, AND the round-trip defect it names is
      live in the code. This is the highest-value finding in R3.
        options.h:64-66  l1_capacity, l1_capacity_value,
                         l1_capacity_units
        options.h:90-92  mm_capacity, mm_capacity_value,
                         mm_capacity_units
        gen.cpp:297-299  writes all three for l1
        gen.cpp:526-528  writes all three for mm
        options.cpp:321  mm_capacity = json["mm_capacity_value"]
      The reload reads the VALUE into the CAPACITY. With the default
      mm_capacity of 8388608, utils.cpp:620-622 renders that as
      value 8 units "MB", so a load-store-load turns 8MB into 8
      bytes. GLOSSARY.md:30-34 describes this defect almost word for
      word: "its reload path read `mm_capacity_value` into
      `mm_capacity`, so a round trip silently turned 4GB into 4."
      Confirmed present at options.cpp:321. Note the l1 path at
      options.cpp:285 reads the correct key, so only mm is broken,
      which is why it survived. The types match, so it is silent.

M-04  getValueAndUnits is declared twice and defined once.
        gen.h:16       CacheGen::getValueAndUnits(uint64_t)
                       declared, never defined anywhere in the
                       manifest. A link error if ever called.
        utils.h:52     Utils::getValueAndUnits(uint64_t), defined at
                       utils.cpp:613
      The definition does integer division (utils.cpp:620-622), so
      1.5MB renders as "1 MB". ValueUnit is a
      pair<uint64_t,string> (utils.h:40) while the commented-out
      struct it replaced used a double (utils.h:42-45), and gen.cpp
      still prints it through FLT, fixed<<setprecision(3)
      (gen.cpp:273, 293), which has no effect on an integer. Fix:
      delete the CacheGen declaration, and decide whether the
      rendered string is allowed to be lossy. GLOSSARY.md:95-97 says
      the elaborated output "may carry a rendered string for the
      datasheet"; a lossy one is worse than none.

M-05  The four hand-maintained PLRU copies GLOSSARY section 6 names
      are all present and they all agree, which is exactly the
      situation that section says is not the same as one source.
        lrurf.v:39-47        update table, RTL
        ram.cpp:19-36        update table, model
        compare.v:85-97      victim decode, RTL
        mdl.h:71-84          victim decode, model
        ram.cpp:12-17        a fifth copy, as a comment
      Verified equivalent by hand: both update tables map way0 to
      {0,b1,0}, way1 to {0,b1,1}, way2 to {1,0,b0}, way3 to
      {1,1,b0}; both victim decodes map states 0,1 to way3, 2,3 to
      way2, 4,6 to way1, 5,7 to way0. The elaborated schema's
      derived.replacement_state.update_table and victim_table
      (cgen_elaborated_config.schema.json:82-94) are the correct
      fix, and neither the RTL nor the model consumes them today.

M-06  Associativity is a runtime option the code cannot vary. It is
      hard-coded to 4 in at least these places:
        ram.cpp:24-33       updateLru switch, cases 0..3, asserts
                            on default
        mdl.h:73-83         getLruWay switch over 8 state values
        addresspacket.h:55-57  bitset<4> for val, mod and lru
        ram.h:97-105        masks 0x7, 0xF, shifts 4 and 8
        utils.cpp:137,190   "if(exp.size() != 4 || act.size() != 4)"
        utils.cpp:144,197   "for(size_t way=0;way<4;++way)"
        mdl_tests.cpp       "for(size_t i=0;i<4;++i)" in eight
                            places
      gen.cpp:437 computes l1_lru_bits = numWays - 1, which is the
      tree-PLRU bit count and is correct only for the tree, guarded
      on l1_replacement_policy == "PLRU" with no other policy
      implemented. The schema offers associativity 1,2,4,8,16 and
      four replacement policies. The model supports one of each.

M-07  The status-bit packing has three hand-agreed descriptions and
      no schema field.
        ram.cpp:35,46,57    {val<<8, mod<<4, lru}
        gen.cpp:100-102     file format "VVVV DDDD 0LLL"
        top.v:112-114       "concatenate format { vvvv,mmmm,lru }"
        utils.cpp:475-480   the reader that must match all three
      The RTL keeps the three fields in three separate arrays
      (cache.v:412, :426, :440) and only the memory image file joins
      them. cgen_elaborated_config.schema.json:78 has
      replacement_state.bits_per_set but nothing describing the
      valid/dirty/replacement packing, so a generator could not emit
      the RTL, the model and the image file consistently. Fix: add a
      derived.status_bits object giving field order, offsets and
      padding.

M-08  ram.h:96 carries its own finding: "FIXME some hard coded magic
      numbers in this set of methods". getLru masks 0x7 (ram.h:97),
      getMod shifts 4 masks 0xF (ram.h:102), getVal shifts 8 masks
      0xF (ram.h:105). All three assume 4 ways and 3 LRU bits. These
      are the constants M-07 would derive.

M-09  BitArray::updateLru uses a stale iterator and can silently grow
      the array. ram.cpp:19-36 calls getLru(idx), which is
      "mem[idx] & 0x7" (ram.h:97) and does NOT set q, then writes
      through q at line 35. By contrast updateMod (ram.cpp:41) and
      updateVal (ram.cpp:52) both re-find q first. updateLru works
      only because of call ordering:
        readHit    mdl.cpp:67       q set by the preceding
                                    bitsLookup at mdl.cpp:316
        readMiss   mdl.cpp:114-116  q set by the preceding updateMod
        writeHit   mdl.cpp:179-180  q set by the preceding updateMod
      Any reordering breaks it. Separately, mem[idx] default-inserts
      a zero entry when idx is absent, so a lookup silently creates
      state. Fix: set q in updateLru and use find, not operator[].

M-10  Ram::ld_line and Ram::st_line dereference q without an end()
      check (ram.cpp:134-138, :178-182). Ram::st calls ld_line at
      line 145. CacheModel::writeBack calls mm->st_line
      (mdl.cpp:373) for a line address that need not be present.
      The code acknowledges it at ram.cpp:132, :141, :176 with
      "FIXME: add error checking ?". Undefined behaviour on a miss.

M-11  Debug output is left in the shipping path, ungated. mdl.cpp:
      101-104, :112-113, :361, :367 print "HERE ..." to cout
      unconditionally, ignoring both the verbose argument and the
      Msg verbosity level. ram.cpp:202 and :206 do the same. The
      output is ASCII, which satisfies the CLAUDE.md preference, but
      it is not controllable.

M-12  The pass criterion is inverted, and the shipped configuration
      always fails.
        mdl_tests.cpp:171   basicRdEvictTest does "++errs;"
                            unconditionally before endTest, with
                            most checks commented out at lines
                            152-169
        mdl_tests.cpp:669-675  basicWrEvictTest is a stub that also
                            does "++errs;"
        mdl_main.cpp:10-17  enables basicTests and basicRdEvictTest
                            only
      So mdl always exits 1. Worse, mdl_run.cpp:55-68 adds 1000 to
      the error count of every DISABLED test, so any run with a
      test switched off can never report zero. Skipping is scored as
      a thousand failures. Fix: report disabled tests as skipped,
      not as errors, and gate completion on failures only.

M-13  The tests ignore the configured data directory and read a
      different file set than the one the file check validates.
        mdl_run.cpp:155-231  runFileChecks validates
                             data_dir/tc_prefix.{bits.memb, dN.memh,
                             tags.memh, mm.memh}
        mdl_tests.cpp        every test opens hard-coded
                             "../rtl/data/..." and
                             "../rtl/golden/..." paths
      opts.data_dir exists and no test uses it. The check therefore
      passes on one set of files while the tests read another.

M-14  File naming drift.
        mdl_tests.cpp:421-426  basicWrHitTest opens
                               "../rtl/data/basicWrHit.dN.memh" with
                               a LITERAL "dN" in the name, four
                               times in a loop. All four ways load
                               the same file, and the file name
                               contains the letter N where the way
                               index belongs. Every other test uses
                               baseFn + to_string(i) + ".memh".
        mdl_tests.cpp:570      basicRdHitTest reads its golden
                               capture from "basicRdHit.data.memh"
        mdl_tests.cpp:116,344  basicRdEvict and basicRdAlloc read
                               ".capd.memh"
      Two suffixes for one artifact kind, and one broken name.

M-15  CacheModel::mm is used uninitialised on the construction path.
      mdl.h:112 declares "Ram *mm;" with no initialiser, unlike
      every other pointer member (mdl.h:109, :114, :117 all use
      {nullptr}). The constructor calls clearResizeMdlArrays
      (mdl.cpp:33), which executes "if(mm) delete mm;"
      (mdl.cpp:244) on an indeterminate pointer. Undefined
      behaviour on every construction. Same pattern is safe for
      bits at mdl.cpp:232 because mdl.h:109 initialises it.

M-16  Options::loadFromJson leaves the object internally
      inconsistent after a reload. It never reads mm_address_bits,
      and gen.cpp never writes it, so after a reload mm_address_bits
      holds the command-line default of 32 (options.cpp:174) while
      l1_tagBits came from a solve that used log2(mm_capacity).
      Any later use of mm_address_bits sees a value the tag width
      does not match. options.cpp:275 also overwrites json_file with
      a value read out of the file it just opened.

M-17  There is no equivalent of the elaborated checks[] array.
      Options::checkOpts (options.cpp:200-250) is a fixed-point
      gate, not a set of geometry checks: it rejects anything that
      is not exactly 1MB, 32B lines, 4 ways, PLRU,
      critical_word_first true, victim buffer 0, store buffer 0, and
      mm_fetch_size equal to l1_line_size. None of GEO-1..4,
      ADDR-1..2, VIPT-1..2, IF-1..3, MSHR-1, RPL-1, TEST-1 or
      EMIT-1..3 is implemented. Because the model rejects every
      other geometry, the option plumbing that does exist has never
      been exercised. Fix: implement the checks and emit them, with
      pass/fail/not_applicable, into the elaborated file.

M-18  Dead code inside checkOpts. options.cpp:220-221 and :233-234
      are "return reportBadOption(...); return false;" where the
      second return is unreachable. options.cpp:225-229 calls
      u.to_upper on five strings and line 231 repeats to_upper on
      l1_replacement_policy.

M-19  Declared-and-never-defined, and stubs:
        gen.h:14           CacheGen::process(), no definition
        mdl_run.cpp:104-146  CacheModel::simulate is entirely
                           commented out and returns false
        mdl.h:34-35        ldd returns 0, std does nothing. These
                           are the 64-bit access path, so the model
                           has no 64-bit core interface even though
                           the schema offers core widths to 1024
                           bits (cgen_config.schema.json:184).
        options.cpp:466    Options::info prints l1_tagKB, which
                           nothing ever assigns (options.h:141)

M-20  Utils::makeMask has a latent overflow. utils.cpp:630 is
      "(1 << (msb-lsb+1)) - 1" where 1 is int. For a 32-bit field
      the shift is undefined behaviour. The widest field in this
      design is the 27-bit mm line mask, so it does not bite today.
      It will for a direct-mapped cache with a wide pa_bits, which
      the schema permits (associativity 1, pa_bits 56). Fix: use
      1u or uint64_t and handle the full-width case.

M-21  The datasheet address diagram is wrong. gen.cpp:428 is
      "for(size_t i=0;i<opts.l1_offBits-2;++i) vx += "o";" but
      l1_offBits is already the WORD offset count, set to
      log2(line_size/4) at gen.cpp:334, which is 3. Subtracting 2
      prints one 'o' where three belong, then line 429 appends
      "-- bbbb". gen.cpp:364 subtracts 2 again for the byte bits,
      correctly, in the tag-width computation. So one name,
      l1_offBits, is read as a word count in getOffsetField
      (mdl.h:43) and as a byte count in the diagram. This is the
      model-side instance of S-02.

M-22  formMMAddress is dead and unit-inconsistent with its
      neighbour. mdl.h:45-46 getMMAddr returns a LINE address
      (a >> mm_lineShift). mdl.h:48-52 formMMAddress returns a BYTE
      address (tag<<tagShift | idx<<setShift). The names do not
      signal the difference, and formMMAddress is never called;
      writeBack (mdl.cpp:364-366) builds the byte address inline and
      then converts. Fix: delete it, or rename both to state the
      unit.

M-23  Model and RTL break a duplicate-tag tie in opposite
      directions. The model's tagLookup scans ways high to low and
      returns the first match, so way3 wins (mdl.cpp:328-344). The
      RTL's line_data casez matches 5'b0???1 first, so way0 wins
      (cache.v:284-291). The condition should be impossible, which
      is precisely why the one-hot assertion of S-16 matters. Note
      the invalid-way selection does NOT diverge: both prefer the
      highest-numbered invalid way (mdl.cpp:381-387 checks val[3]
      first; compare.v:74-80 casez 4'b0??? gives way3 first).
      compare.v:68 calls that "the left most invalid way", which is
      ambiguous in this bit order and should say way3-first.

M-24  Three different main memory sizes are in play at once.
        options.h:107   default_mm_entries = 1048576>>2 = 262144,
                        with a comment reading "//256K", which is
                        correct
        options.cpp:183 mm_capacity default 8388608 bytes = 8MB.
                        8388608/32 = 262144, so these agree today
                        by coincidence, not by derivation:
                        gen.cpp:518 sets mm_entries =
                        default_mm_entries and never divides
                        mm_capacity by mm_fetch_size. Change
                        mm_capacity and they diverge silently.
        top.v:14        EXP_MM_ENTRIES = 1048576 lines, passed to
                        mainmemory at top.v:266. That is 32MB, four
                        times the model's 8MB.
      Fix: derive mm_entries, and drive the testbench parameter from
      the elaborated file.

M-25  Summary judgement for R3. The model cannot be driven from
      cgen_config.schema.json without: renaming five policy and
      geometry values to the schema vocabulary (M-01), deleting or
      homing four out-of-scope fields (M-01 EXTRA), adding the
      emission, maintenance and miss_handling groups, converting
      mm_fetch_size from bytes to bits, replacing the derived
      mm_address_bits with an input pa_bits (S-12), and rewriting
      the JSON writer to the elaborated schema (M-02). None of these
      is deep, but there are enough of them that a field-by-field
      port is the wrong approach; the option struct should be
      regenerated from the schema.

## Test Case Results

No tests were run. This task is a read-only assessment and was
granted no execute permission for build or simulation. The only
commands issued were the validation tool named in the /run procedure,
file reads of the manifest, and two directory listings (see
Assumptions). Nothing was compiled and no simulation was invoked.

Static observations that bear on future test runs, from reading only:

- mdl as configured at mdl_main.cpp:10-17 will always exit 1. See
  M-12. Any future task that gates on "model tests pass" will block
  until basicRdEvictTest:171 and basicWrEvictTest are repaired.
- The RTL will not pass Verilator as written. The continuous assign
  to a reg output in bitrf.v:35 and lrurf.v:35 is illegal, and
  cache.v has a multiply-driven net at lines 156 and 400. See S-17c,
  S-17i.
- probes.v will not elaborate against top.v. See S-17f.

## Assumptions made not explicit in the prompt

A-1  R2 says to compare the RTL against "the input configuration
     json, planning/schema/cgen_config.schema.json". That path is a
     schema, not a configuration instance. I assumed the intent was
     to compare the design against the schema, that is, to ask
     whether the schema can express this design. I did not read
     misc/example_pacino_icache.json or its elaborated companion,
     because they are not in the Context Loaded manifest.

A-2  I performed two directory listings to check claims in
     PROJECT_CORE.md about paths that exist (P-06, P-12). I listed
     names only and read no file outside the manifest. I judged that
     asserting "this path is missing" without checking would violate
     the CLAUDE.md rule against unverified assertions, and that a
     name listing is not a file read. If that reading of the rule is
     wrong, findings P-06 and P-12 should be treated as unverified
     and the rest of R1 is unaffected.

A-3  PROJECT_CORE.md:142 names templates/TASK_TEMPLATE.md as the
     declaration of task file structure. That file exists but is not
     in the manifest, so I did not open it. No finding depends on
     its content.

A-4  planning/PROJECT_STATUS.md exists but is not in the manifest.
     PROJECT_CORE.md makes it the sole authority for document status
     (P-02). I could not check whether it agrees with the STATUS
     fields in the file headers, so P-02 reports the inconsistency
     between the declared value set and the headers, and stops short
     of claiming what PROJECT_STATUS.md says.

A-5  I assumed "the fpga design is not part of this effort" excludes
     any synthesis, timing or resource commentary. S-17 reports RTL
     defects as functional and lint issues only.

A-6  Where the RTL and the model disagree, I assumed neither is
     authoritative and reported the disagreement rather than picking
     a side. S-12 and M-24 are the two cases.

## Decisions made not explicit in the prompt

D-1  I gave every finding a stable ID (P-nn, S-nn, M-nn) and a file
     and line citation. The prompt asked for enumerations; IDs make
     them citable from the next task file without re-deriving them.

D-2  I recorded S-01 as an explicit negative result: the geometry
     half of the schema is sound and this design fits it. The
     PROJECT_CORE standing rule says a negative result is a
     deliverable, and "the schema is adequate here" bounds the work
     as usefully as a defect does.

D-3  I reported RTL and model defects found while assessing (S-17,
     M-04, M-09, M-10, M-12, M-15, M-18, M-19, M-20) even though
     R2 and R3 asked specifically about schema issues. FIX WHAT YOU
     FIND applies, and this task has no write permission for those
     files, so reporting is the available action.

D-4  I read examples/1mb_l1/model/inc/json/json.h and
     examples/1mb_l1/model/src/jsoncpp.cpp in full, as the manifest
     requires. They are stock jsoncpp 1.7.4 amalgamation
     (json.h:93-96) and contributed nothing to any finding. See
     Deferred Work for the recommendation.

D-5  I did not propose replacement text for any planning document.
     CLAUDE.md and PROJECT_CORE.md:123 both put drafting with the
     user. Each finding states what is wrong and what the fix should
     achieve, not the words to use.

D-6  I ordered findings by document and by severity within R2 and
     R3, putting S-12 and M-03 first in the Summary because both are
     silent-wrong-answer defects rather than gaps.

## RVA23 compliance risks and gaps noticed

The manifest contains no RVA23 material and no ISA-facing code. The
cache is addressed by a flat 32-bit physical address with no
privilege, PMA, PMP or fence semantics anywhere in the RTL or the
model. Nothing in this task's context can be assessed for RVA23
compliance, so this section is TBD by absence rather than by
omission.

Three items are worth carrying forward for whoever does own RVA23
compliance:

R-1  fence.i. GLOSSARY.md:68 states that invalidate_all is fence.i,
     and cgen_config.schema.json:206 says invalidate_all is
     "Required for fence.i on an icache". The example implements no
     maintenance operation at all (S-11). An icache generated from
     this example as a template would not support fence.i.

R-2  Sv39/Sv48/Sv57. cgen_config.schema.json:91 offers va_bits
     32/39/48/57 and page_bytes fixed at 4096. The model has no
     virtual address concept beyond an l1_mmu_present boolean
     (options.h:81) and the RTL has none. A VIPT L1 sized against
     RVA23 page rules cannot be generated or checked today. GLOSSARY
     VIPT-1 is the check that would matter and it is not implemented
     (M-17).

R-3  Address width. addressing.pa_bits allows 32 to 56
     (cgen_config.schema.json:96-98). The RTL is hard-wired to a
     32-bit request address (cache.v:29) and a 14-bit tag
     (cache.v:51). Any RVA23 implementation with a physical address
     wider than 32 bits requires the parameterization that S-12 and
     S-17a show is not there.

## Deferred Work

DW-1  Repair the RTL defects in S-17. Ten items, a to j. Three block
      Verilator (c, d/e, i) and so gate the tool flow that
      planning/tools/verilator_decisions.md describes. Suggested
      order: i, c, d/e, then f, then the rest.

DW-2  Repair the model defects in R3. Suggested order by severity:
      M-03 (silent 8MB to 8 on reload), M-15 (undefined behaviour on
      construction), M-09 and M-10 (stale and unchecked iterators),
      M-12 (inverted pass criterion), then the rest.

DW-3  Decide the schema additions from R2. The ones that require a
      decision rather than an edit: S-02 offset units and sub-field,
      S-03 byte-enable width, S-04 splitting critical_word_first,
      S-05 next-level model, S-06 timing group, S-07 per-array
      storage, S-13 reset polarity, S-14 extension point.

DW-4  Mechanical schema edits with no decision needed: S-08 add
      "probes" to both enums and make them one $ref, S-09 split
      vectors, S-10 extend TEST-1, S-11 default invalidate_all to
      false.

DW-5  Reconcile the naming in P-03, P-04 and P-05 in one pass. Three
      documents, two files, five wrong references. This is the
      cheapest R1 item and it removes the most confusion per edit.

DW-6  Populate ANTIPATTERNS.md (P-01). This task itself produced
      candidate entries: a manifest that names a 7400-line
      third-party amalgamation, and a Hypothesis that asks the IA to
      compare a design against a schema while calling it "the input
      configuration json".

DW-7  Remove examples/1mb_l1/model/inc/json/json.h and
      examples/1mb_l1/model/src/jsoncpp.cpp from future context
      manifests unless the task is about JSON parsing. They are 7401
      of the 14727 manifest lines, they are unmodified third-party
      code, and they contributed to no finding (D-4). If the manifest
      must name them for completeness, mark them "read not required".

DW-8  Decide whether examples/1mb_l1 is a pacino-profile template or
      a generic-profile reference (S-18). Everything downstream that
      treats it as the former will need a Verilog-2001 to
      SystemVerilog-2023 translation first.

DW-9  Decide the ownership question of GLOSSARY open question 1
      (regenerate in place vs hand-own after first emission). It is
      the only one of the four that is not already answered
      elsewhere (P-16) and GLOSSARY.md:196-197 calls it "close to
      irreversible in practice".

## Other Notes

N-1  Hypothesis result, stated plainly. "Test whether the support and
     planning files are consistent" -- they are not; 20 findings, of
     which P-03, P-04, P-05, P-07, P-08, P-09 and P-10 are
     inconsistencies between documents rather than gaps within one.
     "Test whether the json configuration schema is complete" -- it
     is not complete for either component, but it is much closer for
     the RTL than for the model. The input schema's geometry,
     addressing, policies and miss_handling groups are in good shape.
     The emission group and the elaborated schema are where the work
     is.

N-2  The schema is markedly better written than the code it will
     replace. cgen_config.schema.json carries rationale in its
     description fields, and GLOSSARY sections 4 and 6 name specific
     defects in the predecessor. This assessment confirms that those
     named defects are real and still present: the value/units round
     trip (M-03) and the four PLRU copies (M-05) are both exactly
     where GLOSSARY says they were. The planning work is being done
     against the right evidence.

N-3  The emit enum and the manifest kind enum are byte-identical
     eight-value lists in two files (cgen_config.schema.json:238 and
     cgen_elaborated_config.schema.json:155). They must stay in step
     and nothing makes them. Same class of defect as M-05, one level
     up. Fold into DW-4.

N-4  Two checks named in GLOSSARY section 5 cannot be evaluated for
     any configuration the current tooling accepts. VIPT-1 and
     VIPT-2 need a VIPT config; the model rejects everything but the
     one PIPT geometry (M-17). They will first run on a config that
     has never been exercised.

N-5  The example's four-way tree PLRU state is 3 bits and the model
     pads it to 4 in the image file (gen.cpp:100-102 "0LLL",
     mdl.cpp:235 comment "pad LRU to 4 bits to match vlg"), while
     the RTL's lrurf is 3 bits wide (lrurf.v:13) and top.v's expect
     array is 12 bits total for 4+4+3 (top.v:112-114). The padding
     exists only in the file format. This is a small thing that will
     bite a generator, and it is the kind of detail the
     derived.status_bits object of M-07 should carry.

N-6  Context cost of this task: 14727 lines across 39 files, of
     which 7401 lines (50.3 percent) were stock jsoncpp. See DW-7.

N-7  Nothing in the manifest was modified. The only file written is
     this one, between the RESULTS markers, plus the Model field in
     the header as CLAUDE.md permits.

## Files Modified
- prompts/INFRA-001.md

:: RESULTS:END ::

=============================================================
:: CONTEXT:START ::
=============================================================

# Context usage

Context used: 33 percent

Window 1,000,000 tokens. Used 332,600. Free 667,400.

## Usage by category

| Category                | Tokens  | Percent |
|-------------------------|---------|---------|
| Messages                | 300.6k  |   30.1  |
| System tools            |  23.2k  |    2.3  |
| System tools, deferred  |  16.3k  |    1.6  |
| System prompt           |   4.4k  |    0.4  |
| Skills                  |   2.6k  |    0.3  |
| Memory files            |   1.8k  |    0.2  |
| MCP tools, deferred     |   1.5k  |    0.1  |
| Free space              | 667.4k  |   66.7  |

## Memory files

```
  Project  1.7k  CLAUDE.md

  AutoMem    45  ~/.claude/projects/
                 -home-jeff-Development-jeffnye-gh-cachegen/
                 memory/MEMORY.md
```

## MCP tools

All six are deferred. None was loaded or called this session.

| Tool                                          | Tokens |
|-----------------------------------------------|--------|
| mcp__claude_ai_Gmail__authenticate             |   206 |
| mcp__claude_ai_Gmail__complete_authentication  |   267 |
| mcp__claude_ai_Google_Calendar__authenticate   |   217 |
| mcp__claude_ai_Google_Calendar__complete_auth. |   282 |
| mcp__claude_ai_Google_Drive__authenticate      |   212 |
| mcp__claude_ai_Google_Drive__complete_auth.    |   277 |

## Skills

| Skill                    | Source   | Tokens |
|--------------------------|----------|--------|
| run                      | Project  |   ~20  |
| dataviz                  | Built-in |  ~380  |
| claude-api               | Built-in |  ~360  |
| design                   | Built-in |  ~340  |
| code-review              | Built-in |  ~270  |
| update-config            | Built-in |  ~240  |
| artifact-capabilities    | Built-in |  ~210  |
| claude-in-chrome         | Built-in |  ~180  |
| schedule                 | Built-in |  ~130  |
| loop                     | Built-in |  ~120  |
| keybindings-help         | Built-in |   ~80  |
| artifact-diagramming     | Built-in |   ~70  |
| artifact-design          | Built-in |   ~70  |
| fewer-permission-prompts | Built-in |   ~60  |
| simplify                 | Built-in |   ~60  |
| security-review          | Built-in |   ~30  |
| init                     | Built-in |   ~20  |

Only the run skill was invoked this session.

## Notes

Figures are the /context snapshot taken after INFRA-001 completed.
Usage has grown since by the size of this exchange.

Nearly all of the 300.6k message total is the INFRA-001 context
manifest, read under the CLAUDE.md rule requiring every context file
to be read completely before acting.

Bash results account for 161.3k tokens, 16 percent of the window. The
harness flagged this and suggested piping through head, tail or grep,
or using Read with offset and limit instead of cat on large files.

About half the Bash total is the two jsoncpp amalgamation files named
in the INFRA-001 manifest:

```
  examples/1mb_l1/model/inc/json/json.h     2139 lines,  70,184 bytes
  examples/1mb_l1/model/src/jsoncpp.cpp     5262 lines, 153,961 bytes
```

Those 7401 lines are 50.3 percent of the manifest's 14,727 lines. They
are unmodified third-party code, jsoncpp 1.7.4, and contributed to no
finding in INFRA-001. Dropping them from future manifests, or marking
them read not required, is recorded as INFRA-001 deferred work item
DW-7.

The remaining Bash cost is not reducible by piping. The manifest rule
requires whole files, so head, tail and grep do not apply. The saving
is in the manifest, not in the command.
:: CONTEXT:END ::

