<!-- SPDX-License-Identifier: Apache-2.0                        -->
<!-- Copyright (c) 2026 Jeff Nye, uarchlabs.com                 -->
<!-- SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com> -->

```             
 FILE:    verilog_style.md
 SOURCE:  various
 STATUS:  WORKING
 UPDATED: n/a   
 CONTACT: Jeff Nye
``` 

## General verilog directives

- Prefer always_comb blocks over cascaded continuous assign statements when
  signals form a dependency chain (A depends on B depends on C). Verilator
  v5.048 evaluates assign statements using an internal dependency DAG and may
  read stale values when a chain is evaluated out of order. always_comb blocks
  evaluate statements in textual order, eliminating the ambiguity.

- Single assign statements with no internal dependencies are acceptable.

## Verilog package imports

Import at file scope, before the module declaration. Do not place import
statements inside the module header between the module name and the port list.

  Correct:
    import xyz_pkg::*;
    import abc_pkg::*;
    module foo (

  Incorrect (Verilator accepts but project rejects):
    module foo
      import xyz_pkg::*;
      import abc_pkg::*;
    (

More on Verilog package imports will be added as the project proceeds.

---
