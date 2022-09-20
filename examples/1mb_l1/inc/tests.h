// vi:syntax=verilog
`include "sim_cntrl.h"
`include "bitcmds.h"
`include "functions.h"
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
task basicLruTest(output int errs,input int verbose);
int i;
reg [1:0]  _byte;
reg [2:0]  word;
reg [12:0] index;

reg [31:0] addr;
reg [31:0] incr;
reg [2:0]  act_lru,exp_lru;
begin
  errs = 0;

  $display("-I: clearing test bench data");
  clear_tb_data(0,EXP_DATA_ENTRIES,1);

  //@(posedge clk);
  $display("-I: setting initial configuration ");
  $readmemh("data/dsram0.cfg0.memh",top.dut0.data[0].dsram.ram);
  $readmemh("data/dsram1.cfg0.memh",top.dut0.data[1].dsram.ram);
  $readmemh("data/dsram2.cfg0.memh",top.dut0.data[2].dsram.ram);
  $readmemh("data/dsram3.cfg0.memh",top.dut0.data[3].dsram.ram);
  load_initial_tags("data/tags.cfg0.memh");
  load_initial_bits("data/bits.cfg0.memb",1);
  show_bit_state(4,1);

  testName = "basicLruTest";
  beginTestMsg(testName);

  _byte = 2'b00;
  word  = 3'b000;
  index = 13'h000;

  //some arbitrary nops inserted, I found a case where back to back
  //worked and yet failed when i added a couple nops. good enough for
  //this quick basic test. random testing will fully test this.

  //(read) access way 3 of index 0    plru 000 -> 110
  addr  = {14'h003,index,word,_byte};
  rd_req(addr,4'b1111,verbose);
  nop(1);

  //(write) access way 1 of index 0    plru 110 -> 011
  addr  = {14'h001,index,word,_byte};
  wr_req(addr,4'b1111,32'h11111111,verbose);

  //(read) access way 2 of index 0     plru 110 -> 101
  addr  = {14'h002,index,word,_byte};
  rd_req(addr,4'b1111,verbose);
  nop(1);

  //(write) access way 0 of index 0    plru 101 -> 000
  addr  = {14'h000,index,word,_byte};
  wr_req(addr,4'b1111,32'h22222222,verbose);

  //(read) access way 3 of index 0     plru 000 -> 011
  addr  = {14'h003,index,word,_byte};
  rd_req(addr,4'b1111,verbose);

  //let final state propagate
  nop(3);

  //Manually verify the resulting LRU bits
  //grab the contents of index 0 in the LRU array
  act_lru = top.dut0.bits0.lbits[0];
  exp_lru = 3'b110;

  if(verbose) $display("-I: basicLruTest : exp:%03b  act:%03b",exp_lru,act_lru);

  if(exp_lru === act_lru) errs = 0;
  else errs = 1;

  nop(4);

  endTestMsg(testName,errs);
end
endtask
// --------------------------------------------------------------------------
// Read hit test
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
task basicRdHitTest(output int errs,input int verbose);
integer i,j,mod;
int v;
begin
  testName = "basicRdHitTest";
  beginTestMsg(testName);

  errs = 0;
  v = verbose;

  $display("-I: clearing test bench data");
  clear_tb_data(0,EXP_DATA_ENTRIES,1);

  $display("-I: setting initial configuration ");
  $readmemh("data/dsram0.cfg0.memh",top.dut0.data[0].dsram.ram);
  $readmemh("data/dsram1.cfg0.memh",top.dut0.data[1].dsram.ram);
  $readmemh("data/dsram2.cfg0.memh",top.dut0.data[2].dsram.ram);
  $readmemh("data/dsram3.cfg0.memh",top.dut0.data[3].dsram.ram);
  load_initial_tags("data/tags.cfg0.memh");
  load_initial_bits("data/bits.cfg0.memb",1);
  //FIXME: these two lines are necessary to make the test pass if it is run
  //after LRU test. There is a race somewhere, find it.
//  @(posedge clk);
  @(posedge clk);
  top.dut0.bits0.lbits[0] = 3'b0;
  @(posedge clk);
  show_bit_state(4,1);
  //show_initial_bits();

        //tag/way index   word                      --wpWpii
  rd_req({14'h003,13'h000,3'h3,2'h0},4'b1111,v); // 00303000 way3  000 ->
  nop(1);
  rd_req({14'h001,13'h001,3'h7,2'h0},4'b1111,v); // 00107001 way1  000 ->
  rd_req({14'h002,13'h002,3'h6,2'h0},4'b1111,v); // 00206002 way2  000 ->
  rd_req({14'h000,13'h003,3'h5,2'h0},4'b1111,v); // 00005003 way0  000 ->
  nop(1);
  rd_req({14'h000,13'h004,3'h2,2'h0},4'b1111,v); // 00002004 way0
  nop(1);
  rd_req({14'h001,13'h001,3'h1,2'h0},4'b1111,v); // 00101001 way1
  nop(3);
  rd_req({14'h002,13'h005,3'h5,2'h0},4'b1111,v); // 00205005 way2
  rd_req({14'h003,13'h007,3'h3,2'h0},4'b1111,v); // 00303007 way3
  rd_req({14'h000,13'h002,3'h2,2'h0},4'b1111,v); // 00002002 way0
  rd_req({14'h001,13'h003,3'h1,2'h0},4'b1111,v); // 00101003 way1
  rd_req({14'h000,13'h003,3'h6,2'h0},4'b1111,v); // 00006003 way0
  rd_req({14'h001,13'h003,3'h7,2'h0},4'b1111,v); // 00107003 way1
  rd_req({14'h003,13'h006,3'h4,2'h0},4'b1111,v); // 00304006 way3
  nop(1);
  rd_req({14'h000,13'h006,3'h3,2'h0},4'b1111,v); // 00003006 way0
  rd_req({14'h000,13'h002,3'h0,2'h0},4'b1111,v); // 00000002 way0
  nop(1);
  rd_req({14'h001,13'h000,3'h1,2'h0},4'b1111,v); // 00101000 way1

  nop(5);

  load_expect_data("./golden/basicRdHit.a.cfg0.memh",
                   "./golden/basicRdHit.d.cfg0.memh",1);
  load_expect_tags("./golden/basicRdHit.t.cfg0.memh",1);
  load_expect_bits("./golden/basicRdHit.b.cfg0.memb",1); //NOTE B file

  check_tb_add_data (errs,0,16,1); //EXP_DATA_ENTRIES);
  check_tb_tags_bits(errs,0,16,1); //EXP_DATA_ENTRIES);

  endTestMsg(testName,errs);
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
task basicWrHitTest(output int errs,input int verbose);
integer i,j,mod;
reg [1:0]  _byte;
reg [2:0]  word;
reg [12:0] index;
reg [13:0] tag;

reg [31:0] addr;
reg [31:0] incr;
reg [2:0]  act_lru,exp_lru;
reg v;
begin
  testName = "basicRdHitTest";
  beginTestMsg(testName);

  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,1);

        //tag/way index   word                      --wpWpii
  wr_req({14'h003,13'h000,3'h3,2'h0},4'b0011,32'h22221111,v); // 00303000
  wr_req({14'h001,13'h001,3'h7,2'h0},4'b1100,32'h3333xxxx,v); // 00107001
  wr_req({14'h002,13'h002,3'h6,2'h0},4'b0001,32'hxxxxxx44,v); // 00206002
  wr_req({14'h000,13'h003,3'h5,2'h0},4'b0100,32'hxx55xxxx,v); // 00005003
  wr_req({14'h000,13'h004,3'h2,2'h0},4'b0010,32'hxxxx66xx,v); // 00002004
  wr_req({14'h001,13'h001,3'h1,2'h0},4'b1000,32'h77xxxxxx,v); // 00101001
  wr_req({14'h002,13'h005,3'h5,2'h0},4'b1111,32'h88888888,v); // 00205005
  wr_req({14'h003,13'h007,3'h3,2'h0},4'b1111,32'h99999999,v); // 00303007
  wr_req({14'h000,13'h002,3'h2,2'h0},4'b1111,32'haaaaaaaa,v); // 00002002
  wr_req({14'h001,13'h003,3'h1,2'h0},4'b1111,32'hbbbbbbbb,v); // 00101003
  wr_req({14'h000,13'h003,3'h6,2'h0},4'b1111,32'hcccccccc,v); // 00006003
  wr_req({14'h001,13'h003,3'h7,2'h0},4'b1111,32'hdddddddd,v); // 00107003
  wr_req({14'h003,13'h006,3'h4,2'h0},4'b1111,32'heeeeeeee,v); // 00304006
  wr_req({14'h000,13'h006,3'h3,2'h0},4'b1111,32'hffffffff,v); // 00003006
  wr_req({14'h000,13'h002,3'h0,2'h0},4'b1111,32'h01020304,v); // 00000002
  wr_req({14'h001,13'h000,3'h1,2'h0},4'b1111,32'h50607080,v); // 00101000
  nop(2);

//  load_expect_data("./golden/basicWrHit.a.cfg0.memh",
//                   "./golden/basicWrHit.d.cfg0.memh",1);
//  load_expect_tags("./golden/basicWrHit.t.cfg0.memh",1);
  //LRU bits are not verified here - FIXME: maybe add expect data for bits
  //load_expect_bits("./golden/basicRdHit.b.cfg0.memb",1); //NOTE B file

//  show_tb_add_data (0,16,1); //EXP_DATA_ENTRIES);
//  show_tb_tags_bits(0,16,1); //EXP_DATA_ENTRIES);
  errs = 1;
end
endtask
// --------------------------------------------------------------------------
// FIXME: this test is incomplete and also needs self checking
// --------------------------------------------------------------------------
// Assert the ram test signal
//   this reinterprets the address inputs to this:
//
//  33222222222211 11111111            
//  10987654321098 7654321098765 432-- 
//  ------------TT SSSSSSSSSSSSS xxxxx 
//
//  Data is 14bits wide, cache.wd[13:0], 
//    so tag select is derived from a[19:18]
//  Index is 13 bits wide derived from 17:5;
//
//  The tag select is derived from a[19:18]
//  Control lines read/write directly access the tag sram's
//
//  walk each tag in order, writing  incrementing value to each location
// --------------------------------------------------------------------------
task tagRwTest;
input integer maxCount;
integer i,j;
reg [11:0]  ax;
reg [1:0]   ta;
reg [12:0]  sidx;
begin
  testName = "tagRwTest";
  ax = 12'bx;
  for(j=0;j<4;++j) begin
    ta = j[1:0];
    for(i=0;i<maxCount;++i) begin
      sidx = i[12:0];
      tb_cmd          = TB_CMD_NOP;
      tb_cc_ram_test  = 1'b1;
      tb_cc_address   = { ax,ta,sidx,5'b0 };
      tb_cc_writedata = ~i[13:0];
      tb_cc_write     = 1'b1;
      tb_cc_read      = 1'b0;
      @(posedge clk);
    end
  end
  tb_cmd          = TB_CMD_NOP;
  tb_cc_ram_test  = 1'b0;
  tb_cc_write     = 1'b0;
  tb_cc_read      = 1'b0;
end
endtask
// --------------------------------------------------------------------------
// FIXME: this test is incomplete and also needs self checking
// --------------------------------------------------------------------------
// In bypass mode these fields control where the data goes
//
//  3322222222221111111111           bbbb
//  109876543210987654321098765432-- 3210
//  iiiiiiiiiiiiiiiiiAAAAAAAAAAWWW   bbbb
// --------------------------------------------------------------------------
task bypassTest;
input integer count;
integer i,j,err;
reg  [1:0]  ba; // byte address not used
reg  [8:0]  ea; // row/entry address
reg  [3:0]  wa; // word address
reg  [16:0] xa; // unused address bits
begin
  testName = "bypassTest";
  xa = 27'bx;
  ba =  2'bx;

  for(i=0;i<count;i=i+1) begin
    tb_cmd               = TB_CMD_BYPASS;
    tb_cc_address        = {xa,ea,wa,ba}; //32'h00000000;
    tb_cc_byteenable     = 4'hF;
    tb_cc_write          = 1'b1;
    tb_cc_read           = 1'b0;
    tb_cc_wide_writedata = { 224'bx,28'h0000000,i[3:0] };
    @(posedge clk);
    tb_cc_wide_writedata = 256'bx;
  end
end
endtask
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
task initState;
begin
  testName = "initState";
  tb_cc_ram_test = 1'b0;
  nop(5);
end
endtask
