<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->
# Project Core — RISC-V RVA23 Processor Co-Design
```
 FILE:    PROJECT_CORE.md
 SOURCE:  various
 STATUS:  DRAFT
 UPDATED: 2026-08-23
 CONTACT: Jeff Nye
```

This file contains slow moving conventions and rules for the flow.
Paste into Claude only when methodology is under discussion. 
Not required for routine sessions.

---

## Terminology

The project record uses two abbreviations that appear in file
names, handoffs and postmortems. 

```
  PA    Planning Assistant. Claude.ai in the web interface.
  IA    Implementation Assistant. Claude Code in the terminal.
```

---

## Project Overview

AI-assisted co-design of a microprocessor cache generator tool (cgen) in C++
which generates a hierarchical SystemVerlog model, and testing environment, 
as well as a C++ functional model and testing environment.

The executable is called cgen. Cgen reads configuration files and generates
the outputs.

Inputs are:
  - populated json configuration, called a cgen config json
  - command line options

Outputs are:
  - populated json configuration, called an elborated cgen config json
  - generated System Verilog(SV) of the design and all components
  - generated SV of the testbenches used for units and top level
  - generated SV tasks for unit and top level testins
  - a Make based build environment and flow for execution of the SV model
  - generated C++ for a functional model of the cache design


There are two components to cgen, the central command line driven application
and a GUI layer overlaid onto the CLI application. The GUI provides an
alternative way to execute the CLI and the simulation environment.

---

## Flow summary

The Interactive Assistant (IA, Claude Code) works with the user to 
define planning files in markdown format which describe the requirements 
of the output and the structure of the C++ forming cgen.

Jeff provides architectural decisions and direction with the assistance of
the IA provides design guidance. The IA writes the experiment prompts. 
The IA executes the prompts.

There templates in place for prompts (prompts are also called task files or
experiment files), status and results reporting.

At times Claude.ai is used to create planning files and emitting
task files.

Repo:  https://github.com/jeffnye-gh/cachegen


Task IDs use various prefixes:

```
  GUI-NNN    a task for generation of GUI code
  CLI-NNN    a task for generation of CLI code
  INFRA-NNN  a read-only IA audit or inventory task
  TOOLS-NNN  a task for generation of support scripts and tools
  COMP-NNN   a shared-component task
```

This is not a complete list.

A PA-direct edit -- a planning document the PA drafts and Jeff
pastes without an IA task -- takes NO task number. Cite it as
"PA-direct correction, session-NNN". Attaching an unissued task
ID to one is how INFRA-012 came to be cited three times before it
was ever generated.

---

### Flow Responsibilities

IA and PA:
- These tools provide automation and research guidance drivend interactively
  by the user.

User:
- Makes all architectural decisions
- Supplies session prompts to the IA for execution
- Reviews IA output and results, occiasionally reports results to the PA
- Applies planning document changes drafted by the IA/PA
- Commits results to git

The IA has no implicit priviledges for read or write to any file except
CLAUDE.md, .claude/commands in this repo. 

The IA has no priviledges to run git/bash/shell/scripts of any sort unless
the permission has been explicitly provided per session or expressly 
declared in CLAUDE.md.

---

## Planning document ownership
PLANNING DOCUMENTS ARE ONLY MODIFIED BY THE IA under restricted permission.

The IA reports what a document should say; the user drafts and applies the change

A planning path in Deliverables, or in a "Files expected to change" list, 
does not grant write permission -- that list is a convenience and was
never scoped to act as one.

WAIVER. Only an explicit statement from Jeff in the task file,
naming the files and the task ID, permits an IA write. A waiver
covers that task only. It is never precedent and does not
propagate to the next task, even one continuing the same work.
BP-098 through BP-105 ran under such a waiver and it expired
with BP-105.

CLAUDE.md carries the enforcing copy of this rule, in Fixed
Constants. This section is the rationale; that one is the rule
the IA reads.

## Experiment file structure

The task file structure is declared as a template in templates/TASK_TEMPLATE.md.

The license for a task file is Apache 2.0

A task file name has a convention, <UU>-NNN.md, UU is the unit or classification
NNN is an integer, 001, 123, etc.

### Task file sections

- Task Header

  Identifies task identification and numbering, stats and status

- Task Overview

- Discussion section

  Captures final IA console output. Location for handwritten results
  assessment. Follow on actions, e.g. CLAUDE.md updates, correction/additions
  for planning files, etc.

- Prompt Section

  This section is written variously, user, IA, PA. It describes the
  specifics of the prompt. These info is contained between the markers
  :: PROMPT:START ::  and  :: PROMPT:END ::

  This section repeats the task ID, lists the explicit paths to context
  to be loaded, comments on context, hypothesis, background discussion,
  specific requirements, constraints, deliverables.

- Results capture section

  This section is populated by the IA as part of the task. A summary
  of what was accomplished, the test matrix as needed, details of what
  was delivered, test case results, assumption made but explicit in 
  prompt, decisions made but not explicit in the prompt, deferred work,
  other notes and then an itemized list of all files modified.
 
- Context capture section

  This is manually populated with context usage information.

### Prompt generation rules
- Do not add CLAUDE.md to the context in generated prompts.
  Claude Code already loads that file. Reloading wastes
  context.

- Do not restate rules already in CLAUDE.md: one module per
  file, line width, indent, reset/clock naming, style rules.

- The task file specifies WORK to be done.

- Do not use results marker syntax in prompt guidance text.
  Using :: RESULTS:START :: / :: RESULTS:END :: markers in
  guidance causes validation script failures. Instead write:
  "Results Capture filled in below."

- IA will violate explicit rules. Review all deliverables against the prompt
  Deliverables section before accepting a result as complete.

- Every factual claim in a Hypothesis or Background section must be verified
  against a file in the manifest before the prompt is issued. 

- Suite-gating waivers: CLAUDE.md requires the IA to run each in-scope module's
  complete suite and blocks completion on any non-waived failure. When writing
  a verification, testbench, debug, or cleanup prompt for a unit that has
  known/open suite failures, the Constraints section MUST enumerate the waived
  tests and cite each one's tech-debt number. A failure not on that list will
  (correctly) block completion. Omitting the waiver list will strand legitimate
  work as in-progress.


### Standing rules
Each of these was paid for by a session and is not re-argued.

- A NEGATIVE RESULT IS A DELIVERABLE. A problem that comes
  back "no defect, here is the enumeration that bounds it" is
  a completed problem, not a failed one.

- FIX WHAT YOU FIND. A defect found while working a problem is
  repaired in the run that found it, not deferred to a
  follow-on task.

- A TASK'S STATUS CHECKBOX IS NOT EVIDENCE THAT IT RAN. 

---

## Repository Layout

cpp and h files are maintained in separate locations

```
  planning/       specification and decision record. See below.
  prompts/        one file per task, BP-NNN.md / INFRA-NNN.md
  templates/      TASK_TEMPLATE.md
  session_handoffs/    session_handoff-NNN.md, PA to next PA session
  docs/           sessions.json, decomposition logs, backend stubs
    examples/     captured exemplary output for documentation purposes
  gui/            location for code implementing or supporting the GUI
    src/          .cpp for gui related source
    inc/          .h include files for gui related source
  cli/            location for code implementing or supporting the CLI
    src/          cpp and .h files are maintained in separate locations
    inc/          .h include files
  output/         This is a temporary directory for cgen output and testing
    <design>/     design name, e.g. l1d, l1i, l2, etc
      rtl/        rtl for the design
      tb/         test bench for the design
  tools/          tools/bin install dir, riscv-opcodes submodule
```

---

## Planning Directory (planning/)

### Document status
A planning document's status is its MATURITY, and it is
recorded in exactly one place: the PROJECT_STATUS Module
Status table. The values in use are DRAFT, CLOSED, NOT STARTED and DEPRECATED.

### Current inventory
Navigational only. PROJECT_STATUS.md Module Status is the
authoritative list and carries each document's status.

```
planning/
  PROJECT_CORE.md            this file
  PROJECT_STATUS.md          living record, updated every session
  CLOSED_TECH_DEBT.md        closure entries, historical
  GLOSSARY.md                
  ANTIPATTERNS.md            known prompt failure modes

  arch/
    cgen_decisions.md        (planned) Top level decisions 
    gui_decisions.md         (planned) gui specific decisions
    cli_decisions.md         (planned) cli specific decisions

  tb/                        (planned)

  schema/
    cache_config.schema.json  configuration json format, input
    output_json.md            elaborated json format, output

  coverage/                  (planned)
```

---

# Tools Status

| Tool           | Version | Location               | Notes            |
|----------------|---------|------------------------|------------------|
| Verilator      | 5.048   | ./tools/bin/verilator  |                  |
| IA status line | --      | ./claude/statusline.sh | Two-line display |

## Submodules 

TBD

