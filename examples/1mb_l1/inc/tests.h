// vi:syntax=verilog
`include "sim_cntrl.h"
`include "bitcmds.h"
`include "functions.h"
// --------------------------------------------------------------------------
// Read miss allocate to invalid way
//
// Reads to invalid ways should allocate from main memory and return 
// the critical word in parallel. 
//
// LRU bits are arbitrarily set
// Mod bits are also arbitrarily set
// Tags are same as basicRdHit, this is to test invalid overriding tag match.
//
// index invalid way  mod    lru
//   0       0        0111   010
//   1       1        0100   100
//   2       2        0010   000
//   3       3        1111   111
//   4       3        0111   000
//   5       -        0000   000
//   6       2        0100   010
//   7       1        1110   001
//   8       -        0010   111
//   9       0        1001   101
//  10       -        1100   010
//  11       1        0111   001
//  12       2        1100   010
//  13       2        0101   110
//  14       3        0010   011
//  15       2        0100   000
//
// --------------------------------------------------------------------------
task basicRdAllocTest(inout int errs,input int verbose);
integer i,j,mod,lclerrs;
int v;
reg [2:0] lru_exp,lru_act;
reg [31:0] addr;
begin
  initTest("basicRdAllocTest",errs);

`ifdef EXPERIMENT
  $display("-I: experiment is ON");
`else
  $display("-I: experiment is OFF");
`endif

  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,v);

  nop(4);

  if(verbose) $display("-I: setting initial configuration ");
  //load main memory
  $readmemh("data/basicRdAlloc.mm.memh",top.mm0.ram);
  //load data arrays
  $readmemh("data/basicRdAlloc.dsram0.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicRdAlloc.dsram1.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicRdAlloc.dsram2.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicRdAlloc.dsram3.memh",top.dut0.dsram3.ram);
  //load tags
  load_initial_tags("data/basicRdAlloc.tags.memh",v);
  //load control bits
  load_initial_bits("data/basicRdAlloc.bits.memb",v);

  nop(4);

  //some arbitrary nops inserted just for simple state test

          //tag/way index   word                  --wpWpii
  rd_req({14'h000,13'h000,3'h3,2'h0},4'b1111,1);//

//  rd_req({14'h003,13'h000,3'h3,2'h0},4'b1111,v);//00303000
//  rd_req({14'h001,13'h001,3'h7,2'h0},4'b1111,v);//00107001
//  nop(4);
//  rd_req({14'h002,13'h002,3'h6,2'h0},4'b1111,v);//00206002
//  nop(1);
//  rd_req({14'h000,13'h003,3'h5,2'h0},4'b1111,v);//00005003
//  rd_req({14'h000,13'h004,3'h2,2'h0},4'b1111,v);//00002004
//  nop(4);
//  rd_req({14'h001,13'h001,3'h1,2'h0},4'b1111,v);//00101001
//  rd_req({14'h002,13'h005,3'h5,2'h0},4'b1111,v);//00205005
//  rd_req({14'h003,13'h007,3'h3,2'h0},4'b1111,v);//00303007
//  nop(2);
//  rd_req({14'h000,13'h002,3'h2,2'h0},4'b1111,v);//00002002
//  rd_req({14'h001,13'h003,3'h1,2'h0},4'b1111,v);//00101003
//  rd_req({14'h000,13'h003,3'h6,2'h0},4'b1111,v);//00006003
//  nop(4);
//  rd_req({14'h001,13'h003,3'h7,2'h0},4'b1111,v);//00107003
//  rd_req({14'h003,13'h006,3'h4,2'h0},4'b1111,v);//00304006
//  rd_req({14'h000,13'h006,3'h3,2'h0},4'b1111,v);//00003006
//  nop(1);
//  rd_req({14'h000,13'h002,3'h0,2'h0},4'b1111,v);//00000002
//  rd_req({14'h001,13'h000,3'h1,2'h0},4'b1111,v);//00101000

  //load expect main memory
  load_expect_main_memory("./golden/basicRdAlloc.mm.memh",v);
  //load expect data arrays
  load_expect_dary_data("./golden/basicRdAlloc.d0.memh",
                        "./golden/basicRdAlloc.d1.memh",
                        "./golden/basicRdAlloc.d2.memh",
                        "./golden/basicRdAlloc.d3.memh",v);
  //load expect tags
  load_expect_tags("./golden/basicRdAlloc.tags.memh",v);
  //load expect control bits
  load_expect_bits("./golden/basicRdAlloc.bits.memb",v);
  //load expect tb capture info
  load_expect_capture_data("./golden/basicRdAlloc.capa.memh",
                           "./golden/basicRdAlloc.capd.memh",v);

  nop(4); //let state propagate

  check_main_memory (errs,0,1,v);
  check_data_arrays (errs,0,1,v);
  //check tags and bits
  check_tb_tags_bits(errs,0,1,v);
  //check capture address and data
  check_tb_capture_info (errs,0,1,v);

  nop(4);
end
endtask

// --------------------------------------------------------------------------
task basicWrAllocTest(inout int errs,input int verbose);
integer i,j,mod,lclerrs;
int v;
reg [2:0] lru_exp,lru_act;
reg [31:0] addr;

begin
  initTest("basicWrAllocTest",errs);
  v = verbose;
  clear_tb_data(0,EXP_DATA_ENTRIES,v);

  nop(4);

//  if(verbose) $display("-I: setting initial configuration ");
//  $readmemh("data/dsram0.cfg0.memh",top.dut0.dsram0.ram);
//  $readmemh("data/dsram1.cfg0.memh",top.dut0.dsram1.ram);
//  $readmemh("data/dsram2.cfg0.memh",top.dut0.dsram2.ram);
//  $readmemh("data/dsram3.cfg0.memh",top.dut0.dsram3.ram);
//
//  load_initial_tags("data/tags.cfg0.memh",v);
//  load_initial_bits("data/bits.cfg0.memb",v);
//
//  nop(4);
//
//  //FIXME: there is a problem somewhere, where the readmemh calls
//  //above do not reset index 0 of the lru/dirty bits. I have not found
//  //the problem yet.
//  @(posedge clk);
//  top.dut0.lrurf0.regs[0] = 3'b0;
//  top.dut0.dirty0.regs[0] = 4'b0;
//  @(posedge clk);
//
//
//  //some arbitrary nops inserted just for simple state test
//
//          //tag/way index   word                  --wpWpii
//  rd_req({14'h003,13'h000,3'h3,2'h0},4'b1111,v);//00303000
//  rd_req({14'h001,13'h001,3'h7,2'h0},4'b1111,v);//00107001
//  nop(4);
//  rd_req({14'h002,13'h002,3'h6,2'h0},4'b1111,v);//00206002
//  nop(1);
//  rd_req({14'h000,13'h003,3'h5,2'h0},4'b1111,v);//00005003
//  rd_req({14'h000,13'h004,3'h2,2'h0},4'b1111,v);//00002004
//  nop(4);
//  rd_req({14'h001,13'h001,3'h1,2'h0},4'b1111,v);//00101001
//  rd_req({14'h002,13'h005,3'h5,2'h0},4'b1111,v);//00205005
//  rd_req({14'h003,13'h007,3'h3,2'h0},4'b1111,v);//00303007
//  nop(2);
//  rd_req({14'h000,13'h002,3'h2,2'h0},4'b1111,v);//00002002
//  rd_req({14'h001,13'h003,3'h1,2'h0},4'b1111,v);//00101003
//  rd_req({14'h000,13'h003,3'h6,2'h0},4'b1111,v);//00006003
//  nop(4);
//  rd_req({14'h000,13'h003,3'h6,2'h0},4'b1111,v);//00006003
//  nop(4);
//  rd_req({14'h001,13'h003,3'h7,2'h0},4'b1111,v);//00107003
//  rd_req({14'h003,13'h006,3'h4,2'h0},4'b1111,v);//00304006
//  rd_req({14'h000,13'h006,3'h3,2'h0},4'b1111,v);//00003006
//  nop(1);
//  rd_req({14'h000,13'h002,3'h0,2'h0},4'b1111,v);//00000002
//  rd_req({14'h001,13'h000,3'h1,2'h0},4'b1111,v);//00101000
//
//
//  load_expect_capture_data("./golden/basicRdHit.a.cfg0.memh",
//                           "./golden/basicRdHit.d.cfg0.memh",v);
//  load_expect_tags("./golden/basicRdHit.t.cfg0.memh",v);
//  load_expect_bits("./golden/basicRdHit.b.cfg0.memb",v); //NOTE B file
//
//  nop(4); //let state propagate
//
//  check_tb_capture_info (errs,0,16,v); //EXP_DATA_ENTRIES);
//  check_tb_tags_bits(errs,0,16,v); //EXP_DATA_ENTRIES);

  endTestMsg(testName,errs);
  nop(4);
end
endtask
// --------------------------------------------------------------------------
//  33222222222211 11111111            
//  10987654321098 7654321098765 432-- 
//  ------------TT SSSSSSSSSSSSS wwwxx 
//
// --------------------------------------------------------------------------
//
// LRU rules
// access to way0    b2=0  b1=b1  b0=0
// access to way1    b2=0  b1=b1  b0=1   1 1 0 0 1 1
// access to way2    b2=1  b1=0   b0=b0    
// access to way3    b2=1  b1=1   b0=b0
//
// LRU state starts at 3'b000.
// All accesses are to index 0 ways, in this order:  3,1,2,0,3.
//
//                  previous
// read             b2  b1  b0  | next LRU   | next value
// -----------------------------|------------|-------------
// access way3      0   0   0   | 1   1  b0  |  1 1 0
// access way1      1   1   0   | 0  b1   1  |  0 1 1
// access way2      0   1   1   | 1   0  b0  |  1 0 1
// access way0      1   0   1   | 0  b1   0  |  0 0 0
// access way3      0   0   0   | 1   1  b0  |  1 1 0
//
// ...will result in the LRU at index 0 being 110 
//
// arrays have been prefilled with sequential data/addresses
// so way order in this test is set by last two bits of tag @index 0
//
// This solely an LRU test. It does read/write requests but does not 
// check data only LRU bits, see basicRdHit, and basicWrHit tests for that.
// --------------------------------------------------------------------------
task basicLruTest(inout int errs,input int verbose);
int i;
reg [1:0]  _byte;
reg [2:0]  word;
reg [12:0] index;

reg [31:0] addr;
reg [31:0] incr;
reg [2:0]  act_lru,exp_lru;
string pfx;
integer n,m;
begin
  initTest("basicLruTest",errs);
  clear_tb_data(0,EXP_DATA_ENTRIES,verbose);

  if(verbose) $display("-I: setting initial configuration ");
  $readmemh("data/basicLru.dsram0.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicLru.dsram1.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicLru.dsram2.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicLru.dsram3.memh",top.dut0.dsram3.ram);

  load_initial_tags("data/basicLru.tags.memh",verbose);
  load_initial_bits("data/basicLru.bits.memb",verbose);

  nop(4);

  _byte = 2'b00;
  word  = 3'b000;
  index = 13'h000;

  //some arbitrary nops inserted, I found a case where back to back
  //worked and yet failed when i added a couple nops. good enough for
  //this quick basic test. random testing will fully test this.

  n = 0;
  m = 0;
  //(read) access way 3 of index 0    plru 000 -> 110
  addr  = {14'h003,index,word,_byte};
  rd_req(addr,4'b1111,verbose);
  nop(m);

  //(write) access way 1 of index 0    plru 110 -> 011
  addr  = {14'h001,index,word,_byte};
  wr_req(addr,4'b1111,32'h11111111,verbose);
  nop(m);

  //(read) access way 2 of index 0     plru 011 -> 101
  addr  = {14'h002,index,word,_byte};
  rd_req(addr,4'b1111,verbose);
  nop(n);

  //(write) access way 0 of index 0    plru 101 -> 000
  addr  = {14'h000,index,word,_byte};
  wr_req(addr,4'b1111,32'h22222222,verbose);
  nop(n);

  //(read) access way 2 of index 0     plru 000 -> 100
  addr  = {14'h002,index,word,_byte};
  rd_req(addr,4'b1111,verbose);

  //let final state propagate
  nop(3);

  //Manually verify the resulting LRU bits
  //grab the contents of index 0 in the LRU array
  act_lru = top.dut0.lrurf0.regs[0];
  exp_lru = 3'b100;

  pfx = "-I:";
  if(act_lru !== exp_lru) begin
    pfx = "-E:";
    errs += 1;
  end

  if(errs > 0 || verbose) begin
    $display("%0s basicLruTest : exp:%03b  act:%03b",pfx,exp_lru,act_lru);
  end

  endTestMsg(testName,errs);
  nop(4);
end
endtask
// --------------------------------------------------------------------------
// Read hit test
//
// directed walk of words within the indexes, way = tag due to initialzing
// data
//
// initial lru state is 000
// See golden/basicRdHit.b.cfg0.memb for derivation of expect data
// and README.txt for the LRU update truth table
//
// --------------------------------------------------------------------------
task basicRdHitTest(inout int errs,input int verbose);
integer i,j,mod,lclerrs;
int v;
reg [2:0] lru_exp,lru_act;
reg [31:0] addr;

begin
  initTest("basicRdHitTest",errs);
  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,v);

  nop(4);

  if(verbose) $display("-I: setting initial configuration ");
  $readmemh("data/basicRdHit.dsram0.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicRdHit.dsram1.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicRdHit.dsram2.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicRdHit.dsram3.memh",top.dut0.dsram3.ram);

  load_initial_tags("data/basicRdHit.tags.memh",v);
  load_initial_bits("data/basicRdHit.bits.memb",v);

  nop(4);

  //FIXME: there is a problem somewhere, where the readmemh calls
  //above do not reset index 0 of the lru/dirty bits. I have not found
  //the problem yet.
  @(posedge clk);
  top.dut0.lrurf0.regs[0] = 3'b0;
  top.dut0.dirty0.regs[0] = 4'b0;
  @(posedge clk);


  //some arbitrary nops inserted just for simple state test

          //tag/way index   word                  --wpWpii
  rd_req({14'h003,13'h000,3'h3,2'h0},4'b1111,v);//00303000
  rd_req({14'h001,13'h001,3'h7,2'h0},4'b1111,v);//00107001
  nop(4);
  rd_req({14'h002,13'h002,3'h6,2'h0},4'b1111,v);//00206002
  nop(1);
  rd_req({14'h000,13'h003,3'h5,2'h0},4'b1111,v);//00005003
  rd_req({14'h000,13'h004,3'h2,2'h0},4'b1111,v);//00002004
  nop(4);
  rd_req({14'h001,13'h001,3'h1,2'h0},4'b1111,v);//00101001
  rd_req({14'h002,13'h005,3'h5,2'h0},4'b1111,v);//00205005
  rd_req({14'h003,13'h007,3'h3,2'h0},4'b1111,v);//00303007
  nop(2);
  rd_req({14'h000,13'h002,3'h2,2'h0},4'b1111,v);//00002002
  rd_req({14'h001,13'h003,3'h1,2'h0},4'b1111,v);//00101003
  rd_req({14'h000,13'h003,3'h6,2'h0},4'b1111,v);//00006003
  nop(4);
  rd_req({14'h001,13'h003,3'h7,2'h0},4'b1111,v);//00107003
  rd_req({14'h003,13'h006,3'h4,2'h0},4'b1111,v);//00304006
  rd_req({14'h000,13'h006,3'h3,2'h0},4'b1111,v);//00003006
  nop(1);
  rd_req({14'h000,13'h002,3'h0,2'h0},4'b1111,v);//00000002
  rd_req({14'h001,13'h000,3'h1,2'h0},4'b1111,v);//00101000

  nop(4); //let state propagate

  load_expect_capture_data("./golden/basicRdHit.a.cfg0.memh",
                           "./golden/basicRdHit.d.cfg0.memh",v);
  load_expect_tags("./golden/basicRdHit.t.cfg0.memh",v);
  load_expect_bits("./golden/basicRdHit.b.cfg0.memb",v); //NOTE B file

  nop(4); //let state propagate

  check_tb_capture_info (errs,0,16,v); //EXP_DATA_ENTRIES);
  check_tb_tags_bits(errs,0,16,v); //EXP_DATA_ENTRIES);

  endTestMsg(testName,errs);

  //$display("HERE basicRdHitTest");
  //show_line(0,0);
  nop(4);
end
endtask
// --------------------------------------------------------------------------
// Write hit test - uses same setup and pattern as read hit test
//
// directed walk of words within the indexes, way = tag due to initialzing
// data
//
// way/tag index   word
//  3        0       3
//  1        1       7
//  2        2       6
//  0        3       5
//  0        4       2
//  1        1       1
//  2        5       5
//  3        7       3
//  0        2       2
//  1        3       1
//  0        3       6
//  1        3       7
//  3        6       4
//  0        6       3
//  0        2       0
//  1        0       1
//
// --------------------------------------------------------------------------
task basicWrHitTest(inout int errs,input int verbose);
integer i,j,mod;
reg [1:0]  _byte;
reg [2:0]  word;
reg [12:0] index;
reg [13:0] tag;

reg [31:0] addr;
reg [31:0] incr;
reg [2:0]  act_lru,exp_lru;
int check;
reg v;
begin
  initTest("basicWrHitTest",errs);
  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,v);
  nop(4);

  if(verbose) $display("-I: setting initial configuration ");

  $readmemh("data/basicWrHit.dsramN.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicWrHit.dsramN.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicWrHit.dsramN.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicWrHit.dsramN.memh",top.dut0.dsram3.ram);

  load_initial_tags("data/basicWrHit.tags.memh",v);
  load_initial_bits("data/basicWrHit.bits.memb",v);

  //FIXME: I have not found why the readmemh's above are not working.
  //I see left over state from the previous read test. Uncomment to
  //see what I mean
  //  $display("HERE basicWrHit before explicit re-init 1");
  //  show_line(0,0);
  //  show_line(1,0);
  //  show_line(2,0);
  //  show_line(3,0);
  @(posedge clk);
  top.dut0.dsram0.ram[0] = 256'b0;
  top.dut0.dsram1.ram[0] = 256'b0;
  top.dut0.dsram2.ram[0] = 256'b0;
  top.dut0.dsram3.ram[0] = 256'b0;
  @(posedge clk);
  //  $display("HERE basicWrHit after explicit re-init 1");
  //  show_line(0,0);
  //  show_line(1,0);
  //  show_line(2,0);
  //  show_line(3,0);

//  way  index word be      value
// --------------------------------
//  0,   0,    0,   1111,   11111111 done
//  0,   0,    1,   0001,   00000022 done
//  0,   0,    2,   0010,   00003300 done
//  0,   0,    3,   0100,   00440000 done
//  0,   0,    4,   1000,   55000000 done
//
//  3,   0,    3,   0011,   22221111 done
//  1,   1,    7,   1100,   3333xxxx done
//  2,   2,    6,   0001,   xxxxxx44 done
//  0,   3,    5,   0100,   xx55xxxx done
//  0,   4,    2,   0010,   xxxx66xx done
//  1,   1,    1,   1000,   77xxxxxx done
//  2,   5,    5,   1111,   88888888 done
//  3,   7,    3,   1111,   99999999 done
//  0,   2,    2,   1111,   aaaaaaaa done
//  1,   3,    1,   1111,   bbbbbbbb done
//  0,   3,    6,   1111,   cccccccc done
//  1,   3,    7,   1111,   dddddddd done
//  3,   6,    4,   1111,   eeeeeeee done
//  0,   6,    3,   1111,   ffffffff done
//  0,   2,    0,   1111,   01020304 done
//  1,   0,    1,   1111,   50607080 done

  //some random nops added for some variety in sequences

  //        //tag/way index   word                      --wpWpii  LRU     
  wr_req({14'h000,13'h000,3'h0,2'h0},4'b1111,32'h01111111,v);//i0 000->000
  wr_req({14'h000,13'h000,3'h1,2'h0},4'b0001,32'h00000022,v); //i0 000->000

  wr_req({14'h000,13'h000,3'h2,2'h0},4'b0010,32'h00003300,v); //i0 000->000
  wr_req({14'h000,13'h000,3'h3,2'h0},4'b0100,32'h00440000,v); //i0 000->000
  wr_req({14'h000,13'h000,3'h4,2'h0},4'b1000,32'h05000000,v); //i0 000->000
  nop(1);
  wr_req({14'h000,13'h000,3'h5,2'h0},4'b1111,32'h0CACACAC,v); //i0 000->000
  wr_req({14'h000,13'h000,3'h6,2'h0},4'b1111,32'h0A4367BC,v); //i0 000->000
  wr_req({14'h000,13'h000,3'h7,2'h0},4'b1111,32'h00E1D2C3,v); //i0 000->000

  wr_req({14'h000,13'h001,3'h0,2'h0},4'b1111,32'h11111111,v); //i1 000->000
  wr_req({14'h000,13'h001,3'h1,2'h0},4'b1111,32'h10000022,v); //i1 000->000
  wr_req({14'h000,13'h001,3'h2,2'h0},4'b1111,32'h10003300,v); //i1 000->000
  wr_req({14'h000,13'h001,3'h3,2'h0},4'b1111,32'h10440000,v); //i1 000->000
  nop(1);
  wr_req({14'h000,13'h001,3'h4,2'h0},4'b1111,32'h15000000,v); //i1 000->000
  wr_req({14'h000,13'h001,3'h5,2'h0},4'b1111,32'h1CACACAC,v); //i1 000->000
  wr_req({14'h000,13'h001,3'h6,2'h0},4'b1111,32'h1A4367BC,v); //i1 000->000
  wr_req({14'h000,13'h001,3'h7,2'h0},4'b1111,32'h10E1D2C3,v); //i1 000->000

          //tag/way index   word                      --wpWpii
  wr_req({14'h000,13'h002,3'h0,2'h0},4'b1111,32'h21111111,v); //i2 000->000
  wr_req({14'h000,13'h002,3'h1,2'h0},4'b1111,32'h20000022,v); //i2 000->000
  nop(2);
  wr_req({14'h000,13'h002,3'h2,2'h0},4'b1111,32'h20003300,v); //i2 000->000
  wr_req({14'h000,13'h002,3'h3,2'h0},4'b1111,32'h20440000,v); //i2 000->000
  wr_req({14'h000,13'h002,3'h4,2'h0},4'b1111,32'h25000000,v); //i2 000->000
  wr_req({14'h000,13'h002,3'h5,2'h0},4'b1111,32'h2CACACAC,v); //i2 000->000
  wr_req({14'h000,13'h002,3'h6,2'h0},4'b1111,32'h2A4367BC,v); //i2 000->000
  wr_req({14'h000,13'h002,3'h7,2'h0},4'b1111,32'h20E1D2C3,v); //i2 000->000

  //      //tag/way index   word                      --wpWpii
  wr_req({14'h003,13'h000,3'h3,2'h0},4'b0011,32'h22221111,v); //i0 000->110
  wr_req({14'h001,13'h001,3'h7,2'h0},4'b1100,32'h3333xxxx,v); //i1 000->001
  wr_req({14'h002,13'h002,3'h6,2'h0},4'b0001,32'hxxxxxx44,v); //i2 000->100
  nop(1);
  wr_req({14'h000,13'h003,3'h5,2'h0},4'b0100,32'hxx55xxxx,v); //i3 000->000
  wr_req({14'h000,13'h004,3'h2,2'h0},4'b0010,32'hxxxx66xx,v); //i4 000->000
  wr_req({14'h001,13'h001,3'h1,2'h0},4'b1000,32'h77xxxxxx,v); //i1 001->001
  wr_req({14'h002,13'h005,3'h5,2'h0},4'b1111,32'h88888888,v); //i5 000->100
  wr_req({14'h003,13'h007,3'h3,2'h0},4'b1111,32'h99999999,v); //i7 000->110
  wr_req({14'h000,13'h002,3'h2,2'h0},4'b1111,32'haaaaaaaa,v); //i2 100->000
  wr_req({14'h001,13'h003,3'h1,2'h0},4'b1111,32'hbbbbbbbb,v); //i3 000->001
  wr_req({14'h000,13'h003,3'h6,2'h0},4'b1111,32'hcccccccc,v); //i3 001->000
  nop(2);
  wr_req({14'h001,13'h003,3'h7,2'h0},4'b1111,32'hdddddddd,v); //i3 000->001
  wr_req({14'h003,13'h006,3'h4,2'h0},4'b1111,32'heeeeeeee,v); //i6 000->110
  wr_req({14'h000,13'h006,3'h3,2'h0},4'b1111,32'hffffffff,v); //i6 110->010
  wr_req({14'h000,13'h002,3'h0,2'h0},4'b1111,32'h01020304,v); //i2 000->000
  wr_req({14'h001,13'h000,3'h1,2'h0},4'b1111,32'h50607080,v); //i0 110->011
  nop(1);

  load_expect_dary_data("./golden/basicWrHit.d0.cfg1.memh",
                        "./golden/basicWrHit.d1.cfg1.memh",
                        "./golden/basicWrHit.d2.cfg1.memh",
                        "./golden/basicWrHit.d3.cfg1.memh",v);

  load_expect_tags("./golden/basicWrHit.t.cfg1.memh",v);
  load_expect_bits("./golden/basicWrHit.b.cfg1.memb",v); //NOTE B file

  nop(4);

  check_data_arrays (errs,0,16,v); 
  check_tb_tags_bits(errs,0,16,v);

  endTestMsg(testName,errs);
  nop(4);
end
endtask
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
task initState;
begin
  testName = "initState";
  tb_cc_ram_test = 1'b0;
end
endtask
