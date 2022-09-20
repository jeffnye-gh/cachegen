// vi:ft=verilog:
`ifndef POSCLK 
`define POSCLK 
//`define POSCLK @(posedge clk)
`endif
// -----------------------------------------------------------------
`include "sim_cntrl.h"
// =================================================================
// TASKS
// =================================================================
task rd_req(input [31:0] a,input [3:0] be,input verbose=0);
begin
  if(verbose) $display("-I: rd req : a:%08x  be:%04b",a,be);
  top.tb_cc_address    = a;
  top.tb_cc_byteenable = be;
  top.tb_cc_read       = 1'b1;
  top.tb_cc_write      = 1'b0;
  @(posedge clk);
  //#1 so the signal has enough hold for the simulator to catch it
//  top.tb_cc_read       = #1 1'b0;
//  top.tb_cc_write      = #1 1'b0;
//  top.tb_cc_address    = #1 31'bx;
end
endtask
// --------------------------------------------------------------------------
task wr_req(input [31:0] a,input [3:0] be,input [31:0] wd,input verbose=0);
begin
  if(verbose) $display("-I: wr req : a:%08x  be:%04b",a,be);
  top.tb_cc_address    = a;
  top.tb_cc_byteenable = be;
  top.tb_cc_read       = 1'b0;
  top.tb_cc_write      = 1'b1;
  top.tb_cc_writedata  = wd;
  @(posedge clk);
//  top.tb_cc_read       = #1 1'b0;
//  top.tb_cc_write      = #1 1'b0;
//  top.tb_cc_address    = #1 31'bx;
end
endtask
// --------------------------------------------------------------------------
// Loads the 11bit memb (NOTE: B)  file and converts the entries to 
// 4 valid bits, 4 dirty bits and 3 lru bits init data
// -----------------------------------------------------------------
task show_bit_state(input int cnt,input int verbose=0);
int i;
begin
  if(verbose) $display("-I: show_bit_state");
  for(i=0;i<cnt;i=i+1) begin
    $display("%0d : v:%04b m:%04b lru:%03b",
      i,
      top.dut0.bits0.vbits[i],
      top.dut0.bits0.mbits[i],
      top.dut0.bits0.lbits[i]);
  end
  //`POSCLK; //@(posedge clk);
end
endtask

// -----------------------------------------------------------------
// Loads the 11bit memb (NOTE: B)  file and converts the entries to 
// 4 valid bits, 4 dirty bits and 3 lru bits init data
// -----------------------------------------------------------------
task clear_initial_bits(input int verbose=0);
int i;
begin
  for(i=0;i<EXP_DATA_ENTRIES;i=i+1) begin
    top.dut0.bits0.lbits[i] = 3'bx;
    top.dut0.bits0.mbits[i] = 4'bx;
    top.dut0.bits0.vbits[i] = 4'bx; 
  end
  `POSCLK; //@(posedge clk);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task load_initial_bits(input string fn,input int verbose=0);
integer i;
reg [10:0] local_bits[EXP_DATA_ENTRIES];
begin
  if(verbose) $display("-I: loading initial bit data from %0s",fn);
  //clear_tmp_data();
  $readmemb(fn,local_bits);
  $display("HERE lb %011b",local_bits[0]);
  for(i=0;i<EXP_DATA_ENTRIES;i=i+1) begin
    top.dut0.bits0.lbits[i] = local_bits[i][2:0];
    if(i==0) $display("HERE 2 lb %03b",local_bits[i][2:0]);
    if(i==0) $display("HERE 3 lb %03b",top.dut0.bits0.lbits[i]);
    top.dut0.bits0.mbits[i] = local_bits[i][6:3];
    top.dut0.bits0.vbits[i] = local_bits[i][10:7];
  end
  $display("HERE 4 lb %03b",top.dut0.bits0.lbits[i]);
  show_bit_state(4,1);
  `POSCLK; //@(posedge clk);
  $display("HERE 5 lb %03b",top.dut0.bits0.lbits[i]);
end
endtask
// -----------------------------------------------------------------
// Loads the 64bit memh file and converts the entries to 4x14b tag
// initialization data
// -----------------------------------------------------------------
task load_initial_tags(input string fn,input int verbose=0);
integer i;
reg  [63:0]  tmp_data[0:EXP_DATA_ENTRIES];
begin
  if(verbose) $display("-I: loading expect tags");
  $readmemh(fn,tmp_data);
  for(i=0;i<EXP_DATA_ENTRIES;i=i+1) begin
    top.dut0.tags[0].tag.ram[i] = tmp_data[i][13:0];
    top.dut0.tags[1].tag.ram[i] = tmp_data[i][29:16];
    top.dut0.tags[2].tag.ram[i] = tmp_data[i][45:32];
    top.dut0.tags[3].tag.ram[i] = tmp_data[i][61:48];
  end
  `POSCLK; //@(posedge clk);
end
endtask
// -----------------------------------------------------------------
// Loads the 64bit memh file and converts the entries to 4x14b tag
// expect data
// -----------------------------------------------------------------
task load_expect_tags(input string fn,input int verbose=0);
integer i;
reg [13:0]  tags[0:3];
reg [63:0]  tmp_data[0:EXP_DATA_ENTRIES];
begin
  if(verbose) $display("-I: loading expect tags");
  $readmemh(fn,tmp_data);
  for(i=0;i<EXP_DATA_ENTRIES;i=i+1) begin
    tags[0] = tmp_data[i][13:0];
    tags[1] = tmp_data[i][29:16];
    tags[2] = tmp_data[i][45:32];
    tags[3] = tmp_data[i][61:48];
    mm_expect_tags[i] = {tags[3],tags[2],tags[1],tags[0]};
  end
  `POSCLK; //@(posedge clk);
end
endtask
// -----------------------------------------------------------------
// NOTE readmem B
// -----------------------------------------------------------------
task load_expect_bits(input string fn,input int verbose=0);
begin
  if(verbose) $display("-I: loading expect bits");
  $readmemb(fn,mm_expect_bits);
  `POSCLK; //@(posedge clk);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task load_expect_data(input string afn,input string dfn,input int verbose=0);
begin
  if(verbose) $display("-I: loading expect data");
  $readmemh(afn,top.mm_expect_addr);
  $readmemh(dfn,top.mm_expect_data);
  `POSCLK; //@(posedge clk);
end
endtask
// -----------------------------------------------------------------
// Separate from clear_tb_data() because this is called multiple
// times when converting files
// -----------------------------------------------------------------
//task clear_tmp_data(input int verbose=0);
//integer i;
//begin
//  if(verbose) $display("-I: clearing tb temp data");
//  for(i=0;i<EXP_DATA_ENTRIES;i=i+1) begin
//    top.mm_tmp_data[i]     = 128'bx;
//  end
//  `POSCLK; //@(posedge clk);
//end
//endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task clear_tb_data(input int start,input int stop,input int verbose=0);
integer i;
begin
  if(verbose) $display("-I: clearing expect, capture and temp data");
  capture_a_index = 0;
  capture_d_index = 0;
  for(i=start;i<stop;i=i+1) begin

    top.mm_expect_addr[i]  = 32'bx;
    top.mm_expect_data[i]  = 32'bx;

    top.mm_capture_addr[i] = 32'bx;
    top.mm_capture_data[i] = 32'bx;

    top.mm_expect_tags[i]  =  56'bx;
    top.mm_expect_bits[i]  =  11'bx;

  end
  `POSCLK; //@(posedge clk);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task check_tb_add_data(output integer errs,input int start,stop,verbose=0);
integer i;
reg matcha,matchd,match;
begin
  //do not clear errs, this should be done by the caller
  if(verbose) $display("-I: showing addr/data expect values:");
  if(start > EXP_DATA_ENTRIES || stop > EXP_DATA_ENTRIES) begin
    $display("-E: in show_tb_add_data(), array range exceeded");
  end else begin
    for(i=start;i<stop;i+=1) begin
      matcha = compare(top.mm_expect_addr[i],top.mm_capture_addr[i]);
      matchd = compare(top.mm_expect_data[i],top.mm_capture_data[i]);
      match  = matcha & matchd;
      if(!match) errs = errs + 1;
      $display("-I: %04d exp:%08x:%08x  act:%08x:%08x %01b",i,
               top.mm_expect_addr[i], top.mm_expect_data[i],
               top.mm_capture_addr[i],top.mm_capture_data[i],match);
    end
    //$display("-I: ERRORS : %04d",errs);
  end
  `POSCLK; //@(posedge clk);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task check_tb_tags_bits(output int errs,input int start,stop,verbose=0);
integer i;
reg matcht,matchb,match;
reg [55:0] local_tags[0:EXP_DATA_ENTRIES];
reg [10:0] local_bits[0:EXP_DATA_ENTRIES];
begin
  //do not clear errs, this should be done by the caller
  if(verbose) $display("-I: showing tag/bits expect values:");
  if(start > EXP_DATA_ENTRIES || stop > EXP_DATA_ENTRIES) begin
    $display("-E: in show_tb_tags_bits(), array range exceeded");
  end else begin
    for(i=start;i<stop;i+=1) begin
      local_tags[i] = { top.dut0.tags[3].tag.ram[i],
                        top.dut0.tags[2].tag.ram[i],
                        top.dut0.tags[1].tag.ram[i],
                        top.dut0.tags[0].tag.ram[i] };
      local_bits[i] = { top.dut0.bits0.vbits[i],
                        top.dut0.bits0.mbits[i],
                        top.dut0.bits0.lbits[i] };
    end
    for(i=start;i<stop;i+=1) begin
      matcht = compare(top.mm_expect_tags[i],local_tags[i]);
      matchb = compare(top.mm_expect_bits[i],local_bits[i]);
      match  = matcht & matchb;
      if(!match) errs = errs + 1;
      $display("-I: %02d exp:%03x %03x %03x %03x : %04b %04b %03b : %1b",
        i,
        top.mm_expect_tags[i][55:42],
        top.mm_expect_tags[i][41:28],
        top.mm_expect_tags[i][27:14],
        top.mm_expect_tags[i][13:0],
        top.mm_expect_bits[i][10:7],
        top.mm_expect_bits[i][6:3],
        top.mm_expect_bits[i][2:0],match);

      $display("-I: %02d act:%03x %03x %03x %03x : %04b %04b %03b : %1b\n",
        i,
        local_tags[i][55:42],
        local_tags[i][41:28],
        local_tags[i][27:14],
        local_tags[i][13:0],
        local_bits[i][10:7],
        local_bits[i][6:3],
        local_bits[i][2:0],match);
    end
    //$display("-I: ERRORS : %04d",errs);
  end
  `POSCLK; //@(posedge clk);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task nop(input int count);
integer i;
begin
  for(i=0;i<count;i=i+1) begin
    tb_cmd = TB_CMD_NOP;
    tb_cc_read     = 1'b0;
    tb_cc_write    = 1'b0;
    tb_cc_ram_test = 1'b0;
    @(posedge clk);
  end
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task terminate;
string msg,prefix;
begin

  $display("-I: basic lru errors    %0d",lru_errs);
  $display("-I: basic rd hit errors %0d",basic_rd_hit_errs);
  $display("-I: basic wr hit errors %0d",basic_wr_hit_errs);
  imsg("CACHE SIM END");
  prefix = "-I: ";
  msg = "PASS";
  if(lru_errs + basic_rd_hit_errs + basic_wr_hit_errs > 0) begin
    prefix = "-E: ";
    msg    = "FAIL";
  end
  $display("%sTERMINATE %s",prefix,msg);
  tb_cmd = TB_CMD_TERM;
  @(posedge clk);
  @(posedge clk);
  $finish;
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task beginTestMsg(input string testName);
begin
  $display("-I: BEGIN TEST : %0s",testName);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task endTestMsg(input string testName,input int errs);
string pf,pre;
begin
  if(errs > 0) begin pf = "FAIL"; pre = "-E: "; end
  else         begin pf = "PASS"; pre = "-I: "; end
  $display("%0s END TEST   : %0s : errors %0d : %0s",pre,testName,errs,pf);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task imsg;
input [50*8-1:0] m;
begin
  $display("-I: %0s",m);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task emsg;
input [50*8-1:0] m;
begin
  $display("-E: %0s",m);
end
endtask
