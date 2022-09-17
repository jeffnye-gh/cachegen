`include "sim_cntrl.h"
// -------------------------------------------------------------------------
// cache 1MB, 4 way, 32B lines
//
//  With byte addressability this becomes
//                                     bbbb
//  33222222222211 11111111            eeee
//  10987654321098 7654321098765 432-- 3210
//  TTTTTTTTTTTTTT SSSSSSSSSSSSS ooo-- bbbb
//
//
//  The tags can be 8KB x 4 x 14b
//  Valid bits are  8KB x 4 x  1b
//  Dirty bits are  8KB x 4 x  1b
//  LRU   bits are  8KB   x    3b
// -------------------------------------------------------------------------
module cache 
(
  //Back to PE/TB
  output wire [31:0]  rd,
  output wire         valid,

  //From PE/TB
  input  wire [31:0]  a,
  input  wire [3:0]   be,
  input  wire         read,
  input  wire         write,
  input  wire [31:0]  wd,

  //From TB
  input  wire ram_test,

  //From cache to main memory
  output wire [31:0]   mm_a,
  output wire [255:0]  mm_wd, // line eviction data
  output wire          mm_write, //to MM, write command
  output wire          mm_read,  //to MM, read command

  input  wire [255:0]  mm_rd,    // line fill data
  input  wire          mm_valid, //fill data valid

  input  wire          reset,
  input  wire          clk
);

`include "bitcmds.h"
localparam integer CACHELINE_BITS = 32*8;
localparam integer TAG_BITS = 14;
localparam integer IDX_BITS = 13;
localparam integer WAYS =  4;

// ------------------------------------------------------------------------
// FIXME: temp
// ------------------------------------------------------------------------
assign mm_read  = 1'b0;
assign mm_write = 1'b0;
// ------------------------------------------------------------------------
// Probes
// iverilog/gtfw will not show the 2d buses
// ------------------------------------------------------------------------
//synthesis off
wire [TAG_BITS-1:0] tag_readdata0 = cc_tag_readdata[0];
wire [TAG_BITS-1:0] tag_readdata1 = cc_tag_readdata[1];
wire [TAG_BITS-1:0] tag_readdata2 = cc_tag_readdata[2];
wire [TAG_BITS-1:0] tag_readdata3 = cc_tag_readdata[3];
wire [255:0]        ary_readdata0 = cc_ary_readdata[0];
wire [255:0]        ary_readdata1 = cc_ary_readdata[1];
wire [255:0]        ary_readdata2 = cc_ary_readdata[2];
wire [255:0]        ary_readdata3 = cc_ary_readdata[3];

wire val_bit0 = val_bits[0];
wire val_bit1 = val_bits[1];
wire val_bit2 = val_bits[2];
wire val_bit3 = val_bits[3];
//synthesis on
// ------------------------------------------------------------------------
wire [CACHELINE_BITS-1:0] fsm_cc_wd;
wire [CACHELINE_BITS-1:0] cc_ary_readdata[3:0];

reg  [TAG_BITS-1:0] ag_wd;
wire [TAG_BITS-1:0] cc_tag_readdata[3:0];

wire [3:0] tag_write;

wire [3:0] val_bits,mod_bits;
wire [3:0] lru_bits; //one hot

wire [3:0]          fsm_bit_cmd;
wire                fsm_bit_cmd_valid;

wire [TAG_BITS-1:0] fsm_cc_tag;
wire [IDX_BITS-1:0] fsm_cc_index;
wire [2:0]          fsm_cc_offset; //word within a line
wire [3:0]          fsm_cc_be;

wire [3:0]          fsm_cc_fill;
wire [3:0]          fsm_cc_ary_write;
wire                fsm_cc_ary_read;
wire                fsm_cc_tag_read;
wire [3:0]          fsm_cc_tag_write;

//wire pe_req_mod;

wire pe_flush;
wire pe_flush_all;
wire pe_invalidate;
wire pe_invalidate_all;

wire [3:0] fsm_cc_way_match;
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
//always @* begin
//  tag_write[0] = ram_test ? (write & tst_tag_sel[0]) : 1'b0;
//  tag_write[1] = ram_test ? (write & tst_tag_sel[1]) : 1'b0;
//  tag_write[2] = ram_test ? (write & tst_tag_sel[2]) : 1'b0;
//  tag_write[3] = ram_test ? (write & tst_tag_sel[3]) : 1'b0;
//
//  tag_wd = ram_test ? wd[13:0] : 14'bx;
//end
// --------------------------------------------------------------------------
// FSM
// --------------------------------------------------------------------------
fsm #(.IDX_BITS(IDX_BITS),.TAG_BITS(TAG_BITS)) fsm0 (

  .tb_ram_test(ram_test),

  .pe_a        (a),
  .pe_read     (read),
  .pe_write    (write),
  .pe_writedata(wd),

  .fsm_bit_cmd(fsm_bit_cmd),
  .fsm_bit_cmd_valid(fsm_bit_cmd_valid),

  .fsm_pe_readdata(rd),
  .fsm_pe_readdata_valid(valid),

  .fsm_cc_tag   (fsm_cc_tag),
  .fsm_cc_index (fsm_cc_index),
  .fsm_cc_offset(fsm_cc_offset),
  .fsm_cc_be    (fsm_cc_be),

  .fsm_cc_fill(fsm_cc_fill),

  .fsm_cc_tag_read(fsm_cc_tag_read),
  .fsm_cc_tag_write(fsm_cc_tag_write),

  .fsm_cc_ary_read(fsm_cc_ary_read),
  .fsm_cc_ary_write(fsm_cc_ary_write),

  .fsm_cc_way_match(fsm_cc_way_match),

  .cc_val_bits(val_bits),
  .cc_mod_bits(mod_bits),

  .cc_tag_readdata(cc_tag_readdata),
  .cc_ary_readdata(cc_ary_readdata),

  .pe_flush(pe_flush),
  .pe_flush_all(pe_flush_all),
  .pe_invalidate(pe_invalidate),
  .pe_invalidate_all(pe_invalidate_all),

  .mm_readdata_valid(mm_valid),

  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
// STATUS BITS
// --------------------------------------------------------------------------

bitarray #(.IDX_BITS(IDX_BITS)) bits0(
  .val(val_bits),
  .mod(mod_bits),
  .lru(lru_bits),

  .index(fsm_cc_index),
  .way_match(fsm_cc_way_match),

  .pe_read(read),
  .pe_write(write),

  .cmd(fsm_bit_cmd),
  .cmd_valid(bit_cmd_valid),
  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
// TAGS
// --------------------------------------------------------------------------
wire [TAG_BITS-1:0] tag_wd;
genvar WAYVAR;
generate for(WAYVAR=0;WAYVAR<4;WAYVAR=WAYVAR+1) begin : tags
  sram #(.DATA_WIDTH(TAG_BITS),.ADDR_WIDTH(IDX_BITS)) tag (
    .a    (fsm_cc_index),
    .wd   (tag_wd),
    .rd   (cc_tag_readdata[WAYVAR]),
    .write(fsm_cc_tag_write[WAYVAR]),
    .read (fsm_cc_tag_read),
    .clk  (clk)
  ); 
end
endgenerate  
// --------------------------------------------------------------------------
// FIXME: DATA ARRAYS
// --------------------------------------------------------------------------
generate for(WAYVAR=0;WAYVAR<4;WAYVAR=WAYVAR+1) begin : data
  dsram #(.ADDR_WIDTH(IDX_BITS)) dsram (
    .a     (fsm_cc_index),
    .offset(fsm_cc_offset),
    .wd    (fsm_cc_wd),
    .be    (fsm_cc_be),
    .rd    (cc_ary_readdata[WAYVAR]),
    .fill  (fsm_cc_fill[WAYVAR]),
    .write (fsm_cc_ary_write[WAYVAR]),
    .read  (fsm_cc_ary_read),
    .clk   (clk)
  );
end
endgenerate
endmodule
