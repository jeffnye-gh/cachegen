<!-- SPDX-License-Identifier: Apache-2.0                       -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com -->
=============================================================
# Task Header 
=============================================================
:: HEADER:START ::

| Field        | Value                   | Notes                    |
|--------------|-------------------------|--------------------------|
| Task ID      | CG-000                  |                          |
| Date         | YYYY.MM.DD              |                          |
| Module       | <module name>           |                          |
| Run time     |                         |                          |
| Ctx %        |                         |                          |
| Model        | <model> <effort>        |                          |
| Resume sha   | <sha>                   |                          |
| IA session   | 000                     |                          |

Task:   [ ] experiment  [ ] implementation  [ ] debug
        [ ] cleanup     [ ] testbench       [ ] verification
Mode:   [ ] automated   [ ] manual          [ ] interactive
Status: [ ] in-progress [ ] complete        [ ] abandoned

# Task Overview

Description of task contained in this file

:: HEADER:END ::

=============================================================
:: DISCUSSION:START ::
=============================================================

# Results Discussion 

## Claude.code Console Output

## Results Assessment

## Follow-on Actions
- [ ] As needed document here
- or Nothing required

:: DISCUSSION:END ::

=============================================================
:: PROMPT:START ::
=============================================================

## Task ID
CG-000

## Context Loaded
@code/src/main.cpp
@code/inc/header.cpp
@planning/cgen_decisions.md

## Context Comments
Comments are not allowed in the Context Loaded section, put
comments about the context here.

## Hypothesis

What this task file is testing or exercising, extended version
of Overview

## Background

As needed.

## Binding Previous Decisions

1. BIND BY MODULE NAME. TD#109. The binds are already correct and
   this task does not touch them. It also must not silently break
   them; see Problem 3.

2. THE ftq UNIT IS 22 TARGETS AND 670 CHECKS. 11 lint, 11 sim.
   That is the number to reproduce, and BP-107's Results Capture
   has the per-target breakdown if you need it. Do not take the
   figure from this prompt as the baseline -- measure it.

3. planning/ IS READ-ONLY, as always.

## Specific Requirements

List of requirements etc.

REQUIREMENT 0 -- BASELINE FIRST, BEFORE ANY CHANGE.
Enumerate the Makefile's targets from the Makefile, not from `all`.

PROBLEM 1 -- THE MOVE.
Move the ten assertion files to rtl/core/frontend/ftq/tb/. 

PROBLEM 2 -- THE MAKEFILE.
Fix every target that referenced the old paths.

PROBLEM 3 -- RE-RUN AND COMPARE.
Run every target individually from a clean tree again. Present the
before and after tables together. Every cell must match. Call out
any difference rather than explaining it away.

## Constraints

Per task limits and constraints

## Deliverables

Task's expected deliverables 


- Results Capture filled in below, in this file:
  prompts/CB-000.md

Fill in every section. Test Matrix may be omitted: this task adds no
test cases. Report the BEFORE and AFTER target tables in full, and
the Problem 3 evidence for all ten files.

:: PROMPT:END ::

=============================================================
:: RESULTS:START ::
=============================================================

## Summary
RESULTS NOT YET WRITTEN -- replace this line when filling in.

## Test Matrix (testbench sessions only, omit otherwise)

## What was delivered

## Test Case Results

## Assumptions made not explicit in the prompt

## Decisions made not explicit in the prompt

## Deferred Work

## Other Notes

## Files Modified
- List every file changed, one file per line, as a bullet.
- No prose. File paths only. Example:
- code/src/main.cpp

:: RESULTS:END ::

=============================================================
:: CONTEXT:START ::
=============================================================

:: CONTEXT:END ::

