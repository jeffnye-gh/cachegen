// vi:syntax=verilog
`ifndef SIM_CNTRL
`define SIM_CNTRL 1

`define FF 
//`define FF #1

`define ENABLE_DUMP 1
`define BASIC_TESTS 1
`define TAG_RW_TEST 1

`define FILLTAGS_VERBOSE 0
`define FILLDARY_VERBOSE 0
`define FILLMEM_VERBOSE  0
`define RDHIT_VERBOSE    0
`define WRHIT_VERBOSE    0
`define RDMISS_VERBOSE   1

`define CLR_TAGS 0
`define CLR_DARY 1
`define CLR_MEM  2

`endif
