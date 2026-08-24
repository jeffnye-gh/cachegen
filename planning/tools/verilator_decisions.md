<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->

```
 FILE:    verilator_decisions.md
 SOURCE:  various    
 STATUS:  WORKING
 UPDATED: 2026.08.24
 CONTACT: Jeff Nye
``` 


## Verilator Makefile Conventions

- Verilator makefile conventions will be provided as context for
  tasks that require Makefile edits and execution.

- Always include -Wno-DECLFILENAME in sim targets. The project naming
  convention (module tb in file tb_<dut>.sv) triggers this warning by
  design. It is not a defect -- suppress it project-wide.
- Use -Wall for all other warnings. Sim and lint targets must both
  exit zero with zero warnings after -Wno-DECLFILENAME and any
  session-specific suppressions noted in the experiment constraints.
- Session-specific suppressions (e.g. -Wno-UNUSED for package-only
  sessions) are listed in the experiment Constraints section and
  must not be added to CLAUDE.md.
- Always include -Wno-IMPORTSTAR in VER_FLAGS. The project
  mandates file-scope wildcard import (import bp_pkg::*;
  before the module declaration). Verilator v5.048 warns on
  wildcard imports in $unit scope. This is structural and
  suppressed project-wide.
- -Wno-VARHIDDEN: add to individual sim or lint targets only when
  a module parameter intentionally shadows a package parameter
  (e.g. NUM_PRED_SLOTS). Do not add to VER_FLAGS.
- Always include -Wno-UNUSED in VER_FLAGS. Package-only
  files and structs not yet consumed by any module will
  trigger unused warnings. This is structural and
  suppressed project-wide.
- add --timing to VER_FLAGS in Makefiles (required by Verilator
  v5.048 for @(posedge clk))


