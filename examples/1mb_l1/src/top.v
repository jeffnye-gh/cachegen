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
localparam integer MM_ADDR_WIDTH     = 20;
localparam integer EXP_MM_ENTRIES    = 131072;
localparam integer EXP_DATA_ENTRIES  = 256;

localparam integer L1_READ_HIT_LAT   = 1;
localparam integer L1_WRITE_HIT_TPUT = 1;

localparam integer MM_READ_LAT       = 4;
localparam integer MM_WRITE_TPUT     = 4;
// ------------------------------------------------------------------------
// Test controls
//
// _bt_rd_hit_test    rd hit
// _bt_wr_hit_test    wr hit
//
// _bt_rd_alloc_test  rd miss   fill an invalid way
// _bt_wr_alloc_test  wr miss   fill an invalid way
//
// _bt_rd_evict_test  rd miss   evict a dirty line
// _bt_wr_evict_test  wr miss   evict a dirty line
//
// _bt_rd_clean_test  rd miss   replace a clean line - save, should be simpler
// _bt_wr_clean_test  wr miss   replace a clean line - save, should be simpler
//
// ------------------------------------------------------------------------
localparam _basic_tests        = 1'b1;
localparam   _bt_lru_test      = 1'b1;
localparam   _bt_rd_hit_test   = 1'b1;
localparam   _bt_wr_hit_test   = 1'b1;
localparam   _bt_rd_alloc_test = 1'b1;
localparam   _bt_wr_alloc_test = 1'b1;
localparam   _bt_rd_evict_test = 1'b1;
localparam   _bt_wr_evict_test = 1'b0;

localparam   _bt_rd_clean_test = 1'b0;
localparam   _bt_wr_clean_test = 1'b0;
// ----------------------------------------------------------------------
reg lru_flag;
reg basic_rd_hit_flag;
reg basic_wr_hit_flag;
reg basic_rd_alloc_flag;
reg basic_wr_alloc_flag;
reg basic_rd_evict_flag;
reg basic_wr_evict_flag;
reg basic_rd_clean_flag;
reg basic_wr_clean_flag;
// ----------------------------------------------------------------------
int count;
int lru_errs            = -1;
int basic_rd_hit_errs   = -1;
int basic_wr_hit_errs   = -1;
int basic_rd_alloc_errs = -1;
int basic_wr_alloc_errs = -1;
int basic_rd_evict_errs = -1;
int basic_wr_evict_errs = -1;
int basic_rd_clean_errs = -1;
int basic_wr_clean_errs = -1;
// ----------------------------------------------------------------------
reg master_clk,clk,reset;
//icarus does not report strings in vcd
string testName;
// ----------------------------------------------------------------------
reg [3:0] tb_cmd;
// ----------------------------------------------------------------------
reg  [31:0]  tb_cc_address;
reg  [3:0]   tb_cc_byteenable;
reg          tb_cc_read;
reg          tb_cc_write;
reg  [31:0]  tb_cc_writedata;
wire [31:0]  cc_tb_readdata;

wire cc_tb_readdata_valid;
wire cc_tb_ready;
// ----------------------------------------------------------------------
wire [31:0]  cc_mm_address;
wire [255:0] cc_mm_writedata;
wire         cc_mm_write;
wire         cc_mm_read;
//wire [31:0]  cc_mm_byteenable;
wire [255:0] mm_cc_readdata;
wire [255:0] xmm_cc_readdata;
wire         mm_cc_readdatavalid;

// ------------------------------------------------------------------------
reg  [255:0] mm_expect_mm[0:EXP_MM_ENTRIES];
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

  tb_cc_read     = 1'b0;
  tb_cc_write    = 1'b0;

  capture_a_index = 0;
  capture_d_index = 0;

  lru_flag = 1'b0;
  basic_rd_hit_flag = 1'b0;
  basic_wr_hit_flag = 1'b0;
  basic_rd_alloc_flag = 1'b0;
  basic_wr_alloc_flag = 1'b0;

  count = 0;

  $dumpfile("csim.vcd");
  $dumpvars(0,top);

  run_tests();
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
// ------------------------------------------------------------------------
wire capture_a = tb_cc_read & cc_tb_ready;
wire capture_d = cc_tb_readdata_valid;
// ------------------------------------------------------------------------
always @(posedge clk) begin
  if(capture_a) begin
    mm_actual_capture_addr[capture_a_index] <= tb_cc_address;
    capture_a_index <= capture_a_index + 1;
  end
  if(capture_d) begin
    mm_actual_capture_data[capture_d_index] <= cc_tb_readdata;
    capture_d_index <= capture_d_index + 1;
  end
end
// ------------------------------------------------------------------------
task run_tests;
begin
  testName = "None";
  nop(1);
  if(_basic_tests) begin

    if(_bt_lru_test)
      basicLruTest(lru_errs,lru_flag,0);

    if(_bt_rd_hit_test)
      basicRdHitTest(basic_rd_hit_errs,basic_rd_hit_flag,0);

    if(_bt_wr_hit_test)
      basicWrHitTest(basic_wr_hit_errs,basic_wr_hit_flag,0);

    if(_bt_rd_alloc_test)
      basicRdAllocTest(basic_rd_alloc_errs,basic_rd_alloc_flag,0);

    if(_bt_wr_alloc_test)
      basicWrAllocTest(basic_wr_alloc_errs,basic_wr_alloc_flag,0);

    if(_bt_rd_evict_test)
      basicRdEvictTest(basic_rd_evict_errs,basic_rd_evict_flag,0);

    if(_bt_wr_evict_test)
      basicWrEvictTest(basic_wr_evict_errs,basic_wr_evict_flag,0);
  end
  nop(1);
  terminate();
end
endtask
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
always @(count) begin
  if(count == 0) imsg("CACHE SIM START");
  if(count < 3)  reset = 1'b1; 
  else           reset = 1'b0;
  if(count > MAX) terminate();
end
// ----------------------------------------------------------------
_probes _prb();
// ----------------------------------------------------------------
cache #(.READ_HIT_LAT(L1_READ_HIT_LAT),
        .WRITE_HIT_TPUT(L1_WRITE_HIT_TPUT)
) dut0(
  //outputs
  .rd        (cc_tb_readdata),
  .rd_valid_d(cc_tb_readdata_valid),
  .ready_d   (cc_tb_ready),

  //from TB 
  .a    (tb_cc_address),
  .be   (tb_cc_byteenable),
  .read (tb_cc_read),
  .write(tb_cc_write),
  .wd   (tb_cc_writedata),

  //from cache to mainmemory
  .mm_a        (cc_mm_address),
  .mm_read_d   (cc_mm_read),
  .mm_write_d  (cc_mm_write),
  //from L1 to main, evict
  .mm_writedata(cc_mm_writedata),

  //from main to L1, fill
  .mm_readdata(mm_cc_readdata),

  .mm_readdata_valid(mm_cc_readdatavalid),
  .mm_ready(mm_cc_ready),

  .reset(reset),
  .clk(clk)
);
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
//mainmemory #(.MEM_RANGE(MM_MEM_RANGE)) mm0(
mainmemory #(.ENTRIES(EXP_MM_ENTRIES),
             .READ_LAT(MM_READ_LAT),
             .WRITE_TPUT(MM_WRITE_TPUT)
) mm0(

  .rd   (mm_cc_readdata),
  .valid(mm_cc_readdatavalid),
  .ready(mm_cc_ready),

  //from CC/TB control
  .a     (cc_mm_address[31:5]), //only line access
  .wd    (cc_mm_writedata),
  .read  (cc_mm_read),
  .write (cc_mm_write),

  .clk(clk)
);

endmodule
