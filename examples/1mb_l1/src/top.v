`include "sim_cntrl.h"
// ----------------------------------------------------------------------
//  Top level test bench
//  See README.txt
// ----------------------------------------------------------------------
module top;
`include "utils.h"    //support tasks/functions
`include "tbcmds.h"   //TB controls
`include "tests.h"    //tasks implementing tests
`include "locals.h"   //other enum like vars

localparam integer MAX = 5000;
localparam integer MM_MEM_RANGE = 256;
localparam integer EXP_DATA_ENTRIES = 256;
// ------------------------------------------------------------------------
// Test controls
// ------------------------------------------------------------------------
localparam _bypass_test = 1'b0;
localparam _tag_rw_test = 1'b0;
localparam _basic_tests      = 1'b1;
localparam   _bt_lru_test    = 1'b1;
localparam   _bt_rd_hit_test = 1'b1;
localparam   _bt_wr_hit_test = 1'b1;

// ----------------------------------------------------------------------
integer count;
integer lru_errs,basic_rd_hit_errs,basic_wr_hit_errs;
reg master_clk,clk,reset;
reg [8*16:0] testName;
// ----------------------------------------------------------------------
reg [3:0] tb_cmd;
reg tb_cc_ram_test;
wire bypass = (tb_cmd == TB_CMD_BYPASS) & !reset;
// ----------------------------------------------------------------------
wire [31:0]  by_mm_address;
wire [255:0] by_mm_writedata;
wire         by_mm_write;
wire         by_mm_read;
wire [3:0]   by_mm_byteenable;
// ----------------------------------------------------------------------
reg  [31:0]  tb_cc_address;
reg  [3:0]   tb_cc_byteenable;
reg          tb_cc_read;
reg          tb_cc_write;
reg  [31:0]  tb_cc_writedata;
reg  [255:0] tb_cc_wide_writedata;
wire [31:0]  cc_tb_readdata;

wire cc_tb_readdata_valid;
wire cc_tb_req_hit;
// ----------------------------------------------------------------------
wire [31:0]  cc_mm_address;
wire [255:0] cc_mm_writedata;
wire         cc_mm_write;
wire         cc_mm_read;
wire [255:0] mm_cc_readdata;
wire [255:0] xmm_cc_readdata;
wire         mm_cc_readdatavalid;

// ------------------------------------------------------------------------
reg  [255:0] mm_expect_dary_0[0:EXP_DATA_ENTRIES];
reg  [255:0] mm_expect_dary_1[0:EXP_DATA_ENTRIES];
reg  [255:0] mm_expect_dary_2[0:EXP_DATA_ENTRIES];
reg  [255:0] mm_expect_dary_3[0:EXP_DATA_ENTRIES];

reg  [31:0] mm_expect_capture_data[0:EXP_DATA_ENTRIES];
reg  [31:0] mm_expect_capture_addr[0:EXP_DATA_ENTRIES];

reg  [31:0] mm_actual_capture_data[0:EXP_DATA_ENTRIES];
reg  [31:0] mm_actual_capture_addr[0:EXP_DATA_ENTRIES];

// tag expect data is combined 4x 14b  {way3[13:0],way2 etc ,way1,way0}
reg  [(4*14)-1:0] mm_expect_tags[0:EXP_DATA_ENTRIES];

//bits = 4x valid bits, 4x mod bits, 3x lru bits = 11b
//concatenate format { vvvv,mmmm,lru }, way3->way0
reg  [10:0] mm_expect_bits[0:EXP_DATA_ENTRIES];
// ------------------------------------------------------------------------
wire [1:0] _byte = 2'b00;
wire [2:0] word = 3'bxxx;

initial begin
  master_clk = 'b0;
  clk        = 'b0;
  reset      = 'b1;

  tb_cmd = TB_CMD_NOP;

  tb_cc_ram_test = 1'b0;
  tb_cc_read     = 1'b0;
  tb_cc_write    = 1'b0;

  lru_errs = 0;
  basic_rd_hit_errs = 0;
  basic_wr_hit_errs = 0;
  count = 0;

  $dumpfile("csim.vcd");
  $dumpvars(0,top);
end

// ------------------------------------------------------------------------
always master_clk = #50 !master_clk;
always @(posedge master_clk) clk   <= !clk;
always @(posedge clk) count <= count + 1;
// ------------------------------------------------------------------------
//reg [31:0] capture_addr;
integer capture_a_index,capture_d_index;
wire [31:0] mm_capture_addr_0 = mm_actual_capture_addr[0];
wire [31:0] mm_capture_addr_1 = mm_actual_capture_addr[1];
reg  [31:0] tb_cc_address_q;
// ------------------------------------------------------------------------
always @(posedge clk) begin

  if(tb_cc_read) begin
    mm_actual_capture_addr[capture_a_index] <= tb_cc_address;
    capture_a_index <= capture_a_index + 1;
  end

  if(cc_tb_readdata_valid) begin
    //$display("-I: capturing data : a:%08x:%08x",capture_addr,cc_tb_readdata);
    mm_actual_capture_data[capture_d_index] <= cc_tb_readdata;
    capture_d_index <= capture_d_index+1;
  end
end
// ------------------------------------------------------------------------
reg [8*8:0] tb_cmd_txt;
always @(tb_cmd or reset) begin
  if(reset) tb_cmd_txt = "RST "; 
  else begin
    case(tb_cmd) 
      TB_CMD_NOP:       tb_cmd_txt = "TB_NOP ";
      TB_CMD_NORMAL:    tb_cmd_txt = "TB_NRML";
      TB_CMD_BYPASS:    tb_cmd_txt = "TB_BYP ";
      TB_CMD_INVAL:     tb_cmd_txt = "TB_INV ";
      TB_CMD_INVAL_ALL: tb_cmd_txt = "TB_INVA";
      TB_CMD_DIRTY:     tb_cmd_txt = "TB_DRTY";
      TB_CMD_CLEAN:     tb_cmd_txt = "TB_CLN ";
      TB_CMD_FLUSH:     tb_cmd_txt = "TB_FLSH";
      TB_CMD_FLUSH_ALL: tb_cmd_txt = "TB_FLSA";
      TB_CMD_WBACK:     tb_cmd_txt = "TB_WBCK";
      TB_CMD_TERM:      tb_cmd_txt = "TB_TERM";
      default:          tb_cmd_txt = "TB_XXXX";
    endcase 
  end
end
// ------------------------------------------------------------------------
always @(count) begin

  if(count == 0) imsg("CACHE SIM START");

  testName = "None";
  if(count < 3)  reset = 1'b1; 
  else           reset = 1'b0;

  if(count > 2) begin

    initState();
    if(_bypass_test) bypassTest(8); 
    if(_tag_rw_test) tagRwTest(8); 
    if(_basic_tests) begin
      if(_bt_lru_test)    basicLruTest(lru_errs,0);
      if(_bt_rd_hit_test) basicRdHitTest(basic_rd_hit_errs,0);
      if(_bt_wr_hit_test) basicWrHitTest(basic_wr_hit_errs,0);
    end
    terminate();
  end

  if(count > MAX) terminate();
end
// ----------------------------------------------------------------
_probes _prb();
cache dut0(
  //outputs
  .rd      (cc_tb_readdata),
  .rd_valid(cc_tb_readdata_valid),
  .req_hit (cc_tb_req_hit),

  //from TB 
  .a    (tb_cc_address),
  .be   (tb_cc_byteenable),
  .read (tb_cc_read),
  .write(tb_cc_write),
  .wd   (tb_cc_writedata),

  .ram_test(tb_cc_ram_test),

  //from cache to mainmemory
  .mm_a    (cc_mm_address),
  .mm_wd   (cc_mm_writedata),
  .mm_write(cc_mm_write),
  .mm_read (cc_mm_read),

  //from main to L1, fill
  .mm_rd   (mm_cc_readdata),
  .mm_valid(mm_cc_readdatavalid),

  .reset(reset),
  .clk(clk)
);
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
assign by_mm_address    = bypass ? tb_cc_address        : cc_mm_address;
assign by_mm_writedata  = bypass ? tb_cc_wide_writedata : cc_mm_writedata;
assign by_mm_write      = bypass ? tb_cc_write          : cc_mm_write;
assign by_mm_read       = bypass ? tb_cc_read           : cc_mm_read;
assign by_mm_byteenable = bypass ? tb_cc_byteenable     : 4'bx;
// ----------------------------------------------------------------------
mainmemory #(.MEM_RANGE(MM_MEM_RANGE)) mm0(
  .rd   (mm_cc_readdata),
  .valid(mm_cc_readdatavalid),

  //from CC/TB control
  .a     (by_mm_address),
  .be    (by_mm_byteenable),
  .read  (by_mm_write),
  .write (by_mm_read),
  .wd    (by_mm_writedata),

  .bypass(bypass),

  .reset(reset),
  .clk(clk)
);

endmodule
