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
  //show_bit_state(4,1);

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
  act_lru = top.dut0.lrurf0.regs[0];
  exp_lru = 3'b110;

  if(verbose) begin
    $display("-I: basicLruTest : exp:%03b  act:%03b",exp_lru,act_lru);
  end

//
//  if(exp_lru === act_lru) errs = 0;
//  else errs = 1;
//
//  nop(4);

  endTestMsg(testName,errs);
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
  testName = "basicRdHitTest";
  beginTestMsg(testName);

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
  @(posedge clk);
  top.dut0.lrurf0.regs[0] = 3'b0;
  @(posedge clk);
  //show_bit_state(4,1);

          //tag/way index   word                  --wpWpii
  rd_req({14'h003,13'h000,3'h3,2'h0},4'b1111,v);//00303000
  rd_req({14'h001,13'h001,3'h7,2'h0},4'b1111,v);//00107001
  rd_req({14'h002,13'h002,3'h6,2'h0},4'b1111,v);//00206002
  rd_req({14'h000,13'h003,3'h5,2'h0},4'b1111,v);//00005003
  rd_req({14'h000,13'h004,3'h2,2'h0},4'b1111,v);//00002004
  rd_req({14'h001,13'h001,3'h1,2'h0},4'b1111,v);//00101001
  rd_req({14'h002,13'h005,3'h5,2'h0},4'b1111,v);//00205005
  rd_req({14'h003,13'h007,3'h3,2'h0},4'b1111,v);//00303007
  rd_req({14'h000,13'h002,3'h2,2'h0},4'b1111,v);//00002002
  rd_req({14'h001,13'h003,3'h1,2'h0},4'b1111,v);//00101003
  rd_req({14'h000,13'h003,3'h6,2'h0},4'b1111,v);//00006003
  rd_req({14'h001,13'h003,3'h7,2'h0},4'b1111,v);//00107003
  rd_req({14'h003,13'h006,3'h4,2'h0},4'b1111,v);//00304006
  rd_req({14'h000,13'h006,3'h3,2'h0},4'b1111,v);//00003006
  rd_req({14'h000,13'h002,3'h0,2'h0},4'b1111,v);//00000002
  rd_req({14'h001,13'h000,3'h1,2'h0},4'b1111,v);//00101000

  load_expect_capture_data("./golden/basicRdHit.a.cfg0.memh",
                           "./golden/basicRdHit.d.cfg0.memh",1);
  load_expect_tags("./golden/basicRdHit.t.cfg0.memh",1);
  load_expect_bits("./golden/basicRdHit.b.cfg0.memb",1); //NOTE B file

  @(posedge clk);
  @(posedge clk);
  @(posedge clk);
  @(posedge clk);

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
  testName = "basicWrHitTest";
  beginTestMsg(testName);

  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,1);
  //Same zero'd image in each data way
  $readmemh("data/dsramN.cfg1.memh",top.dut0.data[0].dsram.ram);
  $readmemh("data/dsramN.cfg1.memh",top.dut0.data[1].dsram.ram);
  $readmemh("data/dsramN.cfg1.memh",top.dut0.data[2].dsram.ram);
  $readmemh("data/dsramN.cfg1.memh",top.dut0.data[3].dsram.ram);
  load_initial_tags("data/tags.cfg1.memh");
  load_initial_bits("data/bits.cfg1.memb",1);
  load_initial_bits("data/bits.cfg1.memb",1);

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

//        //tag/way index   word                      --wpWpii  LRU     
wr_req({14'h000,13'h000,3'h0,2'h0},4'b1111,32'h01111111,v);//i0 000->000
wr_req({14'h000,13'h000,3'h1,2'h0},4'b0001,32'h00000022,v); //i0 000->000

wr_req({14'h000,13'h000,3'h2,2'h0},4'b0010,32'h00003300,v); //i0 000->000
wr_req({14'h000,13'h000,3'h3,2'h0},4'b0100,32'h00440000,v); //i0 000->000
wr_req({14'h000,13'h000,3'h4,2'h0},4'b1000,32'h05000000,v); //i0 000->000
wr_req({14'h000,13'h000,3'h5,2'h0},4'b1111,32'h0CACACAC,v); //i0 000->000
wr_req({14'h000,13'h000,3'h6,2'h0},4'b1111,32'h0A4367BC,v); //i0 000->000
wr_req({14'h000,13'h000,3'h7,2'h0},4'b1111,32'h00E1D2C3,v); //i0 000->000

wr_req({14'h000,13'h001,3'h0,2'h0},4'b1111,32'h11111111,v); //i1 000->000
wr_req({14'h000,13'h001,3'h1,2'h0},4'b1111,32'h10000022,v); //i1 000->000
wr_req({14'h000,13'h001,3'h2,2'h0},4'b1111,32'h10003300,v); //i1 000->000
wr_req({14'h000,13'h001,3'h3,2'h0},4'b1111,32'h10440000,v); //i1 000->000
wr_req({14'h000,13'h001,3'h4,2'h0},4'b1111,32'h15000000,v); //i1 000->000
wr_req({14'h000,13'h001,3'h5,2'h0},4'b1111,32'h1CACACAC,v); //i1 000->000
wr_req({14'h000,13'h001,3'h6,2'h0},4'b1111,32'h1A4367BC,v); //i1 000->000
wr_req({14'h000,13'h001,3'h7,2'h0},4'b1111,32'h10E1D2C3,v); //i1 000->000

        //tag/way index   word                      --wpWpii
wr_req({14'h000,13'h002,3'h0,2'h0},4'b1111,32'h21111111,v); //i2 000->000
wr_req({14'h000,13'h002,3'h1,2'h0},4'b1111,32'h20000022,v); //i2 000->000
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
wr_req({14'h000,13'h003,3'h5,2'h0},4'b0100,32'hxx55xxxx,v); //i3 000->000
wr_req({14'h000,13'h004,3'h2,2'h0},4'b0010,32'hxxxx66xx,v); //i4 000->000
wr_req({14'h001,13'h001,3'h1,2'h0},4'b1000,32'h77xxxxxx,v); //i1 001->001
wr_req({14'h002,13'h005,3'h5,2'h0},4'b1111,32'h88888888,v); //i5 000->100
wr_req({14'h003,13'h007,3'h3,2'h0},4'b1111,32'h99999999,v); //i7 000->110
wr_req({14'h000,13'h002,3'h2,2'h0},4'b1111,32'haaaaaaaa,v); //i2 100->000
wr_req({14'h001,13'h003,3'h1,2'h0},4'b1111,32'hbbbbbbbb,v); //i3 000->001
wr_req({14'h000,13'h003,3'h6,2'h0},4'b1111,32'hcccccccc,v); //i3 001->000
wr_req({14'h001,13'h003,3'h7,2'h0},4'b1111,32'hdddddddd,v); //i3 000->001
wr_req({14'h003,13'h006,3'h4,2'h0},4'b1111,32'heeeeeeee,v); //i6 000->110
wr_req({14'h000,13'h006,3'h3,2'h0},4'b1111,32'hffffffff,v); //i6 110->010
wr_req({14'h000,13'h002,3'h0,2'h0},4'b1111,32'h01020304,v); //i2 000->000
wr_req({14'h001,13'h000,3'h1,2'h0},4'b1111,32'h50607080,v); //i0 110->011
  nop(2);

  load_expect_dary_data("./golden/basicWrHit.d0.cfg1.memh",
                        "./golden/basicWrHit.d1.cfg1.memh",
                        "./golden/basicWrHit.d2.cfg1.memh",
                        "./golden/basicWrHit.d3.cfg1.memh",1);

  load_expect_tags("./golden/basicWrHit.t.cfg1.memh",1);
  load_expect_bits("./golden/basicWrHit.b.cfg1.memb",1); //NOTE B file

  @(posedge clk);
  @(posedge clk);
  @(posedge clk);
  @(posedge clk);

  check_data_arrays (errs,0,16,0); 
  check_tb_tags_bits(errs,0,16,1);
  endTestMsg(testName,errs);
//  show_tb_add_data (0,16,1); //EXP_DATA_ENTRIES);
//  show_tb_tags_bits(0,16,1); //EXP_DATA_ENTRIES);
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