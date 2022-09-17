// vi:ft=verilog:
// -----------------------------------------------------------------
`include "sim_cntrl.h"
// =================================================================
// TASKS
// =================================================================
task rd_req(input [31:0] a,input [3:0] be,input verbose=0);
begin
  if(verbose) $display("-I: rd req : a:%08x  be:%04b",a,be);
  top.tb_cc_address = a;
  top.tb_cc_byteenable = be;
  top.tb_cc_read       = 1'b1;
  top.tb_cc_write      = 1'b0;
  @(posedge clk);
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
end
endtask
// --------------------------------------------------------------------------
//                                     bbbb
//  33222222222211 11111111            eeee
//  10987654321098 7654321098765 432-- 3210
//  TTTTTTTTTTTTTT SSSSSSSSSSSSS ooo-- bbbb
//
//  index 13'h0 way0:tag: 14'h0  ea: {14'h0,13'h0,word,byte} 
//  index 13'h0 way1:tag: 14'h1  ea: {14'h1,13'h0,word,byte}
//  index 13'h0 way2:tag: 14'h2  ea: {14'h2,13'h0,word,byte}
//  index 13'h0 way3:tag: 14'h3  ea: {14'h3,13'h0,word,byte}
//
//  index 13'h1 way0:tag: 14'h4  ea: {14'h4,13'h1,word,byte}
//  index 13'h1 way1:tag: 14'h5  ea: {14'h5,13'h1,word,byte}
//  index 13'h1 way2:tag: 14'h6  ea: {14'h6,13'h1,word,byte}
//  index 13'h1 way3:tag: 14'h7  ea: {14'h7,13'h1,word,byte}
//
//  index 13'h2 way0:tag: 14'h8  ea: {14'h8,13'h2,word,byte}
//  index 13'h2 way1:tag: 14'h9  ea: {14'h9,13'h2,word,byte}
//  index 13'h2 way2:tag: 14'ha  ea: {14'ha,13'h2,word,byte}
//  index 13'h2 way3:tag: 14'hb  ea: {14'hb,13'h2,word,byte}
//
//  index 13'h3 way0:tag: 14'hc  ea: {14'hc,13'h3,word,byte}
//  index 13'h3 way1:tag: 14'hd  ea: {14'hd,13'h3,word,byte}
//  index 13'h3 way2:tag: 14'he  ea: {14'he,13'h3,word,byte}
//  index 13'h3 way3:tag: 14'hf  ea: {14'hf,13'h3,word,byte}
// --------------------------------------------------------------------------
//     33222222222211 11111111           
//     10987654321098 7654321098765 432--
//     TTTTTTTTTTTTTT SSSSSSSSSSSSS ooo--
// EA: 00000000000000 0000000000000 xxx0   EA: 32'h000000XX0
// EA: 00000000000001 0000000000000 xxx0   EA: 32'h000400XX0
// EA: 00000000000010 0000000000000 xxx0   EA: 32'h000800XX0
// EA: 00000000000011 0000000000000 xxx0   EA: 32'h000c00XX0
// EA: 00000000000100 0000000000001 xxx0   EA: 32'h001000XX0
// EA: 00000000000101 0000000000001 xxx0   EA: 32'h001400XX0
// EA: 00000000000110 0000000000001 xxx0   EA: 32'h001800XX0
// EA: 00000000000111 0000000000001 xxx0   EA: 32'h001c00XX0
// EA: 00000000001000 0000000000010 xxx0   EA: 32'h002000XX0
// EA: 00000000001001 0000000000010 xxx0   EA: 32'h002400XX0
// EA: 00000000001010 0000000000010 xxx0   EA: 32'h002800XX0
// EA: 00000000001011 0000000000010 xxx0   EA: 32'h002c00XX0
// EA: 00000000001100 0000000000011 xxx0   EA: 32'h003000XX0
// EA: 00000000001101 0000000000011 xxx0   EA: 32'h003400XX0
// EA: 00000000001110 0000000000011 xxx0   EA: 32'h003800XX0
// EA: 00000000001111 0000000000011 xxx0   EA: 32'h003c00XX0
//
//
//

// --------------------------------------------------------------------------
task cfgBasicTest(input int cfgsel);
begin
  testName = "cfgBasicTest";
  case(cfgsel) 
    0: begin
       $display("-I: setting configuration 0");
       //data arrays
       $readmemh("data/dsram0.cfg0.memh",top.dut0.data[0].dsram.ram);
       $readmemh("data/dsram1.cfg0.memh",top.dut0.data[1].dsram.ram);
       $readmemh("data/dsram2.cfg0.memh",top.dut0.data[2].dsram.ram);
       $readmemh("data/dsram3.cfg0.memh",top.dut0.data[3].dsram.ram);
       //tag arrays
       $readmemh("data/tag0.cfg0.memh",top.dut0.tags[0].tag.ram);
       $readmemh("data/tag1.cfg0.memh",top.dut0.tags[1].tag.ram);
       $readmemh("data/tag2.cfg0.memh",top.dut0.tags[2].tag.ram);
       $readmemh("data/tag3.cfg0.memh",top.dut0.tags[3].tag.ram);
       //valid bits
       $readmemh("data/val.cfg0.memh",top.dut0.bits0.vbits);
       //dirty bits
       $readmemh("data/mod.cfg0.memh",top.dut0.bits0.mbits);
       //LRU bits
       $readmemh("data/lru.cfg0.memh",top.dut0.bits0.lbits);
       end
  endcase
  @(posedge clk);
end
endtask
// -----------------------------------------------------------------
// Loads the 64bit memh file and converts the entries to 4x14b tag
// expect data
// -----------------------------------------------------------------
task load_expect_tags(input string fn,input int verbose=0);
integer i;
reg [13:0] tags[0:3];
begin
  if(verbose) $display("-I: loading expect tags");
  clear_tmp_data();
  $readmemh(fn,mm_tmp_data);
  for(i=0;i<EXP_DATA_ENTRIES;i=i+1) begin
    tags[0] = mm_tmp_data[i][13:0];
    tags[1] = mm_tmp_data[i][29:16];
    tags[2] = mm_tmp_data[i][45:32];
    tags[3] = mm_tmp_data[i][61:48];
    mm_expect_tags[i] = {tags[3],tags[2],tags[1],tags[0]};
  end
  @(posedge clk);
end
endtask
// -----------------------------------------------------------------
// NOTE readmem B
// -----------------------------------------------------------------
task load_expect_bits(input string fn,input int verbose=0);
begin
  if(verbose) $display("-I: loading expect bits");
  $readmemb(fn,mm_expect_bits);
  @(posedge clk);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task load_expect_data(input string afn,input string dfn,input int verbose=0);
begin
  if(verbose) $display("-I: loading expect data");
  $readmemh(afn,top.mm_expect_addr);
  $readmemh(dfn,top.mm_expect_data);
  @(posedge clk);
end
endtask
// -----------------------------------------------------------------
// Separate from clear_tb_data() because this is called multiple
// times when converting files
// -----------------------------------------------------------------
task clear_tmp_data(input int verbose=0);
integer i;
begin
  if(verbose) $display("-I: clearing tb temp data");
  for(i=0;i<EXP_DATA_ENTRIES;i=i+1) begin
    top.mm_tmp_data[i]     = 128'bx;
  end
  @(posedge clk);
end
endtask
// -----------------------------------------------------------------
// set all design state to x
// -----------------------------------------------------------------
task clear_design_data(input int verbose=0);
integer i;
begin
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task clear_tb_data(input int start,input int stop,input int verbose=0);
integer i;
begin
  if(verbose) $display("-I: clearing expect, capture and temp data");
  capture_a_index = 0;
  capture_d_index = 0;
  for(i=start;i<stop;i=i+1) begin
    top.mm_expect_data[i]  = 32'bx;
    top.mm_expect_addr[i]  = 32'bx;
    top.mm_capture_addr[i] = 32'bx;
    top.mm_capture_data[i] = 32'bx;
    top.mm_tmp_data[i]     = 128'bx;
    top.mm_expect_tags[i]  =  56'bx;
    top.mm_expect_bits[i]  =  11'bx;
  end
  @(posedge clk);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task show_tb_add_data(input int start,stop,verbose=0);
integer i,errs;
reg matcha,matchd,match;
begin
  if(verbose) $display("-I: showing addr/data expect values:");
  if(start > EXP_DATA_ENTRIES || stop > EXP_DATA_ENTRIES) begin
    $display("-E: in show_tb_add_data(), array range exceeded");
  end else begin
    errs = 0;
    for(i=start;i<stop;i+=1) begin
      matcha = compare(top.mm_expect_addr[i],top.mm_capture_addr[i]);
      matchd = compare(top.mm_expect_data[i],top.mm_capture_data[i]);
      match  = matcha & matchd;
      if(!match) errs = errs + 1;
      $display("-I: %04d exp:%08x:%08x  act:%08x:%08x %01b",i,
               top.mm_expect_addr[i], top.mm_expect_data[i],
               top.mm_capture_addr[i],top.mm_capture_data[i],match);
    end
    $display("-I: ERRORS : %04d",errs);
  end
  @(posedge clk);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task show_tb_tags_bits(input int start,stop,verbose=0);
integer i,errs;
reg matcht,matchb,match;
reg [55:0] local_tags[0:EXP_DATA_ENTRIES];
reg [10:0] local_bits[0:EXP_DATA_ENTRIES];
begin
  if(verbose) $display("-I: showing tag/bits expect values:");
  if(start > EXP_DATA_ENTRIES || stop > EXP_DATA_ENTRIES) begin
    $display("-E: in show_tb_tags_bits(), array range exceeded");
  end else begin
    errs = 0;
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
    $display("-I: ERRORS : %04d",errs);
  end
  @(posedge clk);
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
