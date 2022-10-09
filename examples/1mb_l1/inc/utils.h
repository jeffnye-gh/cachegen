// vi:ft=verilog:
// -----------------------------------------------------------------
`include "sim_cntrl.h"
`include "functions.h"
// =================================================================
// TASKS
// =================================================================
task rd_req(input [31:0] a,input [3:0] be,input verbose=0);
int watchdog;
begin
  watchdog = 0;
  if(verbose) $display("-I: rd req : a:%08x  be:%04b",a,be);
  top.tb_cc_address    <= `FF a;
  top.tb_cc_byteenable <= `FF be;
  top.tb_cc_read       <= `FF 1'b1;
  top.tb_cc_write      <= `FF 1'b0;
  do begin
    @(posedge clk); 
    ++watchdog;
    if(watchdog > 1000) begin watchdog_err("rd_req"); $finish; end
  end while(!cc_tb_req_hit);
end
endtask
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
task wr_req(input [31:0] a,input [3:0] be,input [31:0] wd,input verbose=0);
int watchdog=0;
begin
  if(verbose) $display("-I: wr req : a:%08x  be:%04b",a,be);
  top.tb_cc_address    = a;
  top.tb_cc_byteenable = be;
  top.tb_cc_read       = 1'b0;
  top.tb_cc_write      = 1'b1;
  top.tb_cc_writedata  = wd;
  do begin
    @(posedge clk); 
    ++watchdog;
    if(watchdog > 1000) begin watchdog_err("wr_req"); $finish; end
  end while(!cc_tb_req_hit);
end
endtask
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
task wr_chk;
input [31:0] a;
input [3:0]  be;
input [31:0] wd;
input integer verbose;
int lclerr;
begin
  //if(verbose) $display("-I: wr req : a:%08x  be:%04b",a,be);
  top.tb_cc_address    = a;
  top.tb_cc_byteenable = be;
  top.tb_cc_read       = 1'b0;
  top.tb_cc_write      = 1'b1;
  top.tb_cc_writedata  = wd;
  @(posedge clk);
  top.tb_cc_read       = #1 1'b0;
  top.tb_cc_write      = #1 1'b0;
  nop(2);
  lclerr = check_word(a,wd);
//  top.tb_cc_address    = #1 31'bx;
end
endtask
// --------------------------------------------------------------------------
// show a single line in a way
//
// 32bytes per line format is : 256x
// -----------------------------------------------------------------
task show_line(input int way, input [12:0] index);
int i;
reg [255:0] data;
begin
  data = 256'h7EADBEEF6EADBEEF5EADBEEF4EADBEEF3EADBEEF2EADBEEF1EADBEEF0EADBEEF;
  //$display("-I: show_line");
       if(way == 0) data = top.dut0.dsram0.ram[index];
  else if(way == 1) data = top.dut0.dsram1.ram[index];
  else if(way == 2) data = top.dut0.dsram2.ram[index];
  else if(way == 3) data = top.dut0.dsram3.ram[index];
  $display("-I: show way%0d d:%032x",way,data);
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
      top.dut0.valid0.regs[i],
      top.dut0.dirty0.regs[i],
      top.dut0.lrurf0.regs[i]);
  end
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
    top.dut0.lrurf0.regs[i] = 3'bx;
    top.dut0.dirty0.regs[i] = 4'bx;
    top.dut0.valid0.regs[i] = 4'bx; 
  end
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
  for(i=0;i<EXP_DATA_ENTRIES;i=i+1) begin
    top.dut0.lrurf0.regs[i] = local_bits[i][2:0];
    top.dut0.dirty0.regs[i] = local_bits[i][6:3];
    top.dut0.valid0.regs[i] = local_bits[i][10:7];
  end
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
end
endtask
// -----------------------------------------------------------------
// NOTE readmem B
// -----------------------------------------------------------------
task load_expect_bits(input string fn,input int verbose=0);
begin
  if(verbose) $display("-I: loading expect bits");
  $readmemb(fn,mm_expect_bits);
end
endtask
// -----------------------------------------------------------------
// This loads the expected capture arrays, primarily for tests with 
// expected read traffic returned to the TB.
// -----------------------------------------------------------------
task load_expect_capture_data(input string afn,input string dfn,
                              input int verbose=0);
begin
  if(verbose) $display("-I: loading expect data");
  $readmemh(afn,top.mm_expect_capture_addr);
  $readmemh(dfn,top.mm_expect_capture_data);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task load_expect_dary_data(input string df0,
                           input string df1,
                           input string df2,
                           input string df3,
                           input int verbose=0);
begin
  if(verbose) $display("-I: loading expect dary data");
  $readmemh(df0,top.mm_expect_dary_0);
  $readmemh(df1,top.mm_expect_dary_1);
  $readmemh(df2,top.mm_expect_dary_2);
  $readmemh(df3,top.mm_expect_dary_3);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task load_expect_main_memory(input string df0,
                             input int verbose=0);
begin
  if(verbose) $display("-I: loading expect main memory data");
  $readmemh(df0,top.mm_expect_mm);
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

    top.mm_expect_capture_addr[i]  = 32'bx;
    top.mm_expect_capture_data[i]  = 32'bx;

    top.mm_actual_capture_addr[i] = 32'bx;
    top.mm_actual_capture_data[i] = 32'bx;

    top.mm_expect_tags[i]  =  56'bx;

    top.mm_expect_bits[i]  =  11'bx;

    top.mm_expect_dary_0[i]  =  256'bx;
    top.mm_expect_dary_1[i]  =  256'bx;
    top.mm_expect_dary_2[i]  =  256'bx;
    top.mm_expect_dary_3[i]  =  256'bx;
  end
end
endtask
// -----------------------------------------------------------------
// Compare the expected capture a/d to the actual capture a/d
// -----------------------------------------------------------------
task check_tb_capture_info(inout integer errs,input int start,stop,verbose=0);
integer i;
reg matcha,matchd,match;
string prefix;
begin
  //do not clear errs, this should be done by the caller
  if(verbose) $display("-I: showing addr/data expect values:");
  if(start > EXP_DATA_ENTRIES || stop > EXP_DATA_ENTRIES) begin
    $display("-E: in show_tb_add_data(), array range exceeded");
  end else begin
    for(i=start;i<stop;i+=1) begin
      matcha = compare32(top.mm_expect_capture_addr[i],
                       top.mm_actual_capture_addr[i]);
      matchd = compare32(top.mm_expect_capture_data[i],
                       top.mm_actual_capture_data[i]);
      match  = matcha & matchd;

      prefix = "-I:";

      if(!match) begin 
        errs = errs + 1;
        prefix = "-E:";
      end 

      if(verbose | !match) begin
        $display("%0s %04d exp:%08x:%08x  act:%08x:%08x %01b",prefix,i,
                                top.mm_expect_capture_addr[i],
                                top.mm_expect_capture_data[i],
                                top.mm_actual_capture_addr[i],
                                top.mm_actual_capture_data[i],match);
      end
    end
    //$display("-I: ERRORS : %04d",errs);
  end
end
endtask
// -----------------------------------------------------------------
task check_main_memory(inout integer errs,input int start,stop,verbose=0);
integer i,j,match;
string pfx;
begin
  if(verbose) $display("-I: showing main memory expect values:");
  if(start > EXP_MM_ENTRIES || stop > EXP_MM_ENTRIES) begin
    $display("-E: in check_main_memory(), array range exceeded");
  end else begin

    for(i=start;i<stop;i+=1) begin
      match = compare256(top.mm_expect_mm[i],top.dut0.mm0.ram[i]);

      pfx = "-I:";
      if(!match) pfx = "-E:";

      if(!match) begin
        $display("%0s index %0d mismatch",pfx,i);
        $display("%0s exp:%032x ",     pfx,top.mm_expect_mm[i]);
        $display("%0s act:%032x m:%0d",pfx,top.dut0.mm0.ram[i],match);
        errs = errs + 1;
      end else if(verbose) begin
        $display("%0s exp:%032x ",     pfx,top.mm_expect_mm[i]);
        $display("%0s act:%032x m:%0d",pfx,top.dut0.mm0.ram[i],match);
      end
    end
  end
end
endtask
// -----------------------------------------------------------------
// This tests the state of the data arrays against expected data.
// This is not used for read only tests, but used in most other tests
// that modify the data array state. There is a separate check_() 
// for read data captured at the test bench.
//
// -----------------------------------------------------------------
task check_data_arrays(inout integer errs,input int start,stop,verbose=0);
integer i,j,matchall;
reg [3:0] matchd;
begin
  if(verbose) $display("-I: showing addr/data expect values:");
  if(start > EXP_DATA_ENTRIES || stop > EXP_DATA_ENTRIES) begin
    $display("-E: in check_data_arrays(), array range exceeded");
  end else begin
  
    for(i=start;i<stop;i+=1) begin
      matchd[0] = compare256(top.mm_expect_dary_0[i],
                             top.dut0.dsram0.ram[i]);
      matchd[1] = compare256(top.mm_expect_dary_1[i],
                             top.dut0.dsram1.ram[i]);
      matchd[2] = compare256(top.mm_expect_dary_2[i],
                             top.dut0.dsram2.ram[i]);
      matchd[3] = compare256(top.mm_expect_dary_3[i],
                             top.dut0.dsram3.ram[i]);
      matchall = &matchd;
      if(!matchall)    $display("-E: index %0d mismatch",i);
      else if(verbose) $display("-I: index %0d    match",i);

      // -------------------------------------------------------------
      if(!matchd[0]) begin
        $display("-E:way0 exp:%032x ",     top.mm_expect_dary_0[i]);
        $display("-E:way0 act:%032x m:%0d",top.dut0.dsram0.ram[i],
                                          matchd[0]);
        errs = errs + 1;
      end else if(verbose) begin
        $display("-I:way0 exp:%032x ",     top.mm_expect_dary_0[i]);
        $display("-I:way0 act:%032x m:%0d",top.dut0.dsram0.ram[i],
                                           matchd[0]);
      end

      // -------------------------------------------------------------
      if(!matchd[1]) begin
        $display("-E:way1 exp:%032x ",   top.mm_expect_dary_1[i]);
        $display("-E:way1 act:%032x m:0",top.dut0.dsram1.ram[i],
                                           matchd[1]);
        errs = errs + 1;
      end else if(verbose) begin
        $display("-I:way1 exp:%032x ",     top.mm_expect_dary_1[i]);
        $display("-I:way1 act:%032x m:%0d",top.dut0.dsram1.ram[i],
                                           matchd[1]);
      end

      // -------------------------------------------------------------
      if(!matchd[2]) begin
        $display("-E:way2 exp:%032x ",   top.mm_expect_dary_2[i]);
        $display("-E:way2 act:%032x m:0",top.dut0.dsram2.ram[i],
                                           matchd[2]);
        errs = errs + 1;
      end else if(verbose) begin
        $display("-I:way2 exp:%032x ",     top.mm_expect_dary_2[i]);
        $display("-I:way2 act:%032x m:%0d",top.dut0.dsram2.ram[i],
                                           matchd[2]);
      end

      // -------------------------------------------------------------
      if(!matchd[3]) begin
        $display("-E:way3 exp:%032x ",     top.mm_expect_dary_3[i]);
        $display("-E:way3 act:%032x m:%0d",top.dut0.dsram3.ram[i],
                                           matchd[3]);
        errs = errs + 1;
      end else if(verbose) begin
        $display("-I:way3 exp:%032x ",     top.mm_expect_dary_3[i]);
        $display("-I:way3 act:%032x m:%0d",top.dut0.dsram3.ram[i],
                                           matchd[3]);
      end
//      end
    end
  end
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task check_tb_tags_bits(inout int errs,input int start,stop,verbose=0);
integer i;
reg matcht,matchb,match;
reg [55:0] local_tags[0:EXP_DATA_ENTRIES];
reg [10:0] local_bits[0:EXP_DATA_ENTRIES];
reg [3:0] vbits;
reg [3:0] mbits;
reg [2:0] lbits;
string prefix;
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

      vbits = top.dut0.valid0.regs[i];
      mbits = top.dut0.dirty0.regs[i];
      lbits = top.dut0.lrurf0.regs[i];
      local_bits[i] = { vbits, mbits, lbits }; 
    end
    for(i=start;i<stop;i+=1) begin
      matcht = compare32(top.mm_expect_tags[i],local_tags[i]);
      matchb = compare32(top.mm_expect_bits[i],local_bits[i]);
      match  = matcht & matchb;
      prefix = "-I:";
      
      if(!match) begin
        errs = errs + 1;
        prefix = "-E:";
      end 

      if(verbose | !match) begin
        $display("%0s %02d exp:%03x %03x %03x %03x : %04b %04b %03b : %1b",
          prefix,i,
          top.mm_expect_tags[i][55:42],
          top.mm_expect_tags[i][41:28],
          top.mm_expect_tags[i][27:14],
          top.mm_expect_tags[i][13:0],
          top.mm_expect_bits[i][10:7],
          top.mm_expect_bits[i][6:3],
          top.mm_expect_bits[i][2:0],match);

        $display("%0s %02d act:%03x %03x %03x %03x : %04b %04b %03b : %1b\n",
          prefix,i,
          local_tags[i][55:42],
          local_tags[i][41:28],
          local_tags[i][27:14],
          local_tags[i][13:0],
          local_bits[i][10:7],
          local_bits[i][6:3],
          local_bits[i][2:0],match);
      end
    end
    //$display("-I: ERRORS : %04d",errs);
  end
end
endtask
// -----------------------------------------------------------------
// FIXME: Make this general
// -----------------------------------------------------------------
task dbl_check_init(inout int errs,input int sel,input string fn,
                    input int verbose=0);
reg [255:0] _local[0:8192-1];
integer i,start,stop,lerr,matchd;
string pfx;
begin
  lerr = 0;
  start = 0;
  stop  = 16;
  pfx   = "-I:";

  $readmemh(fn,_local);

  for(i=start;i<stop;i=i+1) begin
    case(sel) 
      0: if(_local[i] !== top.dut0.dsram0.ram[i]) lerr = lerr + 1;
      1: if(_local[i] !== top.dut0.dsram1.ram[i]) lerr = lerr + 1;
      2: if(_local[i] !== top.dut0.dsram2.ram[i]) lerr = lerr + 1;
      3: if(_local[i] !== top.dut0.dsram3.ram[i]) lerr = lerr + 1;
      default: begin
        $display("-E: unknown selector in DDD");
        lerr += 1;
      end
    endcase

    if(lerr >0) begin
      pfx = "-E:"; 
      $display("%0s dbl_check_init() mismatch",pfx);
    end

    if(lerr > 0 | verbose) begin

      if(lerr == 0) matchd = 1;
      else          matchd = 0;

      $display("%0s0 i:%0d e:%032x ", pfx,i,_local[i]);

      case(sel)
        0: $display("%0s0 i:%0d a:%032x m:%0d",
                    pfx,i,top.dut0.dsram0.ram[i],matchd);
        1: $display("%0s1 i:%0d a:%032x m:%0d",
                    pfx,i,top.dut0.dsram1.ram[i],matchd);
        2: $display("%0s2 i:%0d a:%032x m:%0d",
                    pfx,i,top.dut0.dsram2.ram[i],matchd);
        3: $display("%0s3 i:%0d a:%032x m:%0d",
                    pfx,i,top.dut0.dsram3.ram[i],matchd);
        default: begin
          $display("-E: unknown selector in DDD");
          lerr += 1;
        end
      endcase
    end
  end
  errs = errs + lerr;
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task nop(input int count,input int verbose=0);
integer i;
begin
  tb_cmd = TB_CMD_NOP;
  tb_cc_read     = 1'b0;
  tb_cc_write    = 1'b0;
  tb_cc_ram_test = 1'b0;
  if(verbose) $display("-I: NOP");
  //@(posedge clk);
  for(i=0;i<count;i=i+1) @(posedge clk);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task terminate;
string msg,pfx;
integer total_errs;
begin
  total_errs = abserr(lru_errs,1000)
             + abserr(basic_rd_hit_errs,1000) 
             + abserr(basic_wr_hit_errs,1000)
             + abserr(basic_rd_alloc_errs,1000)
             + abserr(basic_wr_alloc_errs,1000)
             ;

  pfx = "-I:";
  msg = "PASS";

  if(total_errs !== 0) begin
    pfx = "-E: ";
    msg = "FAIL";
  end
  $display("==================================================");
  $display("%0s basic lru errors      : %0d",pfx,lru_errs);
  $display("%0s basic rd hit errors   : %0d",pfx,basic_rd_hit_errs);
  $display("%0s basic wr hit errors   : %0d",pfx,basic_wr_hit_errs);
  $display("%0s basic rd alloc errors : %0d",pfx,basic_rd_alloc_errs);
  $display("%0s basic wr alloc errors : %0d",pfx,basic_wr_alloc_errs);
  $display("");
  $display("%0s Total errors          : %0d",pfx,total_errs);
  $display("==================================================");

  imsg("CACHE SIM END");
  $display("%sTERMINATE %s",pfx,msg);
  tb_cmd = TB_CMD_TERM;
  @(posedge clk);
  @(posedge clk);
  $finish;
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task initTest(input string tn,inout errs);
begin
  errs = 0;
  nop(4);
  testName = tn;
  beginTestMsg(testName);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task beginTestMsg(input string testName);
begin
  $display("-I: BEGIN TEST : %014s",testName);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task endTestMsg(input string testName,input int errs);
string pf,pre;
begin
  if(errs > 0) begin pf = "FAIL"; pre = "-E: "; end
  else         begin pf = "PASS"; pre = "-I: "; end
  $display("%0sEND TEST   : %014s : errors %0d : %0s",pre,testName,errs,pf);
end
endtask
// -----------------------------------------------------------------
// -----------------------------------------------------------------
task imsg( input string m); begin $display("-I: %0s",m); end endtask
task emsg( input string m); begin $display("-E: %0s",m); end endtask
// -----------------------------------------------------------------
task watchdog_err(input string m);
begin
  $display("-E: ###################################################");
  $display("-E: watch dog time out in %0s",m);
  $display("-E: ###################################################");
  $finish;
end
endtask
