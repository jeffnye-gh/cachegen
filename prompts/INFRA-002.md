<!-- SPDX-License-Identifier: Apache-2.0                       -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com -->
=============================================================
# Task Header
=============================================================
:: HEADER:START ::

| Field        | Value                   | Notes                    |
|--------------|-------------------------|--------------------------|
| Task ID      | INFRA-002               |                          |
| Date         | 2026-08-24              |                          |
| Module       | infra structure         | assessment task          |
| Run time     | 20m.42s                 |                          |
| Ctx %        | 30%                     |                          |
| Model        | claude-opus-5[1m]       | effort not exposed       |
| Resume sha   | 6b3295ee-de44-411d-8f06-53a9e5505380 |             |
| IA session   | 002                     |                          |

Task:   [x] experiment  [ ] implementation  [ ] debug
        [ ] cleanup     [ ] testbench       [ ] verification
Mode:   [x] automated   [ ] manual          [ ] interactive
Status: [ ] in-progress [x] complete        [ ] abandoned

# Task Overview

Assess cgen_config.schema.json version 2.0.0, which replaced the
version INFRA-001 assessed.

Four questions to answer:

1. Which of the 64 INFRA-001 findings are closed by 2.0.0, which
   remain open, and which were superseded or withdrawn.

2. Does 2.0.0 validate correctly as a JSON Schema document, that
   is, does every conditional actually fire for the configurations
   it is meant to constrain.

3. Can the design in examples/1mb_l1/rtl and examples/1mb_l1/model
   now be represented, and what remains missing.

4. What did the version bump break. 2.0.0 removed a field and
   retyped another. Existing config instances and the elaborated
   schema were not revised with it.

Questions 2 and 4 are new. INFRA-001 assessed expressiveness only.

:: HEADER:END ::

=============================================================
:: DISCUSSION:START ::
=============================================================

# Results Discussion

## Claude.code Console Output

All 64 IDs present, no duplicates, counts match the stated totals. Validation
  re-passes and no context file was modified.

  INFRA-002 executed. Results Capture written to prompts/INFRA-002.md between
  the RESULTS markers, plus the Model header field.

  Answers to the four questions:
  
  1. Closure. 7 CLOSED, 6 PARTIAL, 1 WITHDRAWN, 50 OPEN, 0 SUPERSEDED, 0
  UNVERIFIABLE. Every closure is in the input schema. 2.0.0 touched neither the
  elaborated schema, the planning documents, the RTL nor the model, so 19 of 20
  P findings and all 25 M findings are open by construction.

  2. The Hypothesis is not confirmed. No allOf branch in 2.0.0 can be bypassed
  by omitting an optional object. Every conditional either guards an object the
  top level already requires, or guards one whose omission also removes the
  constrained field. Recorded as a negative result with per-branch trigger and
  reachability analysis.

  The real mechanical defects are elsewhere:
  - R1-1 — GLOSSARY.md:74-80 states the schema rejects inapplicable fields
    rather than accepting and ignoring them. 2.0.0 does that in one branch of
    four. PIPT + va_bits, mshrs: 0 + mshr_targets: 8, and sram_model: "inferred"
    + macro_name all validate today.
  - R1-2 — schema_version (:56) accepts any semver. A 1.0.0 file validates
    against 2.0.0 unchanged. The major bump Background decision 1 rests on has
    no enforcement, and the prompt's expectation that every instance fails on
    schema_version alone is not met.

  3. Expressibility. RTL: 3 gaps remain excluding the deferred set (offset
  units/word sub-field, array-side byte-enable width, recording probes). Model:
  its JSON can be made to satisfy the elaborated schema; R3 gives the six
  requirements, one of which (manifest) is blocked because the model's artifacts
  are preload images and manifest[].kind has no preload.

  4. What 2.0.0 broke — three items, none previously reported: emission.emit 
  "golden" vs verification.golden_vectors are now two mechanisms for one
  decision with no precedence; the clock port name fell out of the pacino
  profile description when reset was promoted to its own fields, so it is now
  stated nowhere; and $defs.artifact_kind asserts at :25 that the two lists "are
  one definition referenced twice" while leaving the elaborated enum unrevised
  — a false claim of unity where there was previously visible duplication.

  On the four drafter decisions: 1, 3 and 4 are right as made. Decision 2 is
  right in the number and wrong in the rationale — capacity_bytes and level have
  no conditional between them, so level 3 was never inexpressible; separately,
  GLOSSARY.md:167 puts L3 out of scope.

  One instruction conflict, recorded as assumption A-1, not halted on:
  Background gives S-07 two dispositions — the DELIBERATELY DEFERRED list says
  "Mark them OPEN"; the S-07 CAVEAT says "record S-07 as WITHDRAWN". Same scope,
  different values. I took WITHDRAWN and quoted both clauses.


## Results Assessment

## Follow-on Actions
- [ ] As needed document here
- or Nothing required

:: DISCUSSION:END ::

=============================================================
:: PROMPT:START ::
=============================================================

## Task ID
INFRA-002

## Context Loaded
@prompts/INFRA-001.md

@planning/ANTIPATTERNS.md
@planning/CLOSED_TECH_DEBT.md
@planning/GLOSSARY.md
@planning/PROJECT_CORE.md
@planning/schema/cgen_config.schema.json
@planning/schema/cgen_elaborated_config.schema.json

@planning/tools/tool_decisions.md
@planning/tools/verilator_decisions.md
@planning/tools/verilog_style.md

@misc/example_pacino_icache.json

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
@examples/1mb_l1/model/inc/mdl.h
@examples/1mb_l1/model/inc/msg.h
@examples/1mb_l1/model/inc/options.h
@examples/1mb_l1/model/inc/ram.h
@examples/1mb_l1/model/inc/utils.h

@examples/1mb_l1/model/src/gen.cpp
@examples/1mb_l1/model/src/gen_main.cpp
@examples/1mb_l1/model/src/mdl.cpp
@examples/1mb_l1/model/src/mdl_main.cpp
@examples/1mb_l1/model/src/mdl_run.cpp
@examples/1mb_l1/model/src/mdl_tests.cpp
@examples/1mb_l1/model/src/options.cpp
@examples/1mb_l1/model/src/ram.cpp
@examples/1mb_l1/model/src/utils.cpp

## Context Comments

All files in the context are read only.

BASELINE. The manifest holds 2.0.0 only. INFRA-001's Results
Capture is the authority for what the previous version said; its
file and line citations refer to that version and will not match
the file you are reading. Cite 2.0.0 lines as evidence for CLOSED
and OPEN. Quote INFRA-001's own text when you need to establish
what the prior state was. If a finding cannot be dispositioned
because the prior state is not recoverable from INFRA-001's text,
mark it UNVERIFIABLE and say which part of INFRA-001 was
insufficient. Do not guess.

json/json.h and jsoncpp.cpp are deliberately absent. They are
stock jsoncpp 1.7.4, they were 50.3 percent of the INFRA-001
manifest, and they produced no finding. This applies INFRA-001
deferred work item DW-7. If any finding here depends on jsoncpp
behaviour, report that dependency and stop rather than reading
the files.

misc/example_pacino_icache.json is new to the manifest. INFRA-001
assumption A-1 records that it exists and was not read. R4 needs
it.

prompts/INFRA-001.md is in the manifest because R0 audits its
findings. Read the Results Capture section; the Discussion and
Prompt sections are background.

## Hypothesis

INFRA-001 found that the input schema expressed the RTL geometry,
policies and replacement but not byte-enable width, next-level
model, hit timing, per-array storage kind, reset polarity or the
probes artifact, and that the C++ model could not be expressed at
all.

2.0.0 addresses a subset of that deliberately. This task tests
whether the subset it addresses is actually closed, whether the
document is mechanically correct, and what the change broke
elsewhere.

The mechanical question matters because a subschema placed under
"properties" applies only when the key is present. Any conditional
constraining an optional object can be bypassed by omitting that
object. INFRA-001 did not test for this and 2.0.0 claims to fix
one instance of it.

## Background

PROVENANCE. 2.0.0 is a PA-direct correction, session-002. It
carries no task number. It was drafted whole rather than patched,
so every line differs from the previous file even where the intent
was unchanged. Treat an unexplained difference in a description
field as a finding, not as an intended edit.

Four decisions were made by the drafter and are open to challenge:

  1. Major version bump to 2.0.0, on the grounds that removing
     policies.critical_word_first and retyping
     verification.assertions from boolean to array are both
     breaking.
  2. geometry.capacity_bytes maximum raised from 8388608 to
     67108864, rather than capping level at 2, so that level 3
     stays expressible.
  3. Both references to VOCABULARY.md changed to GLOSSARY.md,
     which is where that content is.
  4. The filename left as cgen_config.schema.json, matching the
     INFRA-001 manifest rather than either of the two other names
     in circulation.

Report any of the four you judge wrong, with the consequence.

DELIBERATELY DEFERRED. These INFRA-001 findings were held back
pending a decision and are expected OPEN. Mark them OPEN and move
on. Do not propose designs for them and do not re-derive them:

  S-05 next-level model description
  S-06 hit timing group
  S-07 per-array storage kind
  M-01 model emission group
  the write buffer field implied by M-01's l1_store_buffer_size

KNOWN BREAKAGE. 2.0.0 introduced $defs.artifact_kind and made
emission.emit reference it. $ref does not cross files. The
manifest[].kind enum in cgen_elaborated_config.schema.json was not
revised and does not contain probes, preload or golden. R5 covers
this. It is stated here so it is not reported as a discovery.

S-07 CAVEAT. INFRA-001 cites four values in the
emission.sram_model enum. 2.0.0 has three. With only 2.0.0 in the
manifest the discrepancy cannot be attributed, so record S-07 as
WITHDRAWN and state that the prior file was not available to check
it against.

## Binding Previous Decisions

1. PLANNING IS READ ONLY. Report the change you would make; do not
   make it. This includes both schema files.

2. NO EXECUTE PERMISSION. No build, no simulation, no test run.
   The model as shipped always exits 1 (INFRA-001 M-12), so
   execution evidence is unavailable in any case and is not being
   asked for. Reason about validation by reading the schema, not
   by running a validator.

3. DIRECTORY LISTING IS GRANTED. You may run ls, names only, to
   verify path claims. This closes INFRA-001 assumption A-2. Do
   not read any file outside the manifest.

4. NEGATIVE RESULTS ARE DELIVERABLES. "This gap is closed and here
   is the field that closes it" is a finding. So is "this whole
   group is now adequate."

## Specific Requirements

R0 -- FINDING CLOSURE AUDIT. One table row for every one of the 64
      INFRA-001 finding IDs. Columns: ID, disposition, evidence.
      Disposition is exactly one of CLOSED, OPEN, PARTIAL,
      SUPERSEDED, WITHDRAWN, UNVERIFIABLE. Evidence cites a file
      and line in the current tree for CLOSED, PARTIAL and OPEN.
      Do not mark anything CLOSED on the strength of a description
      alone; cite the field that closes it.

R1 -- SCHEMA CORRECTNESS. Assess cgen_config.schema.json as a JSON
      Schema document, independent of what it describes. For each
      branch of allOf, state the configuration that triggers it and
      confirm the constraint is reachable. Report every case where
      omitting an optional object bypasses a constraint meant to
      apply. Report any property no configuration can reach, any
      enum value with no consumer, any two fields whose ranges
      contradict each other, and any use of patternProperties or
      additionalProperties that does not do what its description
      claims.

R2 -- RTL EXPRESSIBILITY. Assess examples/1mb_l1/rtl/src against
      2.0.0. The question is whether the schema can express this
      design. Enumerate remaining missing fields, extra fields and
      ambiguities, excluding the deferred set named in Background.

R3 -- MODEL EXPRESSIBILITY. Same assessment for
      examples/1mb_l1/model, against both schemas. State whether
      the model's emitted JSON can now be made to satisfy
      cgen_elaborated_config.schema.json and what that requires.

R4 -- INSTANCE VALIDATION. For misc/example_pacino_icache.json,
      state field by field whether it validates against 2.0.0.
      Every instance in the tree is expected to fail on
      schema_version alone; the useful output is the full list of
      what else fails, and the minimum edit that makes it valid.
      If the file names a field 2.0.0 removed, say what replaces
      it.

R5 -- CROSS-SCHEMA CONSISTENCY. Enumerate every construct that
      appears in both schema files and must agree. For each, state
      whether it currently agrees and give both citations. The
      artifact_kind and manifest[].kind pair is known broken and is
      one row; find the others. Recommend the mechanism that keeps
      them in step, given that $ref does not cross files.

R6 -- FIELD MAPPING TABLE. One row per field of 2.0.0. Columns:
      schema path, elaborated path, Options member, status. Status
      is OK, RENAME, ABSENT or EXTRA. Include a row for every
      Options member with no schema path, marked EXTRA. This table
      is the input to the options regeneration work and is the
      primary deliverable after R0.

## Constraints

Do not modify any files in the context.

The only file you may write is this one, in the results section
between the markers, plus the Model field in the header.

Report and halt on any inconsistent or ambiguous instruction
rather than choosing an interpretation. If you do choose one,
record it as an assumption and quote the wording that was
ambiguous.

Every finding cites a file and a line. A finding you cannot cite
is not reported.

Do not propose replacement text for any planning document or
schema. State what is wrong and what a fix must achieve.

Findings in the deferred set named in Background are OPEN by
design. Do not pad R2 or R3 with them.

## Deliverables

- Results Capture filled in below, in this file:
  prompts/INFRA-002.md

- The R0 closure table, all 64 IDs, no omissions.

- The R5 cross-schema consistency table.

- The R6 field mapping table, complete in both directions.

Fill in every section. Test Matrix may be omitted: this task adds
no test cases. Test Case Results states that no execution
permission was granted and lists any static observation bearing on
a future run.

:: PROMPT:END ::

=============================================================
:: RESULTS:START ::
=============================================================

## Summary

Read-only assessment of cgen_config.schema.json 2.0.0. All 40
manifest files were read completely. json/json.h and jsoncpp.cpp
were not read and no finding depends on jsoncpp behaviour. Nothing
in the context was modified.

Citation legend, used throughout to stay inside 80 columns:

  IN   planning/schema/cgen_config.schema.json          (2.0.0)
  EL   planning/schema/cgen_elaborated_config.schema.json (1.0.0)
  GL   planning/GLOSSARY.md
  PC   planning/PROJECT_CORE.md
  I1   prompts/INFRA-001.md
  EX   misc/example_pacino_icache.json

RTL and model files are cited by basename; each is unique in the
manifest.

Answers to the four Task Overview questions.

1. Closure. Of the 64 INFRA-001 findings: 7 CLOSED, 6 PARTIAL,
   1 WITHDRAWN, 50 OPEN, 0 SUPERSEDED, 0 UNVERIFIABLE. Every
   closure is in the input schema. 2.0.0 did not touch the
   elaborated schema, the planning documents, the RTL or the
   model, so 19 of 20 P findings and all 25 M findings are open by
   construction rather than by neglect.

2. Mechanical correctness. The Hypothesis is not confirmed. No
   allOf branch in 2.0.0 can be bypassed by omitting an optional
   object in a way that admits an illegal configuration. Every
   conditional either guards an object the top level already
   requires, or guards an optional object whose omission also
   removes the thing being constrained. That is a negative result
   and it is a deliverable.

   The real mechanical defects are elsewhere. The largest is that
   2.0.0 applies the "reject rather than accept and ignore"
   principle of GL:74-80 in one branch and not in the other three.
   An icache is forbidden its write fields; a PIPT config is not
   forbidden va_bits, a blocking config is not forbidden
   mshr_targets, and an inferred-SRAM config is not forbidden
   macro_name. All three validate today. See R1-1.

   Second: schema_version accepts any semver (IN:56). A file
   saying 1.0.0 validates against 2.0.0 unchanged. The major bump
   that Background decision 1 rests on has no enforcement
   mechanism, and the prompt's expectation that every instance in
   the tree fails on schema_version alone is not met. See R1-2
   and R4.

3. Expressibility. The RTL is now expressible for byte-enable
   width, reset polarity and name, fill-bus bypass, and the probes
   artifact as a request. It is still not expressible for offset
   units and the word-offset sub-field, and the probes artifact
   cannot be recorded once emitted. Excluding the deferred set,
   R2 has 3 remaining gaps, 1 extra field and 2 ambiguities.

   The model still cannot be driven from either schema. Its
   emitted JSON can be made to satisfy EL, and R3 states the six
   things that requires. One of them is blocked: the model's
   output artifacts are preload images, and EL:155 has no preload
   kind.

4. What the change broke. Three things, all introduced by 2.0.0
   and none of them reported before.

   - emission.emit "golden" (IN:33) and verification.golden_vectors
     (IN:319) are now two mechanisms for one decision, with no
     stated precedence. See R1-6.
   - The pacino profile description lost "clk/rstn" when reset was
     promoted to its own fields. IN:271 now names no clock, while
     GL:100 still says the profile carries clk/rstn. The clock port
     name is stated nowhere in the schema. See R1-7.
   - $defs.artifact_kind (IN:24-38) made emission.emit and
     manifest[].kind formally one definition in its own text
     (IN:25) while leaving EL:155 at the old eight values. The
     divergence the definition was written to prevent is now
     asserted in prose and contradicted in fact. This is the
     KNOWN BREAKAGE; it is R5 row 1 and it is stated here only
     because 2.0.0 is what created the false claim of unity.

## Test Matrix (testbench sessions only, omit otherwise)

Omitted. This task adds no test cases.

## What was delivered

### R0 -- Finding closure audit

All 64 INFRA-001 finding IDs, one row each. Disposition is one of
CLOSED, OPEN, PARTIAL, SUPERSEDED, WITHDRAWN, UNVERIFIABLE.
Evidence cites a file and line in the current tree.

  ID    DISP       EVIDENCE
  ----  ---------  --------------------------------------------
  P-01  OPEN       ANTIPATTERNS.md:1-12 and
                   CLOSED_TECH_DEBT.md:1-12 are still SPDX plus
                   header block only. No content added.

  P-02  OPEN       PC:261 still declares DRAFT, CLOSED, NOT
                   STARTED, DEPRECATED. GL:8, ANTIPATTERNS.md:8,
                   CLOSED_TECH_DEBT.md:8, tool_decisions.md:8,
                   verilator_decisions.md:8 and
                   verilog_style.md:8 still carry WORKING.
                   PC:259-261 exactly-one-place rule unchanged.

  P-03  OPEN       PC:283-284 still lists
                   cache_config.schema.json and output_json.md.
                   Listing planning/schema/ shows
                   cgen_config.schema.json and
                   cgen_elaborated_config.schema.json.

  P-04  OPEN       GL:15-16 still names cache_config.schema.json
                   and cache_elaborated.schema.json. Neither
                   exists. Three documents, three name pairs.

  P-05  CLOSED     IN:5 now reads "See GLOSSARY.md for the checks
                   this schema cannot express" and IN:138 reads
                   "see GLOSSARY.md exclusions". Both VOCABULARY
                   references are gone. GLOSSARY.md exists
                   (listing of planning/). Background decision 3.

  P-06  OPEN       PC:234-252 unchanged. Listing the tree: gui/,
                   cli/ and output/ absent; docs/ empty;
                   examples/, misc/, src/, versions/,
                   planning/tools/ and .claude/ present and not
                   listed.

  P-07  OPEN       PC:66-67, PC:106 and PC:72-73 unchanged. Three
                   authors, no precedence rule.

  P-08  OPEN       PC:26 "Implementation Assistant" and PC:61
                   "Interactive Assistant" unchanged. Two of the
                   three expansions are in the manifest and they
                   already disagree.

  P-09  OPEN       PC:136-138 still cites a CLAUDE.md "Fixed
                   Constants" section; PC:206-208 still cites a
                   CLAUDE.md suite-gating requirement.

  P-10  OPEN       PC:121 and PC:123 unchanged and still
                   contradict on who modifies a planning file.

  P-11  OPEN       PC:80-86 prefix list still omits BP; BP-NNN
                   still cited at PC:133-134 and PC:236.

  P-12  OPEN       PC:295 still gives ./claude/statusline.sh.
                   Listing .claude/ shows statusline.sh, so the
                   leading dot is still missing.

  P-13  OPEN       tool_decisions.md:9 and verilog_style.md:9
                   still UPDATED n/a. verilator_decisions.md:9
                   still 2026.08.24 against 2026-08-24 elsewhere.

  P-14  OPEN       verilator_decisions.md:29-31 still names
                   bp_pkg; :33-35 still names NUM_PRED_SLOTS.
                   IN:60 still names <name>_pkg.sv.

  P-15  OPEN       tool_decisions.md:13-24 is still a bare tool
                   list; the dangling "# " at :26 is still there.

  P-16  OPEN       GL:200-203 Q2 and GL:204-205 Q3 still open.
                   IN:33 still carries "model" in artifact_kind
                   and IN:103 still puts banks in geometry, so
                   both questions are still answered elsewhere.

  P-17  OPEN       GL:37-52 unchanged; no planning document names
                   the host document. IN:3 $id and IN:5 still
                   describe a standalone file.

  P-18  OPEN       PC:247-250 output/<design>/rtl,/tb; IN:282
                   rtl/, tb/, tests/; examples/1mb_l1 on disk is
                   rtl/src and model/inc,src. Three layouts.

  P-19  OPEN       No planning document states the language level
                   or extension. IN:271 still says the pacino
                   profile applies always_comb over chained
                   assign; cache.v:196 and fsm.v:144 are
                   always @*. DW-8 undecided.

  P-20  OPEN       Verified still present in PC: "SystemVerlog"
                   :34, "Cgen" :37, "elborated" :45, "testins"
                   :48, "There templates" :69, "drivend" :101,
                   "occiasionally" :107, "priviledges" :111,:114.

  S-01  CLOSED     Negative result, still true and now wider.
                   capacity_bytes 1048576 sits inside IN:92-93
                   [256, 67108864]; line_bytes 32 IN:97,
                   associativity 4 IN:101, refill_width_bits 256
                   IN:223, core widths 32 IN:210,:214, pa_bits 32
                   IN:129. The geometry half of the schema still
                   expresses this design.

  S-02  OPEN       IN:96 says line_bytes "Sets the offset field
                   width" without stating the unit. EL:69 emits
                   one offset field and EL:191-203 defines it
                   with no unit. The RTL still uses a 3-bit word
                   offset at cache.v:110 and a 2-bit byte
                   position. No sub-granule field was added and
                   EL:52 additionalProperties false blocks one.

  S-03  PARTIAL    Input half CLOSED: IN:216-220
                   write_granularity_bytes exists and states the
                   formula. Derived half OPEN: EL:53-141 has no
                   byte_enable_bits and EL:52 forbids adding one.
                   IN:217 also gives the core-side width only
                   (32/8/1 = 4 for this design, cache.v:30); the
                   array-side width dsram.v:14 needs is 32 and
                   nothing derives it.

  S-04  CLOSED     IN:230-234 refill_beat_order and IN:235-239
                   fill_bypass_to_core are two fields. IN:149-165
                   policies no longer carries
                   critical_word_first. The split S-04 asked for.

  S-05  OPEN       Deferred by design. IN:225-229 next_level_name
                   is still a bare informational string against
                   mainmemory.v:10-14 ENTRIES, READ_LAT,
                   WRITE_TPUT.

  S-06  OPEN       Deferred by design. No timing group in IN.
                   cache.v:18-19 READ_HIT_LAT and WRITE_HIT_TPUT
                   are still declared and still unread by fsm.v.

  S-07  WITHDRAWN  IN:298-302 sram_model has three values,
                   bw_ram, inferred, macro. I1:528-529 cites four
                   in the prior file. The prior file is not in
                   the manifest, so the discrepancy cannot be
                   attributed. Recorded WITHDRAWN per the S-07
                   CAVEAT. See Assumption A-1 for the conflict
                   with the deferred-set instruction.

  S-08  PARTIAL    Request half CLOSED: IN:34 adds probes to
                   $defs.artifact_kind, referenced by
                   emission.emit at IN:279. Record half OPEN:
                   EL:155 manifest[].kind is unchanged and has no
                   probes. probes.v remains 238 lines of output
                   that can be asked for and not recorded.

  S-09  PARTIAL    Request half CLOSED: IN:32-33 splits preload
                   and golden, described at IN:275. Record half
                   OPEN: EL:155 still carries "vectors" and
                   neither preload nor golden. GL:138 EMIT-2
                   still reads "tb in emit implies vectors in
                   emit", naming a value IN no longer has.

  S-10  PARTIAL    IN:325 now names the four non-cache_type
                   inapplicability conditions and says to record
                   not_applicable. That is a description change
                   only; no field was added and the
                   directed_tests enum IN:329-343 is unchanged.
                   GL:136 TEST-1 still tests against cache_type
                   alone. Cannot be CLOSED under the R0 rule
                   against closing on a description.

  S-11  CLOSED     IN:250 invalidate_all "default": false, and
                   IN:248-249 states the tool verifies emission
                   rather than assuming the datapath supports it.
                   Exactly the fix S-11 asked for.

  S-12  PARTIAL    Schema half CLOSED: IN:126-131 pa_bits is an
                   integer input and IN:127 states "This is an
                   input, never a derivation from the size of the
                   next level." Implementations still disagree:
                   cache.v:51 TAG_BITS = 14 hard-wired;
                   gen.cpp:363-364 derives l1_tagBits from
                   gen.cpp:347 log2(mm_capacity), which is 23
                   bits at the options.cpp:183 default of 8MB,
                   giving 5. Nine bits apart, unchanged.

  S-13  CLOSED     IN:286-290 reset_polarity and IN:291-297
                   reset_name. The example's active-high reset
                   named reset (cache.v:45, bitrf.v:21,
                   lrurf.v:21, fsm.v:123) is now expressible as
                   active_high plus "reset". IN:287 states the
                   record-what-was-emitted intent. See R5 row 4
                   for the half of that intent EL cannot honour.

  S-14  PARTIAL    Top level CLOSED: IN:19-21 patternProperties
                   "^x_": true alongside additionalProperties
                   false at IN:18. Nested levels OPEN: IN:87,
                   116, 148, 171, 206, 245, 268 and 317 all still
                   additionalProperties false with no ^x_ escape,
                   while IN:5 claims without qualification that
                   "Any field prefixed x_ is a project
                   extension". geometry.x_foo is still rejected.

  S-15  OPEN       IN:282 is still one of three layouts. See
                   P-18. Substance unchanged.

  S-16  CLOSED     IN:346-357 assertions is an array of
                   $defs/assertion_kind with uniqueItems and a
                   four-value default, replacing the boolean.
                   EL:167-181 checks[] with result enum
                   pass/fail/not_applicable at EL:177 is the
                   recording half and already existed.

  S-17  OPEN       All ten RTL defects verified present this
                   session and outside anything a schema can fix.
                   a compare.v:3 with :20-23 and :15-18,:26;
                   b compare.v:95 against :12; c cache.v:156 and
                   :400 with :76; d top.v:258; e dut.v:56,:58-59
                   with top.v:214-230; f probes.v:181-189 against
                   top.v:234,:269; g mainmemory.v:40;
                   h bitrf.v:32 and lrurf.v:32 against :25,:29;
                   i bitrf.v:14,:35 and lrurf.v:13,:35;
                   j sram.v:25 with :13.

  S-18  OPEN       Every file in examples/1mb_l1/rtl/src is still
                   .v Verilog-2001. IN:271 still describes the
                   pacino profile as always_comb over chained
                   assign and file-scope package imports.
                   verilog_style.md:15-19 unchanged. DW-8 open.

  S-19  CLOSED     Recorded as a non-defect and still accurate.
                   IN:223 refill_width_bits still admits
                   multi-beat; cache.v:253-255 fills in one beat
                   and options.cpp:245 rejects any case other
                   than mm_fetch_size == l1_line_size. The case
                   S-19 anticipated has now arrived: EX:10 and
                   EX:38 give refill_beats = 512/256 = 2, the
                   first multi-beat configuration in the tree.

  M-01  OPEN       Deferred by design. R6 below is the revised
                   and complete replacement for this table.

  M-02  OPEN       EL:3 is still elaborated-1.0.0 and EL:7-8
                   still requires exactly provenance, input,
                   derived, manifest, checks with
                   additionalProperties false. gen.cpp:278-559
                   still writes a flat object. 2.0.0 changed only
                   the input schema, so it could not close this.
                   R3 states what closing it requires.

  M-03  OPEN       options.cpp:321 still reads
                   json["mm_capacity_value"] into mm_capacity.
                   utils.cpp:620-622 still renders 8388608 as
                   value 8 units MB. options.cpp:285 still reads
                   the correct key on the l1 path, so only mm is
                   broken. GL:30-34 unchanged.

  M-04  OPEN       gen.h:16 CacheGen::getValueAndUnits still
                   declared and undefined; utils.h:52 and
                   utils.cpp:613 hold the only definition;
                   utils.cpp:620-622 still integer division;
                   gen.cpp:273,:293 still print it through FLT.

  M-05  OPEN       lrurf.v:39-47, ram.cpp:19-36, compare.v:85-97,
                   mdl.h:71-84 and the fifth copy at
                   ram.cpp:12-17 all still present and still
                   agreeing. EL:82-94 update_table and
                   victim_table still consumed by neither.

  M-06  OPEN       ram.cpp:24-33, mdl.h:73-83,
                   addresspacket.h:55-57, ram.h:97-105,
                   utils.cpp:137,:144,:190,:197 and gen.cpp:437
                   all unchanged. IN:101 still offers
                   1,2,4,8,16 and IN:164 four policies.

  M-07  OPEN       ram.cpp:35,:46,:57; gen.cpp:100-102;
                   top.v:112-114; utils.cpp:475-480. EL:78 still
                   has bits_per_set only and EL:52
                   additionalProperties false blocks a
                   status_bits object.

  M-08  OPEN       ram.h:96 FIXME, :97 mask 0x7, :102 shift 4
                   mask 0xF, :105 shift 8 mask 0xF. Unchanged.

  M-09  OPEN       ram.cpp:19-36 updateLru still calls
                   getLru(idx) (ram.h:97), which does not set q,
                   then writes through q at ram.cpp:35.
                   ram.cpp:41 and :52 still re-find. Ordering
                   dependence at mdl.cpp:67 (after :316),
                   :114-116, :179-180 unchanged.

  M-10  OPEN       ram.cpp:134-138 and :178-182 still dereference
                   q with no end() check; ram.cpp:145 still calls
                   ld_line; mdl.cpp:373 still calls st_line for a
                   line address that need not be present. FIXMEs
                   at ram.cpp:132,:141,:176.

  M-11  OPEN       mdl.cpp:101-104,:112-113,:361,:367 and
                   ram.cpp:202,:206 still print HERE to cout
                   unconditionally.

  M-12  OPEN       mdl_tests.cpp:171 unconditional ++errs with
                   :152-169 commented out; mdl_tests.cpp:670-675
                   basicWrEvictTest stub also ++errs;
                   mdl_main.cpp:10-17 unchanged;
                   mdl_run.cpp:55-68 still adds 1000 per
                   disabled test. mdl still always exits 1.

  M-13  OPEN       mdl_run.cpp:155-231 still validates
                   data_dir/tc_prefix.*; mdl_tests.cpp:15,:294,
                   :411,:531,:614 still open hard-coded
                   ../rtl/data/ paths. opts.data_dir still unused
                   by any test.

  M-14  OPEN       mdl_tests.cpp:421-426 still opens
                   basicWrHit.dN.memh with a literal dN, four
                   times. :570 still reads .data.memh while :116
                   and :344 read .capd.memh.

  M-15  OPEN       mdl.h:112 still declares Ram *mm; with no
                   initialiser against mdl.h:109,:114,:117 which
                   use {nullptr}. mdl.cpp:33 still calls
                   clearResizeMdlArrays, which executes
                   mdl.cpp:244 if(mm) delete mm on an
                   indeterminate pointer.

  M-16  OPEN       options.cpp:253-359 loadFromJson still never
                   reads mm_address_bits and gen.cpp never writes
                   it; the options.cpp:174 default of 32 survives
                   a reload while l1_tagBits came from a solve
                   using log2(mm_capacity). options.cpp:275 still
                   overwrites json_file from the file it opened.

  M-17  OPEN       options.cpp:200-250 checkOpts is still a
                   fixed-point gate. None of GEO-1..4, ADDR-1..2,
                   VIPT-1..2, IF-1..3, MSHR-1, RPL-1, TEST-1,
                   EMIT-1..3 (GL:121-139) is implemented, and
                   nothing writes EL:167-181 checks[].

  M-18  OPEN       options.cpp:220-221 and :233-234 still have
                   the unreachable second return; :225-229 and
                   :231 still call to_upper on
                   l1_replacement_policy twice.

  M-19  OPEN       gen.h:14 process() still undefined; the
                   "+CacheGen::process()" text at gen.cpp:276 is
                   inside createDataSheet. mdl_run.cpp:104-146
                   simulate still commented out returning false.
                   mdl.h:34-35 ldd and std still stubs against
                   IN:210 core widths to 1024. options.cpp:466
                   still prints l1_tagKB, never assigned
                   (options.h:141).

  M-20  OPEN       utils.cpp:630 still (1 << (msb-lsb+1)) - 1
                   with int 1. IN:101 still permits associativity
                   1 and IN:130 pa_bits 56, the case that bites.

  M-21  OPEN       gen.cpp:428 still subtracts 2 from
                   l1_offBits, which gen.cpp:334 already sets to
                   the word count log2(line_size/4). gen.cpp:364
                   subtracts 2 again, correctly, and mdl.h:43
                   reads the same name as a word count.

  M-22  OPEN       mdl.h:45-46 getMMAddr returns a line address;
                   mdl.h:48-52 formMMAddress returns a byte
                   address and is still never called;
                   mdl.cpp:364-366 still builds it inline.

  M-23  OPEN       mdl.cpp:328-344 still scans ways high to low
                   so way3 wins; cache.v:284-291 casez matches
                   5'b0???1 first so way0 wins. Invalid-way
                   selection still agrees (mdl.cpp:381-387,
                   compare.v:74-80) and compare.v:68 still says
                   "the left most invalid way".

  M-24  OPEN       options.h:107 default_mm_entries 262144;
                   options.cpp:183 mm_capacity 8388608;
                   gen.cpp:518 still assigns mm_entries from
                   default_mm_entries without dividing;
                   top.v:14 EXP_MM_ENTRIES 1048576 lines passed
                   at top.v:266. Three sizes, unchanged.

  M-25  OPEN       Summary judgement. No element of its list is
                   closed. pa_bits was already an input in the
                   prior schema per I1:588; IN:126-131 restates
                   it more firmly but changes nothing for the
                   model. mm_fetch_size is still bytes
                   (options.cpp:187) against IN:221-224 bits, the
                   five vocabulary values are unchanged
                   (options.cpp:129,:133,:137,:141,:157), the
                   four out-of-scope fields are unchanged
                   (options.h:75,:77,:81,:82) and gen.cpp:278-559
                   still writes the wrong shape.

Counts: CLOSED 7, PARTIAL 6, WITHDRAWN 1, OPEN 50, SUPERSEDED 0,
UNVERIFIABLE 0. Total 64.

### R1 -- Schema correctness of cgen_config.schema.json 2.0.0

Assessed by reading all 450 lines. The document was not machine
parsed and no validator was run; see Assumption A-2.

#### R1-0  Every allOf branch: trigger, reachability, bypass

Branch 1, IN:363-387. Triggers when cache_type is "icache"
(IN:365). Reachable: cache_type is in the top-level required list
(IN:10) and "icache" is in its enum (IN:74). The policies leg
(IN:370-375) and the interfaces leg (IN:376-381) both fire for
every icache, because policies (IN:14) and interfaces (IN:15) are
themselves top-level required, so properties/policies and
properties/interfaces are never vacuous. The maintenance leg
(IN:382-384) applies only when maintenance is supplied, since
maintenance is optional (absent from IN:7-17). Omitting
maintenance skips the constraint, but omitting it also makes
flush_all unsettable, and IN:260 defaults it false. No
configuration escapes with flush_all true. Constraint reachable,
omission not exploitable.

Branch 2, IN:388-400. Triggers when cache_type is "dcache" or
"unified" (IN:390). Reachable. Both legs fire for the same reason
as branch 1. The "required": ["interfaces"] at IN:394 is dead: the
top level already requires interfaces at IN:15, so this clause can
never change an outcome. Harmless, but it is the one clause in
2.0.0 that looks like a bypass fix and is not one.

Branch 3, IN:401-416. Triggers when addressing.indexing is "VIPT"
(IN:405). Reachable and not bypassable: addressing is top-level
required (IN:13) and indexing is required inside addressing
(IN:115), so the guard at IN:404-406 can never be vacuous. va_bits
and page_bytes are then required (IN:413).

Branch 4, IN:417-432. Triggers when miss_handling is present and
mshrs is at least 1 (IN:420-423). miss_handling is optional, so
this is the one branch whose guard can be vacuous. Omitting
miss_handling, or supplying it without mshrs, makes the if false;
in both cases mshrs takes its IN:178 default of 0, for which
mshr_targets is correctly not required. Constraint reachable,
omission not exploitable.

Branch 5, IN:433-448. Triggers when emission.sram_model is "macro"
(IN:437). Reachable: emission is top-level required (IN:16).
Omitting sram_model defaults it to "inferred" (IN:301), for which
macro_name is correctly not required. Omission not exploitable.
The requirement is satisfiable vacuously, though: see R1-4.

Result. The Hypothesis's mechanical question is answered no. No
subschema placed under "properties" in 2.0.0 constrains an
optional object in a way that omitting the object admits an
illegal configuration. Every such conditional either guards an
object the top level already requires (branches 1, 2, 3, 5) or
guards one whose absence removes the field being constrained
(branch 4). This is a negative result and it is recorded as a
deliverable.

#### R1-1  Inapplicable fields are forbidden in one branch of four

This is the largest correctness finding in R1.

GL:74-80 states the principle: the schema "rejects write_hit,
write_miss, core_write_width_bits and flush_all on an icache
rather than accepting and ignoring them. A parameter that is
accepted and ignored is how a config comes to mean something other
than what it says."

Branch 1 applies that principle, using not/anyOf/required at
IN:371-374 and IN:377-380. The other three conditionals require
the applicable field and do not forbid the inapplicable one. All
three of the following validate against 2.0.0 today:

  addressing: indexing "PIPT" with va_bits 39, page_bytes 4096
    and alias_policy "forbid". Branch 3 (IN:411-414) only adds
    requirements on the VIPT side. Three fields that describe a
    virtual index on a design that has none.

  miss_handling: mshrs 0 with mshr_targets 8. Branch 4
    (IN:427-430) only adds a requirement when mshrs is at least 1.
    IN:181 says "1 means no merging" and IN:174 says mshrs 0
    "emits a blocking FSM: one miss outstanding". A merge count on
    a machine with nothing to merge.

  emission: sram_model "inferred" with macro_name "foo". Branch 5
    (IN:443-446) only adds a requirement when sram_model is
    "macro". IN:304 says macro_name is "Required when sram_model
    is macro"; nothing says it is forbidden otherwise.

What a fix must achieve: each of the three conditionals needs the
mirror of what branch 1 already does, so that the inapplicable
field is rejected rather than accepted and dropped. The pattern is
already in the file at IN:371-374 and needs no new construct.

#### R1-2  schema_version cannot reject the version it replaced

IN:53-57 types schema_version as a string matching
^[0-9]+\.[0-9]+\.[0-9]+$ and IN:54 states "Major bump means
breaking. 2.0.0 removed policies.critical_word_first and retyped
verification.assertions."

The pattern accepts "1.0.0", "0.0.1" and "99.99.99" equally. A
file that declares itself as targeting the schema this one
replaced validates against this one without complaint, provided
its body happens to fit. Background decision 1 rests entirely on
the major bump being meaningful, and the schema provides no
mechanism to act on it.

Consequence, stated concretely: EX:2 declares "1.0.0" and this
field is the one thing in that file that passes. The prompt's
expectation that "Every instance in the tree is expected to fail
on schema_version alone" is not met. See R4.

What a fix must achieve: the field has to be able to reject a
version this document cannot honour. A const, or a pattern
anchored to the major digit, is enough; the choice between them is
whether minor versions are meant to validate against a later
schema.

#### R1-3  The x_ extension escape exists at one level of nine

IN:5 states, unqualified, "Any field prefixed x_ is a project
extension and is not interpreted by the tool." IN:18-21 implements
that at the top level: additionalProperties false alongside
patternProperties "^x_": true. In 2020-12 additionalProperties
applies only to properties matched by neither properties nor
patternProperties, so the escape works as written.

It exists nowhere else. additionalProperties false with no ^x_
escape appears at IN:87 (geometry), 116 (addressing), 148
(policies), 171 (miss_handling), 206 (interfaces), 245
(maintenance), 268 (emission) and 317 (verification). x_note at
the top level validates; geometry.x_note does not.

This is the S-14 half that is still open, and it is also a case of
a description claiming more than the schema does.

#### R1-4  Two required-when fields are satisfiable vacuously

IN:303-306 macro_name is "type": "string" with no minLength and no
pattern. Branch 5 requires it when sram_model is "macro"
(IN:445). "macro_name": "" satisfies that requirement and names no
cell. Compare IN:59-64 name and IN:291-297 reset_name, both of
which carry a pattern that forces a first character.

IN:281-285 output_dir is the same shape, "type": "string" with
default ".". It is not conditionally required, so the consequence
is smaller, but an empty output_dir is equally meaningless.

#### R1-5  Two single-value enums, one of them required

IN:132-136 page_bytes is enum [4096] with default 4096. IN:137-141
alias_policy is enum ["forbid"] with default "forbid". Neither can
carry information: there is exactly one legal value and a default
supplying it.

page_bytes is the one that costs something. Branch 3 (IN:413)
requires it for every VIPT configuration, so a VIPT author must
write out the only value the field can hold. EX:19 does exactly
that. alias_policy is not required and EX:20 states it anyway.

This is defensible as forward compatibility and IN:133-134 and
IN:138-140 both explain the intent. Recorded because the two
fields differ in whether they are required and nothing explains
why, and because a required field with one legal value is the kind
of ceremony a generator prompt will get wrong.

#### R1-6  Golden data has two mechanisms and no precedence

New in 2.0.0 and not previously reported.

IN:33 puts "golden" in $defs.artifact_kind and IN:275 describes it
as "the expected end-state capture", adding that preload and
golden "are separate artifacts in separate directories and
requesting one does not imply the other". IN:319-323 keeps
verification.golden_vectors, a boolean defaulting true, described
as "Produce reference array images and an expected capture stream
from the C++ model".

Two fields now decide one thing. Nothing states which wins. All
four combinations validate, and two of them are contradictory:
golden in emit with golden_vectors false, and golden absent from
emit with golden_vectors true. IN:275 goes out of its way to state
that preload does not imply golden and says nothing about the
boolean that also asks for golden.

GL:138 EMIT-2 checks "tb in emit implies vectors in emit", using
the value 2.0.0 deleted, so the tool check that would have
adjudicated this now names a nonexistent artifact kind.

What a fix must achieve: one of the two must go, or the boolean
must be redefined as something other than a request for the
golden artifact.

#### R1-7  The clock port name is now stated nowhere

New in 2.0.0 and not previously reported.

I1:601-602 records that the prior file said the pacino profile
applies "clk/rstn". IN:271 describes the profile as "80 columns,
2-space indent, one module per file, file-scope package imports
with defines before structs, always_comb over chained assign,
ASCII comments" and closes with "Reset naming and polarity are
stated separately below rather than implied by the profile."

Reset was promoted out of the profile into IN:286-297. The clock
went with it and landed nowhere. No field in 2.0.0 names the clock
port, and the profile description no longer mentions it.

GL:99-105 still says pacino applies "clk/rstn". So the schema and
the glossary now disagree about the content of the same named
profile, and the disagreement was created by the edit that fixed
S-13.

This is an unexplained difference in a description field with a
consequence, which the Background asked to be reported as a
finding. What a fix must achieve: either restore the clock to the
profile description or give it a field beside reset_name, and
reconcile GL:99-105 either way.

#### R1-8  assertions defaults to all, directed_tests defaults to none

IN:346-357 gives assertions an explicit four-value default.
IN:324-345 gives directed_tests no default and no minItems. IN:5
states that "The elaborate stage materialises every default into
the elaborated output".

So omitting assertions yields all four assertions and omitting
directed_tests yields no tests, silently, from two adjacent fields
of the same group with the same array shape. Nothing states the
asymmetry. A config that omits both, with golden_vectors at its
IN:322 default of true, asks for golden data for zero tests.

#### R1-9  Ranges and enums: contradictions and dead values

No two fields have ranges that contradict outright. Three
observations that R1 asks for and that are worth the row:

  Asymmetric core widths. IN:210 core_read_width_bits reaches
  1024; IN:214 core_write_width_bits stops at 256. Defensible for
  an icache fetch bundle against a store, which IN:209 says
  explicitly, but cache_type "unified" (IN:74) selects the same
  asymmetry for an L2 where it has no such justification. I cannot
  attribute this to 2.0.0; I1 does not record the prior write
  enum. Recorded as unattributed.

  No dead enum values within IN. Every value of artifact_kind
  (IN:26-37), assertion_kind (IN:42-47), directed_tests
  (IN:329-343), the geometry enums and the interface enums is
  reachable for some legal configuration. refill_width_bits 256
  and above is unreachable with line_bytes 16 under GL:132 IF-2,
  but reachable with larger lines, so it is not dead.

  Three artifact_kind values have no consumer outside this file.
  preload (IN:32), golden (IN:33) and probes (IN:34) can be
  requested and cannot be recorded, because EL:155 does not carry
  them. That is R5 row 1, not a defect internal to IN.

  Level against scope. IN:81 permits level 3. GL:167 states the
  in-scope set as "L1 instruction, L1 data, and L2 unified
  caches". Level 3 is outside the scope the glossary declares.
  This bears directly on Background decision 2; see R1-10.

#### R1-10  The four drafter decisions

Decision 1, the major bump to 2.0.0. Correct. Removing
policies.critical_word_first and retyping verification.assertions
from boolean to array both break every existing instance, as EX
demonstrates twice. The judgement is right and the consequence is
that it is unenforceable: see R1-2. Recommend keeping the bump and
adding the mechanism.

Decision 2, capacity_bytes maximum raised to 67108864 rather than
capping level at 2. The change is defensible; the stated reason is
not. capacity_bytes (IN:89-93) and level (IN:77-82) are
independent fields with no conditional between them anywhere in
IN:362-449, so level 3 was expressible at the old 8MB ceiling and
would remain expressible if the ceiling were lowered again.
Raising capacity does not make level 3 expressible; nothing ever
made it inexpressible. Separately, both halves of the reasoning
point at a level GL:167 puts out of scope. Consequence: the
justification will not survive review, and the same edit should be
argued from the L3 capacity point directly, or GL:167 should be
widened first. The number itself I would keep.

Decision 3, VOCABULARY.md changed to GLOSSARY.md at both sites.
Correct and verified: IN:5 and IN:138 both now name GLOSSARY.md,
and planning/GLOSSARY.md exists. This closes P-05. It leaves the
naming inconsistency one-directional: GL:15-16 still names
cache_config.schema.json and cache_elaborated.schema.json (P-04)
and PC:283-284 names a third pair (P-03), so the schema now points
correctly at a document that points incorrectly back.

Decision 4, filename left as cgen_config.schema.json. Correct. It
matches the file on disk, the INFRA-001 manifest and this task's
manifest. Renaming would have invalidated both prior manifests to
satisfy two documents that are wrong for other reasons. The two
documents are the things to fix.

### R2 -- RTL expressibility against 2.0.0

Scope note. S-05 next-level model, S-06 hit timing and S-07
per-array storage kind are the deferred set named in Background
and are not counted here. The design as read is unchanged from
I1:443-465 and is not re-derived.

#### R2-A  What 2.0.0 now expresses that the prior schema did not

Negative results, recorded as deliverables.

  Byte-enable width, core side. IN:216-220
  write_granularity_bytes with value 1 gives cache.v:30 be [3:0]
  as core_write_width_bits 32 / 8 / 1 = 4. The input decision now
  has a field. The array-side width is still missing; see R2-B1.

  Fill-bus bypass. IN:235-239 fill_bypass_to_core with value true
  states what cache.v:310-321 does: the frd path selected by
  fsm_cc_rd_fill returns the requested word from mm_readdata_q
  rather than waiting on the array write. IN:230-234
  refill_beat_order is separately not_applicable for this design,
  because 32 * 8 / 256 gives one beat (cache.v:253-255). The S-04
  split does exactly the job it was added for, on this design.

  Reset. IN:286-290 active_high plus IN:291-297 reset_name
  "reset" expresses cache.v:45, bitrf.v:21, lrurf.v:21 and
  fsm.v:123. The example no longer contradicts the schema on
  reset; it contradicts only GL:100, which still binds clk/rstn to
  the pacino profile. See R1-7.

  Maintenance. IN:250 invalidate_all default false now matches the
  RTL, which declares pe_flush, pe_flush_all, pe_invalidate and
  pe_invalidate_all at cache.v:93-96 and drives none of them, and
  whose INVAL_ALL state is decoded at probes.v:33 and entered
  nowhere in fsm.v.

  Probes as a request. IN:34 lets emission.emit ask for probes.v.

  Assertions. IN:346-357 as an array lets the design ask for
  one_hot_way_hit alone and record the other three
  not_applicable, which is what mshrs 0 requires.

#### R2-B  Missing fields that remain

R2-B1  Offset units and the word-offset sub-field. S-02, open.
  IN:96 says line_bytes "Sets the offset field width" and never
  states whether offset counts bytes or words. The RTL needs both:
  a 3-bit word offset at cache.v:110 driving the read and write
  muxes at cache.v:201-221 and cache.v:298-319, and a 2-bit byte
  position only the byte enables see. EL:69 emits one offset field
  and EL:191-203 defines it without a unit. The word count is
  derivable, log2(line_bytes / (core_read_width_bits / 8)) = 3
  from IN:97 and IN:210, but no field states it and EL:52
  additionalProperties false means a generator cannot add one to
  derived without changing the elaborated schema.

R2-B2  Byte-enable width, array side. S-03 remainder. IN:217
  computes the core-side width. dsram.v:14 takes be [31:0], one
  bit per byte of the 256-bit line, and cache.v:212-221 positions
  the 4-bit core be into that 32-bit field. The array-side width
  is line_bytes / write_granularity_bytes = 32. Neither schema
  carries it and EL:52 blocks adding it.

R2-B3  Recording the probes artifact. S-08 remainder. IN:34 can
  request it; EL:155 cannot record it. Same for preload and
  golden. This is R5 row 1.

#### R2-C  Extra fields for this design

  IN:230-234 refill_beat_order. The design has one refill beat, so
  the field is inert. IN:231 handles this correctly by saying to
  record it not_applicable rather than reject, which is right and
  is the treatment GL:133 IF-3 describes for a field that no
  longer exists. Recorded as extra-but-correctly-handled, not as a
  defect.

  IN:103-109 banks. The RTL instantiates four dsram, one per way
  (cache.v:491-529), not per bank. banks 1 expresses the design;
  the field is simply unused here.

  IN:303-306 macro_name and IN:307-311 verification_params are
  unused. verification_params is worth a note: IN:308 promises
  that setting it "leaves a small number of parameters live in the
  emitted RTL", which is the direct opposite of what compare.v:3
  and :20-23 do to the WIDTH parameter (S-17a). Not a schema
  defect; recorded because a generator built from IN:308 must not
  reproduce compare.v.

#### R2-D  Ambiguities that remain

R2-D1  core_read_width_bits against the return path. IN:209 says
  "Width returned to the core per access." cache.v:23 declares rd
  as [31:0] and cache.v:298-321 always returns a full 32-bit word
  regardless of be, which fsm.v:13 states as policy: "Reads ignore
  byte enables and return the whole word." Nothing in IN says
  whether the returned width is a fixed bundle or the width of the
  requested access. For this design the two coincide; for a
  512-bit icache fetch bundle against a 32-bit instruction they do
  not. IN:209's second sentence gestures at this for icache and
  leaves dcache unstated.

R2-D2  Which array sram_model applies to. This is S-07 and is
  deferred, but the ambiguity that is not deferred is that IN:299
  describes sram_model as "How array storage is emitted", singular,
  while the design emits three storage shapes: sram.v for tags
  (address-sensitive read, sram.v:25), dsram.v for data
  (registered read qualified by read_q, dsram.v:53-55) and
  bitrf.v/lrurf.v for status (flop file with a synchronous
  clear-all, bitrf.v:47-49). Recorded only to note that the
  wording still reads as though there were one array.

Answer to R2. Excluding the deferred set, three fields are
missing (R2-B1, B2, B3), one field is extra and inert (R2-C
refill_beat_order) and two ambiguities remain (R2-D1, D2). The
geometry, addressing, policies, miss_handling and maintenance
groups now express this design completely.

### R3 -- Model expressibility against both schemas

Scope note. M-01 and the write-buffer field implied by
l1_store_buffer_size are the deferred set and are not counted.
R6 is the complete replacement for the M-01 table.

#### R3-A  What 2.0.0 changes for the model

Two things, both small.

  l1_critical_word_first (options.h:80, default true at
  options.cpp:161, hard-required true at options.cpp:219) now maps
  to interfaces.fill_bypass_to_core (IN:235-239) rather than to a
  policies field. That is a rename across groups, and it is the
  only mapping in R6 that moves a field from one group to another.
  Any regeneration that walks the schema group by group will get
  this one wrong if it is not called out.

  Byte granularity now has a home. Utils::stBytes
  (utils.cpp:654-677) and Ram::st (ram.cpp:142-174) both operate
  on four fixed byte lanes, that is write_granularity_bytes 1
  (IN:216-220). The value was previously inexpressible.

Nothing else. The rest of the model's distance from the input
schema is unchanged: mm_fetch_size is still bytes
(options.cpp:187) against IN:221-224 bits; the vocabulary is still
uppercase and hyphenated (options.cpp:129, :133, :137, :141,
:157) against IN's lower snake case; l1_coherency_protocol
(options.h:75) is still a field GL:177-179 forbids by name;
l1_mmu_present and l1_mpu_present (options.h:81-82) still have no
counterpart; and l1_tag_type "PHYSICAL" (options.cpp:157) is still
tag physicality where IN:118-121 indexing is index source.

#### R3-B  Can the emitted JSON be made to satisfy EL, and how

Today, no. gen.cpp:278-559 writes a flat object whose keys are
json_format_version, l1_*, mm_*, tc_prefix, data_dir, cmd_file,
bits_file, tags_file, mm_file and dary_files. EL:7-8 requires
exactly provenance, input, derived, manifest and checks with
additionalProperties false. The intersection is empty.

It can be made to satisfy EL. Six things are required, listed in
increasing cost. 2.0.0 changes none of them, because it did not
touch EL; EL:3 is still elaborated-1.0.0.

R3-B1  derived, address fields. EL:51 requires sets, offset, index
  and tag; EL:191-203 requires bits, lsb, msb, mask and shift on
  each. The model already computes all five for each of tag, set
  and off: gen.cpp:363-369, :387-390 and :400-403, plus l1_sets at
  gen.cpp:439. Two conversions are needed. EL:200 types mask as a
  string matching ^0x[0-9a-f]+$ and gen.cpp:374, :395 and :408
  write it as a JSON integer, so a render step is required. And
  offset.bits is ambiguous: gen.cpp:334 sets l1_offBits to
  log2(line_size / 4), a word count of 3, while a byte reading of
  EL:69 wants 5. This is R2-B1 arriving on the model side and it
  must be settled before the field can be filled in at all.

R3-B2  derived, the rest. lines_total, bytes_per_way,
  data_array_bits, tag_array_bits, overhead_bits and
  overhead_percent (EL:56-67) are all computable from what
  gen.cpp:333-338 already has and none is computed today.
  refill_beats (EL:98) is 1 for this model by construction, since
  options.cpp:245 rejects anything else. replacement_state
  (EL:73-95) is the M-05 table and the model holds it as code at
  ram.cpp:19-36 and mdl.h:73-83 rather than as data.

R3-B3  provenance. EL:15 requires tool_version, input_sha256,
  generated_utc and elaborated_schema_version. The model has none
  of the four. VERSION exists but is only printed
  (options.cpp:375) and never written to JSON. There is no digest
  anywhere in the manifest and no timestamp. This is new code, not
  a remapping.

R3-B4  manifest. EL:150 requires path, kind and sha256 per entry.
  gen.cpp:549-551 writes three bare path strings and
  gen.cpp:553-558 writes dary_files as an array of {name}
  objects. No digest, no size, no kind. Regeneration cannot diff,
  which is what EL:146 says the manifest is for.

  This one is blocked, not merely unimplemented. The four
  artifacts the model produces are preload images: bits.memb,
  tags.memh, dN.memh and mm.memh, written at gen.cpp:83, :180,
  :138 and :229 and described at gen.cpp:76-78 and :122 as the
  cold-start contents of the arrays. IN:32 calls that kind
  "preload". EL:155 has no preload value. The model's own output
  cannot be classified under 2.0.0's vocabulary. Closing R3-B4
  requires R5 row 1 to be closed first.

R3-B5  checks. EL:167-181 wants every constraint the input schema
  cannot express, recorded pass, fail or not_applicable.
  options.cpp:200-250 is a fixed-point gate rejecting anything
  that is not the one shipped geometry, and implements none of the
  GL:121-139 checks. M-17, unchanged.

R3-B6  input. EL:44 wants the input verbatim after defaults. The
  model does not retain it; options.cpp:275 overwrites json_file
  with a value read out of the file it just opened, so even the
  input path does not survive. EL:45 types input as a bare object
  with no restriction, so the model's existing l1_* keys would
  validate there mechanically. They should not go there: EL:44
  says input, and an input is a 2.0.0 document.

Answer to R3. Yes, the emitted JSON can be made to satisfy EL.
Five of the six requirements are ordinary work on data the model
already has or can compute. The sixth, R3-B4, is blocked on the
cross-schema break in R5 row 1. The offset-unit question in R3-B1
must be decided before any of it starts, because it changes a
value rather than a name.

### R4 -- Instance validation of misc/example_pacino_icache.json

Field by field against 2.0.0. Every key in the file is examined.
Result: three failures, and schema_version is not one of them.

  FIELD                          LINE  RESULT
  -----------------------------  ----  ---------------------------
  schema_version "1.0.0"         EX:2  PASSES. IN:56 pattern
                                       ^[0-9]+\.[0-9]+\.[0-9]+$
                                       matches. See R1-2.
  name "icache"                  EX:3  PASS. IN:62 pattern, IN:63
                                       maxLength 32.
  description                    EX:4  PASS. 89 chars, IN:69
                                       maxLength 200.
  cache_type "icache"            EX:5  PASS. IN:74. Fires allOf
                                       branch 1 (IN:363).
  level 1                        EX:6  PASS. IN:80-81 [1,3].
  geometry.capacity_bytes 32768  EX:9  PASS. IN:92-93
                                       [256, 67108864].
  geometry.line_bytes 64         EX:10 PASS. IN:97.
  geometry.associativity 8       EX:11 PASS. IN:101.
  geometry.banks 2               EX:12 PASS. IN:106-107 [1,8].
  addressing.indexing "VIPT"     EX:16 PASS. IN:120. Fires allOf
                                       branch 3 (IN:401).
  addressing.va_bits 39          EX:17 PASS. IN:124. Required by
                                       branch 3, present.
  addressing.pa_bits 56          EX:18 PASS. IN:129-130 [32,56],
                                       at the maximum.
  addressing.page_bytes 4096     EX:19 PASS. IN:134. Required by
                                       branch 3, present.
  addressing.alias_policy        EX:20 PASS. IN:139.
  policies.read_miss "allocate"  EX:24 PASS. IN:152.
  policies.replacement           EX:25 PASS. IN:164 tree_plru.
  policies.critical_word_first   EX:26 FAIL 1. policies has
                                       additionalProperties false
                                       at IN:148 and 2.0.0 removed
                                       the field; IN:149-165 has
                                       only read_miss, write_miss,
                                       write_hit, replacement.
  miss_handling.mshrs 4          EX:30 PASS. IN:176-177 [0,16].
                                       Fires allOf branch 4.
  miss_handling.mshr_targets 2   EX:31 PASS. IN:183-184 [1,8].
                                       Required by branch 4,
                                       present.
  miss_handling.victim_buffer_.. EX:32 PASS. IN:189-190 [0,16].
  miss_handling.fill_buffer_..   EX:33 PASS. IN:196-197 [0,8].
  interfaces.core_read_width 512 EX:37 PASS. IN:210.
  interfaces.refill_width 256    EX:38 PASS. IN:223.
  interfaces.next_level_name     EX:39 PASS. IN:227-228.
  interfaces, branch 1 check     EX:36 PASS. IN:377-380 forbids
                                       core_write_width_bits and
                                       write_granularity_bytes on
                                       an icache; neither present.
  maintenance.invalidate_all     EX:43 PASS. IN:249 boolean.
  maintenance.invalidate_line    EX:44 PASS. IN:254.
  maintenance.flush_all false    EX:45 PASS. IN:383 requires
                                       const false on an icache;
                                       it is false.
  emission.profile "pacino"      EX:49 PASS. IN:272.
  emission.emit "vectors"        EX:50 FAIL 2. IN:26-37
                                       artifact_kind has no
                                       "vectors". The other seven
                                       values in the array all
                                       pass; IN:278 uniqueItems is
                                       satisfied.
  emission.output_dir            EX:51 PASS. IN:283 string.
  emission.sram_model "bw_ram"   EX:52 PASS. IN:300. Branch 5
                                       (IN:437) does not fire, so
                                       macro_name is not required.
  emission.verification_params   EX:53 PASS. IN:310 boolean.
  verification.golden_vectors    EX:57 PASS. IN:321 boolean.
  verification.assertions true   EX:58 FAIL 3. IN:348 types this
                                       as an array. A boolean
                                       fails the type check.
  verification.directed_tests    EX:59-  PASS. All eight values are
                                 68     in the IN:329-343 enum and
                                        all eight are distinct.

Required-field check. IN:7-17 requires schema_version, name,
cache_type, level, geometry, addressing, policies, interfaces and
emission. All nine are present. The group-level required lists at
IN:86, 115, 147, 205 and 267 are all satisfied. No unknown key
appears at the top level, so IN:18 additionalProperties false is
not triggered there.

Fields 2.0.0 removed, and what replaces them.

  policies.critical_word_first replaced by two fields, because
  I1:495-506 established that it conflated two decisions. For this
  instance the intent is stated at EX:4 and by the mshr and
  fill-buffer settings: it wants the requested word returned
  during the fill. That is interfaces.fill_bypass_to_core
  (IN:235-239), whose IN:238 default is already true. Beat
  ordering is a separate decision and here it is live, not
  not_applicable, because line_bytes 64 against refill_width_bits
  256 gives two beats; interfaces.refill_beat_order (IN:230-234)
  defaults to critical_first, which is what this design wants. So
  both replacements are satisfied by their defaults and the
  minimum edit adds nothing.

  emission.emit "vectors" replaced by "preload" and "golden"
  (IN:32-33), which IN:275 states are separate artifacts and that
  requesting one does not imply the other. This instance has "tb"
  in emit and golden_vectors true at EX:57, so the value it means
  is "golden".

  verification.assertions boolean replaced by an array of
  $defs/assertion_kind (IN:346-357). true meant all of them, and
  IN:351-356 makes all four the default.

Minimum edit that makes the file valid. Three edits, all deletion
or substitution; nothing has to be added.

  1. Delete line EX:26, "critical_word_first": true, from
     policies. Both replacement fields default to the values this
     instance wants, so no compensating addition is required.
  2. On EX:50, change "vectors" to "golden".
  3. Delete line EX:58, "assertions": true. IN:351-356 supplies
     all four assertions by default. Writing the explicit
     four-element array is equivalent and more legible; either
     satisfies the schema.

Optionally, EX:2 should read "2.0.0" to say what it now targets.
That edit is not required to validate, which is the finding in
R1-2.

Semantic notes, not validation results. Recorded because they
bear on whether the edited file is a good instance as well as a
legal one.

  The GL:121-139 tool checks all pass for this geometry.
  bytes_per_way is 32768 / 8 = 4096, exactly page_bytes, so VIPT-1
  passes with zero headroom, which is what EL:128 describes and
  what EX:4 says the design is for. GEO-2 gives 64 sets, GEO-4
  banks 2 divides 64, ADDR-1 gives 6 + 6 + 44 = 56, IF-1 has
  core_read_width_bits 512 equal to line_bytes * 8, IF-2 has 512
  a multiple of 256, MSHR-1 has 4 well under 512, RPL-1 has
  associativity 8 a power of two.

  All four of the conditions S-10 named hold favourably here:
  mshrs is 4 so mshr_merge and mshr_full apply, refill_beats is 2
  so refill_beat_order applies, and victim_hit is correctly not
  requested since victim_buffer_entries is 0. This instance is the
  one configuration in the tree for which GL:136 TEST-1 as written
  would have been adequate.

  misc/example_pacino_icache.elaborated.json exists in the tree
  (verified by listing) and is not in this task's manifest, so it
  was not read and no claim is made about it.

### R5 -- Cross-schema consistency

Every construct that appears in both schema files and must agree.
Twelve rows. Each states the verdict and gives both citations.

R5-1  Artifact kinds.  DOES NOT AGREE.
  IN:24-38 $defs/artifact_kind, ten values: pkg, rtl, tb,
  makefile, model, preload, golden, probes, datasheet, docs,
  referenced by emission.emit at IN:279.
  EL:154-156 manifest[].kind, eight values: pkg, rtl, tb,
  makefile, model, vectors, datasheet, docs.
  preload, golden and probes can be requested and cannot be
  recorded. vectors can be recorded and cannot be requested.
  IN:25 asserts the two "must not diverge; they are one definition
  referenced twice", which is false across files: the $ref at
  IN:279 resolves inside IN only. This is the KNOWN BREAKAGE, and
  it blocks R3-B4 and S-08 and S-09.

R5-2  Check identifiers.  DOES NOT AGREE.
  IN:329-343 directed_tests is a closed thirteen-value enum and
  IN:40-48 $defs/assertion_kind is a closed four-value enum,
  referenced at IN:350.
  EL:175 checks[].id is "type": "string", unconstrained.
  IN:325 and IN:347 both state that an inapplicable member "is
  recorded not_applicable in the elaborated checks array", so the
  two files are explicitly coupled and only one side enumerates.
  A misspelled id is unrecordable as wrong, and there is no
  enumerated id for the GL:121-139 checks either. This is the same
  defect class as R5-1 one level down, and it is the second
  substantive divergence the prompt asked to be found.

R5-3  Input schema version.  DOES NOT AGREE.
  IN:53-57 schema_version is a string with a semver pattern.
  EL:20 provenance.input_schema_version is a bare string with no
  pattern; EL:21 elaborated_schema_version is the same.
  The elaborated record of the input version is weaker than the
  input's own field, so a value the input schema would reject is
  recordable as provenance. Combined with R1-2, neither file can
  reject a version mismatch in either direction.

R5-4  Reset polarity and name.  DOES NOT AGREE.
  IN:286-297 reset_polarity and reset_name, with IN:287 stating
  the purpose: "Recorded here so the elaborated manifest states
  what was emitted rather than what a style profile implies."
  EL:145-165 manifest[] records path, kind, sha256, bytes and
  module and nothing about reset.
  The claim is satisfied only by EL:43-46 input carrying the field
  verbatim, which is not the manifest. A field added in 2.0.0
  makes a promise about a file 2.0.0 did not change. This is an
  unexplained description difference with a consequence.

R5-5  Byte-enable width.  DOES NOT AGREE.
  IN:216-220 write_granularity_bytes, with IN:217 naming the
  derivation: "Sets the byte enable width: core_write_width_bits /
  8 / this value."
  EL:53-141 derived has no byte_enable_bits, and EL:52
  additionalProperties false forbids adding one without a schema
  change. The input schema names a derived quantity the elaborated
  schema has nowhere to put. Same shape as R5-4 and also new in
  2.0.0. This is the open half of S-03.

R5-6  Address field decomposition.  AGREES.
  IN:126-131 pa_bits, IN:95-98 line_bytes, IN:99-102
  associativity.
  EL:69-71 derived.offset, index, tag, each EL:191-203 with bits,
  lsb, msb, mask and shift.
  Vocabulary and structure are consistent. The arithmetic tie is
  GL:127 ADDR-1 and lives in the tool by design, which GL:113-119
  states. The one unresolved question is the unit of offset, which
  is R2-B1 and is a gap in both files rather than a disagreement
  between them.

R5-7  VIPT group.  AGREES, with a reachability gap.
  IN:118-141 indexing, va_bits, page_bytes, alias_policy.
  EL:117-136 derived.vipt, whose EL:132 names alias_policy forbid
  directly and whose EL:118 says the object is "Present only when
  indexing is VIPT".
  The vocabulary agrees. Nothing enforces the presence rule: EL:51
  requires only sets, offset, index and tag, and there is no
  conditional in EL making vipt required when input.indexing is
  VIPT. A VIPT elaboration that omits derived.vipt validates.

R5-8  MSHR group.  AGREES, with the same reachability gap.
  IN:169-201 mshrs and mshr_targets.
  EL:104-115 derived.mshr, whose EL:111 says blocking is "True
  when mshrs is 0".
  Vocabulary agrees. derived.mshr is optional with no conditional,
  so a non-blocking elaboration that omits it validates.

R5-9  Replacement state.  AGREES, with an internal inconsistency.
  IN:162-165 replacement, four values.
  EL:73-95 derived.replacement_state.
  EL:79-80 marks tree_depth "Absent for other policies". Its two
  siblings update_table (EL:82-89) and victim_table (EL:90-94) are
  described at EL:83 and EL:91 as generated "from the tree
  construction", so they are equally tree_plru specific, and
  neither is marked absent for lru, random or fifo. Three sibling
  fields with the same applicability and one of them documented.

R5-10  Refill beats.  AGREES.
  IN:221-224 refill_width_bits and IN:95-98 line_bytes.
  EL:98-101 refill_beats, stating the formula "line_bytes * 8 /
  refill_width_bits", and EL:102 beat_index_bits.
  Consistent. Note that IN:231 tells the tool to record
  refill_beat_order not_applicable when there is one beat, and the
  place to record it is EL:167-181 checks[], which under R5-2 has
  no enumerated id for it and whose GL:133 name IF-3 still refers
  to critical_word_first.

R5-11  Instance name and module name.  AGREES.
  IN:59-64 name, with IN:60 stating it drives <name>.sv,
  <name>_pkg.sv and tb_<name>.sv.
  EL:159-162 manifest[].module, "SystemVerilog module or package
  name, when the file declares one".
  Consistent, though the coupling is convention only: nothing
  requires manifest[].module to derive from input.name.

R5-12  Output location.  UNSPECIFIED, so neither agrees nor
  conflicts.
  IN:281-285 output_dir with IN:282 naming rtl/, tb/ and tests/.
  EL:153 manifest[].path, a bare string.
  Nothing states whether manifest paths are absolute, relative to
  the working directory, or relative to output_dir. A consumer
  cannot resolve a manifest path without an assumption. See P-18
  for the three competing layouts this sits inside.

#### Recommended mechanism

Rows 1 and 2 are the ones that need a mechanism; rows 3, 4 and 5
need edits to EL and rows 7, 8 and 9 need edits within EL.

The project already has the argument for the mechanism and it is
GL:145-157, "Generated, not transcribed", written about the PLRU
tables. The identical reasoning applies here one level up, and
I1:1301-1305 N-3 said so about the emit and kind enums before
2.0.0 turned the informal duplication into an explicit and false
claim of unity at IN:25.

The recommendation: hold artifact_kind, assertion_kind, the
directed_tests enum and the GL:121-139 check ids in one source of
truth outside both schema files, and emit both schema documents
from it. Add a verifier that regenerates and compares, and fails
when a checked-in file differs. That keeps the enums in step
without requiring $ref to cross files, it matches the treatment
the project already applies to the replacement tables, and it
gives the check ids of R5-2 an enumerated home that both files can
be generated against.

One caveat on the premise, offered as information and not as a
recommendation. The prompt states that $ref does not cross files.
That is true of a bare relative reference with no schema registry.
The 2020-12 specification resolves $ref by URI against the
resources a validator has been given, and both files carry
absolute $id values (IN:3, EL:3), so a validator handed both
documents as one resource set can resolve a cross-document $ref
between them. I did not test this in this session and no
validator was run, so it is recorded as worth confirming before
the generator above is built, not as a finding.

### R6 -- Field mapping table

One row per field of 2.0.0, then every Options member with no
2.0.0 field. This is the input to the options regeneration work.

Legend for STATUS.
  OK      same meaning, same units; the name differs only by the
          l1_ or mm_ prefix convention.
  RENAME  present, but under a different identifier, in a
          different group, or in different units.
  ABSENT  a 2.0.0 field with no Options member carrying the same
          decision. A near-miss member is named where one exists.
  EXTRA   an Options member with no 2.0.0 field.

Convention for the elaborated column. EL:43-46 carries the whole
input verbatim, so every input field trivially has an
input.<same path> home. That is not useful, so the column names
the derived, provenance or manifest path that consumes or records
the field, and reads "input only" when the verbatim copy is the
only home. Options members are cited to options.h unless the
value or default is what matters, in which case options.cpp.

#### R6-A  2.0.0 fields, 43 of them plus 2 $defs

  schema_version                                        RENAME
    elab  provenance.input_schema_version (EL:20)
    opts  json_format_version, options.h:61. Value "0.0.1" is
          not the schema version and is not read as one.

  name                                                  ABSENT
    elab  manifest[].module (EL:159)
    opts  near-miss tc_prefix, options.h:100. That is a file
          prefix defaulting to "tc_" (options.cpp:108), not a
          SystemVerilog identifier. IN:60 gives name a different
          job.

  description                                           ABSENT
    elab  input only
    opts  none.

  cache_type                                            ABSENT
    elab  input only
    opts  none. The model is unconditionally a write-back
          write-allocate dcache; options.cpp:207-247 rejects
          every other shape.

  level                                                 ABSENT
    elab  input only
    opts  none. options.h:52 comments a future multi-level
          struct; every live field is l1_ or mm_ prefixed.

  geometry.capacity_bytes                               RENAME
    elab  derived.bytes_per_way (EL:57), data_array_bits (EL:61)
    opts  l1_capacity, options.h:64. Units agree, bytes.

  geometry.line_bytes                                   RENAME
    elab  derived.offset (EL:69), refill_beats (EL:98)
    opts  l1_line_size, options.h:67. Units agree, bytes.

  geometry.associativity                                OK
    elab  derived.bytes_per_way (EL:57),
          replacement_state.bits_per_set (EL:78)
    opts  l1_associativity, options.h:68.

  geometry.banks                                        ABSENT
    elab  none
    opts  none.

  addressing.indexing                                   ABSENT
    elab  derived.vipt (EL:117)
    opts  near-miss l1_tag_type, options.h:78, default
          "PHYSICAL" (options.cpp:157). Tag physicality is not
          index source; PIPT against VIPT is not expressible.

  addressing.va_bits                                    ABSENT
    elab  derived.vipt.* (EL:117-136)
    opts  near-miss l1_mmu_present, options.h:81, a boolean.

  addressing.pa_bits                                    RENAME
    elab  derived.tag.bits (EL:71), address_map (EL:138)
    opts  mm_address_bits, options.h:89. Also a derivation, not
          an input: gen.cpp:347 overwrites the options.cpp:174
          default with log2(mm_capacity). This is S-12.

  addressing.page_bytes                                 ABSENT
    elab  derived.vipt.page_offset_bits (EL:122)
    opts  none.

  addressing.alias_policy                               ABSENT
    elab  derived.vipt.translated_index_bits (EL:131)
    opts  none.

  policies.read_miss                                    RENAME
    elab  input only
    opts  l1_read_miss_policy, options.h:71. Value "ALLOCATE"
          (options.cpp:129) against IN:152 "allocate"; case
          differs and options.cpp:226 upcases on load.

  policies.write_miss                                   RENAME
    elab  input only
    opts  l1_write_miss_policy, options.h:72. Value "ALLOCATE"
          (options.cpp:133); same case difference.

  policies.write_hit                                    RENAME
    elab  input only
    opts  l1_write_hit_policy, options.h:73. Value
          "NO-WRITE-THRU" (options.cpp:137) against IN:160
          "write_back". A third spelling of one concept.

  policies.replacement                                  RENAME
    elab  derived.replacement_state.* (EL:73-95)
    opts  l1_replacement_policy, options.h:74. Value "PLRU"
          (options.cpp:141) against IN:164 "tree_plru".
          options.cpp:232 accepts no other value.

  miss_handling.mshrs                                   ABSENT
    elab  derived.mshr.blocking (EL:110), index_bits (EL:108)
    opts  none. The model has one outstanding access by
          construction; mdl.h:122 states the assumption.

  miss_handling.mshr_targets                            ABSENT
    elab  derived.mshr.target_index_bits (EL:109)
    opts  none.

  miss_handling.victim_buffer_entries                   RENAME
    elab  none
    opts  l1_victim_buffer_size, options.h:76. Forced to 0 at
          options.cpp:237. Units agree, entries.

  miss_handling.fill_buffer_entries                     ABSENT
    elab  none
    opts  none.

  interfaces.core_read_width_bits                       RENAME
    elab  none; GL:131 IF-1 only
    opts  l1_word_size, options.h:108, fixed 32 with a FIXME.
          One member serves both directions; see the next row.

  interfaces.core_write_width_bits                      RENAME
    elab  none
    opts  l1_word_size, options.h:108. The same member as the
          row above. A two-to-one collision: the model cannot
          express differing read and write widths at all.

  interfaces.write_granularity_bytes                    ABSENT
    elab  none. See R5-5.
    opts  none. Byte granularity 1 is hard-coded in the four
          fixed lanes of Utils::stBytes (utils.cpp:654-677) and
          Ram::st (ram.cpp:150-169). New in 2.0.0.

  interfaces.refill_width_bits                          RENAME
    elab  derived.refill_beats (EL:98), beat_index_bits (EL:102)
    opts  mm_fetch_size, options.h:93. Units differ: BYTES
          (options.cpp:187 default 32) against IN:222 BITS. A
          unit mismatch on a field that feeds a width.

  interfaces.next_level_name                            ABSENT
    elab  input only
    opts  none.

  interfaces.refill_beat_order                          ABSENT
    elab  checks[] (EL:167) as not_applicable, per IN:231
    opts  none. New in 2.0.0.

  interfaces.fill_bypass_to_core                        RENAME
    elab  none
    opts  l1_critical_word_first, options.h:80, default true
          (options.cpp:161), required true (options.cpp:219).
          New in 2.0.0 and the only cross-group rename in this
          table: the member's counterpart moved from policies to
          interfaces. See R3-A.

  maintenance.invalidate_all                            ABSENT
    elab  input only
    opts  none.

  maintenance.invalidate_line                           ABSENT
    elab  input only
    opts  none.

  maintenance.flush_all                                 ABSENT
    elab  input only
    opts  none.

  emission.profile                                      ABSENT
    elab  input only
    opts  none.

  emission.emit                                         ABSENT
    elab  manifest[].kind (EL:154). Broken; see R5-1.
    opts  none. The model's artifact set is fixed at
          gen.cpp:64-70.

  emission.output_dir                                   RENAME
    elab  manifest[].path (EL:153), unconstrained; see R5-12
    opts  data_dir, options.h:101, default "data"
          (options.cpp:112). Loose: IN:282 names rtl/, tb/,
          tests/ subdirectories the model does not use.

  emission.reset_polarity                               ABSENT
    elab  none. See R5-4.
    opts  none. The model has no reset concept. New in 2.0.0.

  emission.reset_name                                   ABSENT
    elab  none. See R5-4.
    opts  none. New in 2.0.0.

  emission.sram_model                                   ABSENT
    elab  input only
    opts  none.

  emission.macro_name                                   ABSENT
    elab  input only
    opts  none.

  emission.verification_params                          ABSENT
    elab  input only
    opts  none.

  verification.golden_vectors                           ABSENT
    elab  manifest[] entries of kind golden; blocked by R5-1
    opts  none. See R1-6 for the overlap with emission.emit.

  verification.directed_tests                           RENAME
    elab  checks[].id and .result (EL:175, EL:177)
    opts  seven booleans, options.h:151-157. Set in code at
          mdl_main.cpp:10-17, never read from JSON, and named
          basicLruTest through basicWrEvictTest against the
          IN:329-343 vocabulary. One array against seven flags.

  verification.assertions                               ABSENT
    elab  checks[].id and .result (EL:175, EL:177)
    opts  none. No SVA and no invariant checking anywhere in the
          model.

  $defs.artifact_kind                                   ABSENT
    elab  manifest[].kind (EL:154). Broken; see R5-1.
    opts  none.

  $defs.assertion_kind                                  ABSENT
    elab  checks[].id (EL:175), unenumerated; see R5-2.
    opts  none.

Totals for R6-A: OK 1, RENAME 15, ABSENT 29. Of the 29 ABSENT,
four are new in 2.0.0 (write_granularity_bytes,
refill_beat_order, reset_polarity, reset_name), so 2.0.0 widened
the gap to the model by four fields while closing one by rename
(fill_bypass_to_core).

#### R6-B  Options members with no 2.0.0 field, all EXTRA

Complete in the other direction. Grouped by why, since the
regeneration work treats the groups differently. Every member of
Options as declared in options.h is accounted for.

  B1. Derived geometry. EXTRA against IN by definition, because
  GL:20-22 puts no derived value in the input. Each has a proper
  elaborated home, named below, and none belongs in a
  regenerated option struct at all.

    l1_tagBits, l1_tagMsb, l1_tagLsb, l1_tagMask, l1_tagShift
      options.h:118-122 -> EL:71 derived.tag.{bits,msb,lsb,
      mask,shift}. Note EL:200 types mask as a string.
    l1_setBits, l1_setMsb, l1_setLsb, l1_setMask, l1_setShift
      options.h:125-129 -> EL:70 derived.index.*
    l1_offBits, l1_offMsb, l1_offLsb, l1_offMask, l1_offShift
      options.h:131-135 -> EL:69 derived.offset.*  Unit
      ambiguous; see R3-B1.
    l1_sets            options.h:124 -> EL:55 derived.sets
    l1_blocks          options.h:113 -> EL:56 derived.lines_total
    l1_blockBits       options.h:114 -> no elaborated home
    l1_assocBits       options.h:116 -> no elaborated home
    l1_address_bits    options.h:111 -> no elaborated home.
                       gen.cpp:333 sets it to log2(l1_capacity),
                       which is a capacity in bits, not an
                       address width. The name is wrong.
    l1_lru_bits        options.h:140 -> EL:78
                       replacement_state.bits_per_set
    l1_wrdShift, l1_wrdMask
      options.h:137-138 -> no elaborated home. These are the
      word-offset sub-field of R2-B1, present in the model and
      absent from both schemas.
    l1_tagKB           options.h:141 -> no home. Never assigned
                       (M-19); delete.
    mm_lineMsb, mm_lineLsb, mm_lineShift, mm_lineMask
      options.h:95-98 -> no elaborated home. Next-level address
      decomposition; belongs with S-05, deferred.
    mm_entries         options.h:94 -> no home. See M-24.
    default_mm_entries options.h:107 -> no home. See M-24.

  B2. Value-and-units pairs. EXTRA and forbidden. GL:95-97 says
  "No value plus units pair anywhere" in the input. EL:139
  permits a rendered string in the elaborated output only.

    l1_capacity_value, l1_capacity_units  options.h:65-66
    mm_capacity_value, mm_capacity_units  options.h:91-92
      The second pair carries M-03, the live round-trip defect
      at options.cpp:321. Delete both pairs.

  B3. Out of scope by a planning document. EXTRA and to be
  deleted or argued for explicitly.

    l1_coherency_protocol  options.h:75. GL:177-179 forbids it
                           by name: "coherence is not a field".
                           Written to the datasheet at
                           gen.cpp:311 and to JSON at
                           gen.cpp:323.
    l1_mmu_present         options.h:81
    l1_mpu_present         options.h:82
    l1_store_buffer_size   options.h:77. This is the write
                           buffer field held in the deferred set
                           by Background; recorded, not argued.

  B4. Next-level configuration. EXTRA against 2.0.0 and squarely
  inside S-05, which is deferred.

    mm_capacity            options.h:90
    mm_fetch_size          options.h:93 is the RENAME row above,
                           listed there, not here.

  B5. Tool invocation, not cache configuration. EXTRA and
  correctly so: these are CLI concerns, and IN has no group for
  them because IN:5 scopes the file to "a decision a person
  makes" about the cache.

    verbose      options.h:48     dry_run      options.h:50
    generate     options.h:55     run          options.h:56
    load_json    options.h:57     interactive  options.h:83
    cmd_file     options.h:103    json_file    options.h:102
    tc_prefix    options.h:100    transactions options.h:60
    preload_mm, preload_tags, preload_bits, preload_dary
                 options.h:84-87
    datasheet    options.h:143
    bits_file, tags_file, mm_file, daryFiles  options.h:144-147.
      These four are the manifest of B4's artifacts and are the
      members that should become EL:145-165 manifest[] entries.
      Blocked on R5-1; see R3-B4.

  B6. Test selection. EXTRA. Listed in R6-A as the RENAME for
  verification.directed_tests and repeated here for completeness
  of the reverse direction.

    basicTests        options.h:150
    basicLruTest, basicRdHitTest, basicWrHitTest,
    basicRdAllocTest, basicWrAllocTest, basicRdEvictTest,
    basicWrEvictTest  options.h:151-157

  B7. Plumbing, not configuration. EXTRA, no schema home wanted.

    msg  options.h:43   u    options.h:44
    vm   options.h:46   json options.h:159

Judgement for the regeneration work. Of the 45 entries above, 43
fields plus the 2 $defs, one maps cleanly, 15 map under a rename
and 29 have no counterpart at all. Of the Options members, groups
B1 and B2 must leave the struct entirely rather than be renamed,
because GL:20-22 and GL:95-97 put derived values and unit pairs on
the other side of the tool. That is the reason I1:1100-1102 M-25
gave for regenerating the struct rather than porting it field by
field, and this table is the evidence for it: a field-by-field
port would carry 34 members of B1 and B2 across a boundary they
are not allowed to cross.

## Test Case Results

No tests were run. Binding Previous Decision 2 grants no execute
permission, so nothing was built, simulated or executed. The only
commands issued were the validation tool named in the /run
procedure, file reads of the manifest, and directory listings
under Binding Previous Decision 3. Validation of 2.0.0 was
reasoned about by reading the schema, as instructed; no JSON
Schema validator was run and the file was not machine parsed.

Static observations bearing on a future run.

- mdl still always exits 1. mdl_tests.cpp:171, the
  basicWrEvictTest stub at mdl_tests.cpp:670-675, and the
  1000-per-disabled-test scoring at mdl_run.cpp:55-68 are all
  unchanged. Any task gating on "model tests pass" is still
  blocked. M-12.
- The RTL still will not pass Verilator. bitrf.v:35 and
  lrurf.v:35 are continuous assigns to reg outputs, and cache.v
  drives mm_write_d from both :156 and :400. S-17c, S-17i.
- probes.v still will not elaborate against top.v. probes.v:181
  through :189 reference top.dut0.mm0.ram, and top.v:234 makes
  dut0 the cache while top.v:269 makes mm0 its sibling. Note that
  probes.v:7-15 do resolve against top.v, since dut0 does contain
  dsram0 through dsram3 (cache.v:491-529); it is the main-memory
  paths that fail there. S-17f.
- misc/example_pacino_icache.json will fail validation on three
  fields once a validator exists. R4 gives them and the minimum
  edit. It will not fail on schema_version, which is R1-2.
- If a validator is stood up before R5-1 is fixed, any elaborated
  file recording a preload, golden or probes artifact will fail
  against EL:155. That is the first thing a generation run will
  hit and it is a schema fix, not a code fix.

## Assumptions made not explicit in the prompt

A-1  The Background gives S-07 two different dispositions. The
     DELIBERATELY DEFERRED list names "S-07 per-array storage
     kind" and says "Mark them OPEN and move on." The S-07 CAVEAT
     says "record S-07 as WITHDRAWN and state that the prior file
     was not available to check it against." Both clauses govern
     the same scope, the disposition of S-07 in the R0 table, and
     they give different values. I took WITHDRAWN, because the
     CAVEAT is specific to S-07, states a reason, and is the later
     of the two. I did not halt because the prompt permits
     choosing an interpretation when it is recorded with the
     ambiguous wording quoted, which is done here.

A-2  Binding Previous Decision 2 says to "Reason about validation
     by reading the schema, not by running a validator." I read
     that as also excluding a mechanical JSON parse, so
     cgen_config.schema.json was assessed by reading all 450
     lines and its brace and bracket structure was checked by
     reading rather than by a parser. Every R1 and R4 statement
     follows from the text as read. If a parse were permitted,
     nothing in R1 or R4 would change but the well-formedness
     claim would be stronger.

A-3  R6 asks for an "elaborated path" column. EL:43-46 carries
     the input verbatim, so every input field has a trivial
     input.<path> home and the column would say the same thing 45
     times. I assumed the useful reading is the derived,
     provenance or manifest path that consumes or records the
     field, and wrote "input only" where the verbatim copy is the
     only home. The convention is stated at the head of R6.

A-4  R0 requires evidence citing "a file and line in the current
     tree" for CLOSED, PARTIAL and OPEN. For the twelve P
     findings that are inconsistencies between two documents, I
     cited the line in each document that still carries the
     inconsistency, rather than one line. For P-06 and P-12 the
     evidence is a document line plus a directory listing, which
     Binding Previous Decision 3 permits.

A-5  For P-08 the third expansion of IA is in CLAUDE.md, which is
     not in the manifest and which PC:186-188 forbids adding to a
     context list. I cited only the two expansions inside the
     manifest, PC:26 and PC:61, which already disagree with each
     other. The finding does not depend on the third.

A-6  R2 and R3 ask what "remains missing". I read that as
     excluding defects in the RTL and the model themselves, which
     no schema change can close, and confined R2 and R3 to
     expressibility. The code defects are carried in R0 at S-17
     and M-03 through M-24 and are not repeated.

## Decisions made not explicit in the prompt

D-1  I recorded the answer to the Hypothesis's mechanical
     question as a negative result in R1-0, with the per-branch
     trigger and reachability analysis R1 asked for, rather than
     reporting only the cases that fail. Binding Previous
     Decision 4 makes "this whole group is now adequate" a
     deliverable, and the finding that no allOf branch is
     bypassable bounds the next task's work as usefully as a
     defect would.

D-2  Where a finding closed on the input side and remains open on
     the elaborated side, I used PARTIAL rather than splitting it
     into two IDs. Six findings are in that state: S-03, S-08,
     S-09, S-10, S-12 and S-14. Splitting them would have changed
     the ID set INFRA-001 established and broken the one-row-per-
     ID requirement of R0.

D-3  I marked S-10 PARTIAL and not CLOSED. R0 forbids closing on
     the strength of a description, and a description at IN:325
     is the only thing that changed. PARTIAL states the progress
     without breaking the rule.

D-4  I marked S-01 and S-19 CLOSED although neither was ever a
     defect. Both were recorded as negative results; both are
     still accurate; none of the six dispositions means "still
     true, no action was required". CLOSED is the least wrong and
     the evidence cell says so in each case.

D-5  I reported three inconsistencies that 2.0.0 introduced and
     that no INFRA-001 finding covers: R1-6 the two mechanisms
     for golden data, R1-7 the clock name lost from the pacino
     profile description, and R5-4 the reset fields promising a
     manifest record that does not exist. The Background asked
     that an unexplained difference in a description field be
     treated as a finding, and these are the three that have a
     consequence rather than being wording.

D-6  I did not propose replacement text for any schema or
     planning document. Each finding states what is wrong and
     what a fix must achieve. This follows the prompt's
     Constraints and PC:123.

D-7  I answered the challenge on all four drafter decisions in
     one place, R1-10, rather than scattering them. Two are
     endorsed as written, one is endorsed with its stated
     rationale rejected, and one is endorsed with the consequence
     that it leaves the naming fix one-directional.

## RVA23 compliance risks and gaps noticed

The manifest still contains no RVA23 material and no ISA-facing
code, so this section remains TBD by absence. The three items
INFRA-001 carried forward are all still live, and 2.0.0 changes
one of them.

R-1  fence.i. GL:68 still equates invalidate_all with fence.i and
     IN:248 still says invalidate_all is "Required for fence.i on
     an icache". 2.0.0 changed the default from true to false
     (IN:250), which is the S-11 fix and is the right direction:
     the tool now has to verify emission rather than assume it.
     The example still implements no maintenance operation at all
     (cache.v:93-96 declared and undriven, probes.v:33 decoding a
     state fsm.v never enters), so an icache generated from it as
     a template would still not support fence.i. What changed is
     that the schema no longer claims otherwise by default.

R-2  Sv39, Sv48, Sv57. IN:122-125 still offers va_bits 32, 39, 48
     and 57 and IN:132-136 still fixes page_bytes at 4096. The
     model still has no virtual address concept beyond
     l1_mmu_present (options.h:81) and the RTL has none. GL:129
     VIPT-1 is still the check that matters and is still
     unimplemented (M-17). New this session: misc/example_pacino_
     icache.json is a Sv39 VIPT instance sitting exactly at the
     VIPT-1 limit, bytes_per_way 4096 against page_bytes 4096, so
     the first configuration that will exercise VIPT-1 now exists
     in the tree and has zero headroom. Any increase in its
     capacity per way introduces synonyms, which under IN:139
     alias_policy forbid the tool must reject.

R-3  Address width. IN:129-130 still allows pa_bits 32 through
     56, and 2.0.0 strengthened the field's contract at IN:127 by
     stating it is an input and never a derivation. The RTL is
     still hard-wired to a 32-bit request address (cache.v:29)
     and a 14-bit tag (cache.v:51), and compare.v:20-25 still
     hard-codes [14-1:0] on every port with the parameterized
     forms commented out. Any RVA23 implementation with a
     physical address wider than 32 bits still needs the
     parameterization S-12 and S-17a show is absent. The example
     instance at EX:18 uses pa_bits 56, the maximum, so the first
     instance in the tree already sits at the width the RTL
     cannot reach.

R-4  New. The generated model would be unable to record the
     result of any of these checks. EL:175 types checks[].id as a
     free string with no enumeration (R5-2), so VIPT-1, ADDR-1
     and the rest have no defined identifier in either schema. An
     RVA23 compliance argument that rests on "the tool checked
     it" needs the id set to be closed before the record means
     anything.

## Deferred Work

DW-1  Close R5-1. Add preload, golden and probes to EL:155 and
      remove or retain vectors deliberately. This is the highest
      value item in the set: it is three enum values, and it
      unblocks S-08, S-09 and R3-B4, which is the manifest half
      of the model rewrite. It also removes the false claim of
      unity at IN:25.

DW-2  Close R5-2. Decide the check identifier set and constrain
      EL:175 against it. The GL:121-139 ids already exist as a
      table; they are unenumerated in both schemas. This is the
      other half of DW-4 in the INFRA-001 set and it gates R-4
      above.

DW-3  Fix R1-1. Add the forbid-when-inapplicable mirror to the
      VIPT, mshr and macro conditionals, using the pattern
      already at IN:371-374. Three edits, no new construct, and
      it makes the schema consistent with the principle GL:74-80
      states.

DW-4  Fix R1-2. Give schema_version a mechanism that can reject
      the version 2.0.0 replaced. The decision to make is whether
      minor versions validate forward.

DW-5  Settle the offset unit, R2-B1 and R3-B1. This blocks the
      model's derived block, the emitter's word-select mux, and
      the datasheet diagram at gen.cpp:428. It is the one open
      item that changes a value rather than a name, so it should
      be settled before any regeneration starts. It is the S-02
      half of INFRA-001 DW-3.

DW-6  Decide R1-6. One of emission.emit "golden" and
      verification.golden_vectors has to go, or the boolean has to
      mean something else. Also revise GL:138 EMIT-2, which names
      an artifact kind that no longer exists.

DW-7  Decide R1-7. Either restore the clock to the pacino profile
      description at IN:271 or give it a field beside
      reset_name, and reconcile GL:99-105 either way.

DW-8  Add the elaborated homes named in R5-4 and R5-5. Reset
      polarity and name, and byte-enable width. Both are fields
      2.0.0 added whose stated purpose EL cannot serve, and both
      need EL:52 additionalProperties false opened deliberately
      rather than by accident.

DW-9  Close the reachability gaps in R5-7 and R5-8. derived.vipt
      and derived.mshr are optional with no conditional making
      them present when the input calls for them.

DW-10 Carry forward unchanged from INFRA-001: DW-1 the ten RTL
      defects, DW-2 the model defects in severity order, DW-5 the
      P-03 and P-04 naming pass, DW-6 populating ANTIPATTERNS.md,
      DW-8 the template-or-reference decision, DW-9 the GLOSSARY
      open question 1. INFRA-001 DW-3 is reduced to S-02, S-05,
      S-06 and S-07 by the closures in R0; INFRA-001 DW-4 is
      reduced to its S-08 and S-09 halves, which are DW-1 here;
      INFRA-001 DW-7 is applied by this task's manifest.

## Other Notes

N-1  Answer to the Hypothesis, stated plainly. The subset 2.0.0
     addresses deliberately is genuinely closed: byte-enable
     width on the input side, the critical-word-first split,
     reset polarity and name, the invalidate_all default, the
     assertions list, the artifact kinds as requests, and the
     extension point at the top level. Seven CLOSED and six
     PARTIAL out of 64 is the right shape for a change that
     touched one of two schema files and none of the code. The
     document is mechanically sound on the specific question the
     Hypothesis raised and unsound on a different one, R1-1.
     What the change broke is in R5-1, R5-4, R5-5, R1-6 and
     R1-7, and four of those five are the same defect: a field
     or claim added to the input schema whose counterpart in the
     elaborated schema was not revised.

N-2  The single most consequential item in this report is R5-1,
     and it was handed to me as known breakage rather than
     discovered. That is worth saying because it is three enum
     values and it blocks the model rewrite, the probes
     artifact, the preload and golden split, and any generation
     run at all. It should not stay open across another task.

N-3  IN:25 is the sentence to watch. "These two lists must not
     diverge; they are one definition referenced twice." They are
     not one definition, they have diverged, and the assertion
     now reads as a guarantee to anyone who does not open the
     other file. A false claim of unity is worse than the
     duplication it replaced, because the duplication was at
     least visible. This is the same argument GL:151-157 makes
     about the four PLRU tables, and it is the reason the
     recommended mechanism in R5 is generation rather than
     documentation.

N-4  2.0.0 widened the model's gap while narrowing the RTL's.
     Four of the fields it added (write_granularity_bytes,
     refill_beat_order, reset_polarity, reset_name) have no
     Options counterpart, so 2.0.0 added four ABSENT rows to
     R6-A. I do not give a before-and-after count: I1:722-785
     collapsed maintenance and emission into one row each, so
     the two tables are not comparable by row count. That is not
     an argument against the additions; it is the reason the
     struct should be regenerated from the schema rather than
     chased, which is what I1:1100-1102 concluded and what R6-B
     now quantifies at 34 members that must leave the struct
     rather than be renamed.

N-5  Two of the three failures in misc/example_pacino_icache.json
     are exactly the two changes Background decision 1 cites as
     breaking, and the third is the artifact kind. That is a
     clean corroboration: the major bump was called for the right
     reasons, and the one instance in the tree demonstrates both
     of them. It also shows the bump is inert, since EX:2 still
     says 1.0.0 and passes.

N-6  Context cost of this task: 40 files, 8,894 lines, against
     the 39 files and 14,727 lines I1:1322-1323 records for
     INFRA-001. The saving
     is INFRA-001 DW-7 applied. No finding in this task needed
     jsoncpp and none was blocked by its absence, which retires
     the question DW-7 raised.

N-7  Nothing in the manifest was modified. The only file written
     is this one, between the RESULTS markers, plus the Model
     field in the header as CLAUDE.md permits. No planning file
     was edited; the changes R1, R5 and DW propose are described
     and not applied, per Binding Previous Decision 1.

## Files Modified
- prompts/INFRA-002.md

:: RESULTS:END ::

=============================================================
:: CONTEXT:START ::
=============================================================

# Context usage

Context used: 30 percent

Window 1,000,000 tokens. Used 304,700. Free 694,700.

Model claude-opus-5[1m]. Figures are the /context snapshot taken
after INFRA-002 completed.

## Usage by category

| Category      | Tokens  | Percent |
|---------------|---------|---------|
| Messages      | 273.7k  |   27.4  |
| System tools  |  23.2k  |    2.3  |
| System prompt |   4.0k  |    0.4  |
| Skills        |   2.7k  |    0.3  |
| Memory files  |   1.8k  |    0.2  |
| Free space    | 694.7k  |   69.5  |

The five occupied categories sum to 305.4k against the reported
total of 304.7k; the difference is rounding in the per-category
figures.

Deferred tool schemas are not a separate line in this snapshot.
The six MCP tools and the deferred built-ins are counted inside
System tools. INFRA-001 reported them separately, at 1.5k and
16.3k, so a direct category comparison with that task is not
valid for the tools rows.

## MCP tools

All six are deferred. None was loaded or called this session.

| Tool                                          | Tokens |
|-----------------------------------------------|--------|
| mcp__claude_ai_Gmail__authenticate            |    206 |
| mcp__claude_ai_Gmail__complete_authentication |    267 |
| mcp__claude_ai_Google_Calendar__authenticate  |    217 |
| mcp__claude_ai_Google_Calendar__complete_auth |    282 |
| mcp__claude_ai_Google_Drive__authenticate     |    212 |
| mcp__claude_ai_Google_Drive__complete_auth    |    277 |

## Memory files

| Type    | Path                            | Tokens |
|---------|---------------------------------|--------|
| Project | CLAUDE.md                       |   1.7k |
| AutoMem | ~/.claude/projects/-home-jeff-D |     45 |
|         | evelopment-jeffnye-gh-cachegen/ |        |
|         | memory/MEMORY.md                |        |

## Skills

| Skill                    | Source   | Tokens |
|--------------------------|----------|--------|
| cntx                     | Project  |   ~30  |
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

Two skills were invoked this session, run and cntx.

## What consumed the Messages total

Messages is 273.7k of the 304.7k used, 89.8 percent of everything
occupied. Two things account for nearly all of it, both measured
this session rather than estimated.

The context manifest, read complete under the CLAUDE.md rule
requiring every context file to be read before acting:

```
  40 files, 8,894 lines, 316,829 bytes
```

The Results Capture written back into prompts/INFRA-002.md:

```
  2,000 lines, 94,601 bytes
```

The manifest breaks down as 11 planning, schema and instance
files, 13 RTL sources, 7 model headers and 9 model sources.

## Effect of INFRA-001 deferred work item DW-7

DW-7 recommended dropping the two stock jsoncpp files from future
manifests. INFRA-002 applied it. The files not read were:

```
  examples/1mb_l1/model/src/jsoncpp.cpp     5,262 lines
  examples/1mb_l1/model/inc/json/json.h     2,139 lines
                                            -----
                                            7,401 lines, 224,145 bytes
```

That is 45.4 percent of what the manifest would otherwise have
been, 8,894 lines against 16,295. INFRA-001 read 14,727 lines
across 39 files and reported 33 percent context used; INFRA-002
read 8,894 lines across 40 files and reports 30 percent.

The saving is smaller than the line count suggests because this
task also read prompts/INFRA-001.md, 1,435 lines, which was not in
the INFRA-001 manifest, and wrote a Results Capture roughly three
times the size of the previous one. No finding in INFRA-002 needed
jsoncpp and none was blocked by its absence, so DW-7 can be closed
rather than carried forward.

## Notes

Headroom remains large. 694.7k free is enough for a task of this
manifest size to run twice more in the same session without
compaction.

The manifest rule, not the command, is where the cost sits. Every
file was read whole because CLAUDE.md requires it, so head, tail
and grep do not apply. The next reduction available is the same
kind DW-7 was: naming fewer files, or marking a file read not
required.
:: CONTEXT:END ::

