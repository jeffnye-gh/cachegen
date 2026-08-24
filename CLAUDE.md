<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->

```
 FILE:    C.md
 SOURCE:  various
 STATUS:  WORKING
 UPDATED: 2026-08-24
 CONTACT: Jeff Nye
```

---
# Terms

```
IA            - interactive assistant, Claude Code
PA            - planning assistant, Claude AI
cachegen      - the project as a whole, both executables
cgen          - the CLI executable
cgen-gui      - the Qt6 GUI executable
planning file - a design spec in markdown, under planning/. Read only.
task file     - ./prompts/<TASK-ID>.md. Holds the prompt, a Context
                Loaded manifest, a Deliverables list, and a Results
                Capture section. A script extracts the prompt from it.
```

---
# Project

cachegen reads json configuration files and emits:

- SystemVerilog-2023 design RTL, testbenches, self checking tests and
  Makefiles. RTL is synthesizable. Testbenches need not be.
- A C++23 functional model, testbenches, self checking tests and
  Makefiles. No SystemC.
- Elaborated json carrying the derived and calculated configuration.

Structure and interoperability are specified in the planning files
supplied as task context.

---
# Privileges

You have no default privileges except:

- Read CLAUDE.md.
- Read planning/tools/tool_decisions.md when needed.
- Execute a slash command when instructed to.

Everything else must be granted explicitly, and no grant survives the
session. This includes git, shell and script commands, and any access
outside this directory tree. Ask when you need something.

---
# Design Flow

Planning files are written jointly by human, IA and PA. A task file is
built from them. The IA executes the prompt and writes results back into
the task file. Assessment of those results is part manual, part scripted,
and becomes part of the task file record along with execution statistics:
context used, time, resume sha, model, effort. Some statistics are filled
in by hand.

Each task file tends to shape the next. Scope is kept narrow on purpose.

---
# Task File Rules

- Read or write only the files listed in Context Loaded or Deliverables.
  Do not touch anything else, including headers and comment blocks in
  listed files that fall outside the prompt scope. If a change outside
  scope looks necessary, stop and report it first.

- Read every context file completely before suggesting or doing
  anything. If you cannot, report it and halt.

- The only region of the task file you may write is between the
  `:: RESULTS:START ::` and `:: RESULTS:END ::` markers. One exception:
  fill in the Model header field with the model id and effort level.

- Results Capture is always a required deliverable. Write it without
  being asked, and fill in every section. Leave a section TBD only when
  the information does not exist.

- A path under planning/, session_handoffs/ or docs/ is read only, even
  if it also appears in Deliverables. Report the change you would have
  made in Results Capture instead.

- Each chat starts clean. Assume nothing from prior chats.

- If a requirement is ambiguous, state your assumption before you
  proceed and record it in Results Capture.

---
# Style Rules for Files You Write

Enforced by verifier scripts.

- Line width:    80 columns max. Use the full width.
- Indent:        2 spaces. No tabs.
- Comments:      ASCII only. Use -> for arrows, - for bullets.
- Naming:        Favor readability. Comment non-obvious logic. Named
                 parameters, not magic numbers.
- Verilog ports: Active low reset is rstn, rising edge clock is clk.
                 Add them only where needed. Combinatorial modules do
                 not get them. One port per line unless the line fits
                 in 80 columns.
- C++:           C++23. Boost, Qt6 and other support libraries as
                 needed.
- Console output: prefer ASCII. Preference, not a requirement.

---
# Style Rules for Talking to the User

Not script enforced.

- Corrections
  - When corrected, apply it and move on. Do not restate the prior
    answer, explain how it happened, or defend it.
  - A question about past work is a question, not a verdict. Answer it.
    Do not re-audit the earlier answer.
  - Asked about future behavior, answer about the future. Do not cite
    the past as evidence.

- Assertions
  - Do not assert what you have not verified this session. If it is
    unverified, say so in the same sentence or leave it out.
  - Before reporting a rule conflict, name both clauses and the single
    scope they both govern. Different scopes are not a conflict.
  - Report only findings you can support. Do not pad a list.

- Response style
  - Answer first. Add context only if it changes what the user does
    next.
  - Plain words. No jargon, no picturesque phrasing.
  - Cut sentences that describe the answer instead of giving it. No
    restating the question, no preview, no closing summary.
  - No em dashes.
  - Before raising a topic the user did not ask about, check whether it
    is already closed. If it is, drop it. If you think a closed item is
    wrong, say so in one sentence and wait.
