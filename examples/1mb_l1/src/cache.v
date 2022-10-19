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
module cache #(
  parameter READ_HIT_LAT = 1,
  parameter WRITE_HIT_TPUT = 1
)
(
  //Back to PE/TB
  output reg   [31:0]  rd,
  output wire          rd_valid_d,

  output wire         req_hit_d,   //status output
  output wire         req_miss_d,
  output wire         req_mod_d,

  //From PE/TB
  input  wire [31:0]  a,
  input  wire [3:0]   be,
  input  wire         read,
  input  wire         write,
  input  wire [31:0]  wd,

  //From cache to main memory
  output wire [31:0]   mm_a,
//  output wire [31:0]   mm_be,      //to MM, byte enables, FIXME: needed?
  output wire [255:0]  mm_writedata,      // line eviction data
  output wire          mm_write_d, //to MM, write command
  output wire          mm_read_d,  //to MM, read command

  input  wire [255:0]  mm_readdata,    // line fill data
  input  wire          mm_readdata_valid, //fill data valid
  input  wire          mm_ready, 

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
//n/a
// ------------------------------------------------------------------------
//assign mm_be = 32'hFFFFFFFF;
assign mm_a = a;
// ------------------------------------------------------------------------
wire [CACHELINE_BITS-1:0] fsm_cc_wd;
wire [CACHELINE_BITS-1:0] dary_out[3:0];

reg  [TAG_BITS-1:0] ag_wd;
wire [TAG_BITS-1:0] tag_out_d[3:0];

wire lru_wr,mod_wr;
wire [3:0] val_out_d;
wire [3:0] mod_out_d;
wire [2:0] lru_out_d;

wire fsm_mm_read_d;
wire fsm_mm_write_d;

wire fsm_cc_readdata_valid;

wire fsm_cc_ary_write_d;
//wire fsm_cc_tag_write_d;
wire fsm_cc_val_write_d;
wire fsm_cc_mod_write_d;
wire fsm_cc_lru_write_d;

wire fsm_cc_is_val_d;
wire fsm_cc_is_mod_d;
wire fsm_cc_fill_d,fsm_cc_rd_fill_d,fsm_cc_wr_fill_d;
reg  fsm_cc_rd_fill;

wire pe_flush;
wire pe_flush_all;
wire pe_invalidate;
wire pe_invalidate_all;

wire [3:0] way_sel_d;
reg  [3:0] way_sel;
wire [3:0] way_mod_d;

// --------------------------------------------------------------------------
// Fix up the names so it's clear what pipe stage they are in
// --------------------------------------------------------------------------
wire  [TAG_BITS-1:0] pe_tag_d     = a[31:18];
wire  [IDX_BITS-1:0] pe_index_d   = a[17:5];
wire  [2:0]          pe_offset_d  = a[4:2];
wire  [3:0]          pe_be_d      = be;
wire                 pe_read_d    = read;
wire                 pe_write_d   = write;
wire  [31:0]         pe_wd_d      = wd;
wire                 cache_read_d = !reset & (pe_read_d | pe_write_d);
// --------------------------------------------------------------------------
//The index is applied to the rams directly, because the rams contain
//the flops on the address
//
//The input tag is clocked for the comparison with the output of the tag rams 
// (also the valid/etc bits)
//
//The offset is clocked for word select on the output of the dary rams
//
//The be's are used both clocked and unclocked. 
//For writes pe_be_d is applied to the rams (gates by control from the FSM). 
//For reads  pe_be is used to align sub word read data to the LSBs
//
//pe_read_d is applied to the rams
// --------------------------------------------------------------------------
reg   [2:0]  pe_offset,pe_offset_q;
reg   [3:0]  pe_be;
reg  [31:0]  pe_wd;
reg [255:0]  mm_rd_q;

wire pe_access_d     = (pe_read_d | pe_write_d) & !reset;
assign fsm_cc_fill_d = fsm_cc_rd_fill_d | fsm_cc_wr_fill_d;

always @(posedge clk) begin
  fsm_cc_rd_fill <= fsm_cc_rd_fill_d;

  pe_offset   <= pe_offset_d;
  pe_offset_q <= pe_offset;

  pe_be   <= pe_be_d;
  pe_wd   <= pe_wd_d;
  way_sel <= way_sel_d;
  mm_rd_q <= mm_readdata;
end
// --------------------------------------------------------------------------
// select logic
// --------------------------------------------------------------------------
wire [3:0] tag_compare_d;
assign tag_compare_d[0] = tag_out_d[0] == pe_tag_d;
assign tag_compare_d[1] = tag_out_d[1] == pe_tag_d;
assign tag_compare_d[2] = tag_out_d[2] == pe_tag_d;
assign tag_compare_d[3] = tag_out_d[3] == pe_tag_d;

//Way is valid and tag matches 
assign way_sel_d[0] = pe_access_d & val_out_d[0] & tag_compare_d[0];
assign way_sel_d[1] = pe_access_d & val_out_d[1] & tag_compare_d[1];
assign way_sel_d[2] = pe_access_d & val_out_d[2] & tag_compare_d[2];
assign way_sel_d[3] = pe_access_d & val_out_d[3] & tag_compare_d[3];

//selected way is modified
assign way_mod_d[0] = way_sel_d[0] & mod_out_d[0];
assign way_mod_d[1] = way_sel_d[1] & mod_out_d[1];
assign way_mod_d[2] = way_sel_d[2] & mod_out_d[2];
assign way_mod_d[3] = way_sel_d[3] & mod_out_d[3];

assign rd_valid_d = fsm_cc_readdata_valid;
assign mm_read_d  = fsm_mm_read_d;
assign mm_write_d = fsm_mm_write_d;

assign req_hit_d  = (|way_sel_d & pe_access_d) | fsm_cc_fill_d;
assign req_miss_d = ~req_hit_d;
assign req_mod_d  = |way_mod_d & pe_access_d;
// --------------------------------------------------------------------------
// write logic
//
//  way selects line
//    offset selects word
//      be selects byte
//  
// write data is aligned to a cache line
// the byte enables are active only for the selected word/byte
//
// word data is aligned to position in a cache line
// --------------------------------------------------------------------------
// 
// --------------------------------------------------------------------------
// WRITE DATA ALIGNMENT MUX
//
// FIXME: once it works create a wrapper and push down a level of hierarchy.
// --------------------------------------------------------------------------
reg [255:0] pe_merge_wd;
//reg [31:0]  pe_merge_be;

reg [255:0] pe_line_wd;
reg [31:0]  pe_line_be;

reg [3:0] dary_write;
reg [3:0] tag_write;

reg [3:0] val_way_sel_d;
reg [3:0] mod_way_sel_d;
reg [3:0] lru_way_sel_d;

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
function [31:0] mux_bytes;
input [3:0] be;
input [31:0] wd,rd;
reg [7:0] b0,b1,b2,b3;
begin
  b0 = be[0] ? wd[ 7: 0] : rd[ 7: 0];
  b1 = be[1] ? wd[15: 8] : rd[15: 8];
  b2 = be[2] ? wd[23:16] : rd[23:16];
  b3 = be[3] ? wd[31:24] : rd[31:24];
  mux_bytes = {b3,b2,b1,b0};
end
endfunction

wire [31:0] wd0_d = mux_bytes(pe_be_d,pe_wd_d,mm_readdata[ 31:  0]);
wire [31:0] wd1_d = mux_bytes(pe_be_d,pe_wd_d,mm_readdata[ 63: 32]);
wire [31:0] wd2_d = mux_bytes(pe_be_d,pe_wd_d,mm_readdata[ 95: 64]);
wire [31:0] wd3_d = mux_bytes(pe_be_d,pe_wd_d,mm_readdata[127: 96]);
wire [31:0] wd4_d = mux_bytes(pe_be_d,pe_wd_d,mm_readdata[159:128]);
wire [31:0] wd5_d = mux_bytes(pe_be_d,pe_wd_d,mm_readdata[191:160]);
wire [31:0] wd6_d = mux_bytes(pe_be_d,pe_wd_d,mm_readdata[223:192]);
wire [31:0] wd7_d = mux_bytes(pe_be_d,pe_wd_d,mm_readdata[255:224]);

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
always @* begin

  pe_line_wd = 256'bx;
  pe_line_be =  32'b0;

  case(pe_offset_d)
    3'b000: pe_line_wd[ 31:  0] = pe_wd_d;
    3'b001: pe_line_wd[ 63: 32] = pe_wd_d;
    3'b010: pe_line_wd[ 95: 64] = pe_wd_d;
    3'b011: pe_line_wd[127: 96] = pe_wd_d;
    3'b100: pe_line_wd[159:128] = pe_wd_d;
    3'b101: pe_line_wd[191:160] = pe_wd_d;
    3'b110: pe_line_wd[223:192] = pe_wd_d;
    3'b111: pe_line_wd[255:224] = pe_wd_d;
  endcase

  case(pe_offset_d)
    3'b000: pe_line_be[ 3: 0] = pe_be_d;
    3'b001: pe_line_be[ 7: 4] = pe_be_d;
    3'b010: pe_line_be[11: 8] = pe_be_d;
    3'b011: pe_line_be[15:12] = pe_be_d;
    3'b100: pe_line_be[19:16] = pe_be_d;
    3'b101: pe_line_be[23:20] = pe_be_d;
    3'b110: pe_line_be[27:24] = pe_be_d;
    3'b111: pe_line_be[31:28] = pe_be_d;
  endcase

  case(pe_offset_d)
    3'b000: pe_merge_wd =
        { mm_readdata[255:224],mm_readdata[223:192],mm_readdata[191:160],mm_readdata[159:128],
          mm_readdata[127: 96],mm_readdata[ 95: 64],mm_readdata[ 63: 32],wd0_d          };
    3'b001: pe_merge_wd =
        { mm_readdata[255:224],mm_readdata[223:192],mm_readdata[191:160],mm_readdata[159:128],
          mm_readdata[127: 96],mm_readdata[ 95: 64],wd1_d         ,mm_readdata[ 31:  0] };
    3'b010: pe_merge_wd =
        { mm_readdata[255:224],mm_readdata[223:192],mm_readdata[191:160],mm_readdata[159:128],
          mm_readdata[127: 96],wd2_d         ,mm_readdata[ 63: 32],mm_readdata[ 31:  0] };
    3'b011: pe_merge_wd =
        { mm_readdata[255:224],mm_readdata[223:192],mm_readdata[191:160],mm_readdata[159:128],
          wd3_d         ,mm_readdata[ 95: 64],mm_readdata[ 63: 32],mm_readdata[ 31:  0] };
    3'b100: pe_merge_wd =
        { mm_readdata[255:224],mm_readdata[223:192],mm_readdata[191:160],wd4_d         ,
          mm_readdata[127: 96],mm_readdata[ 95: 64],mm_readdata[ 63: 32],mm_readdata[ 31:  0] };
    3'b101: pe_merge_wd =
        { mm_readdata[255:224],mm_readdata[223:192],wd5_d         ,mm_readdata[159:128],
          mm_readdata[127: 96],mm_readdata[ 95: 64],mm_readdata[ 63: 32],mm_readdata[ 31:  0] };
    3'b110: pe_merge_wd =
        { mm_readdata[255:224],wd6_d         ,mm_readdata[191:160],mm_readdata[159:128],
          mm_readdata[127: 96],mm_readdata[ 95: 64],mm_readdata[ 63: 32],mm_readdata[ 31:  0] };
    3'b111: pe_merge_wd =
        { wd7_d         ,mm_readdata[223:192],mm_readdata[191:160],mm_readdata[159:128],
          mm_readdata[127: 96],mm_readdata[ 95: 64],mm_readdata[ 63: 32],mm_readdata[ 31:  0] };
  endcase
end
// --------------------------------------------------------------------------
// Mux in FILL data if active
// --------------------------------------------------------------------------
wire  [255:0] line_wd;
wire  [31:0]  line_be;

assign line_wd  =  fsm_cc_rd_fill_d ? mm_readdata
                : (fsm_cc_wr_fill_d ? pe_merge_wd  : pe_line_wd);

assign line_be  =  fsm_cc_fill_d    ? 32'hFFFFFFFF : pe_line_be;
// --------------------------------------------------------------------------
// select an invalid way over eviction
// --------------------------------------------------------------------------
reg [3:0] inval_sel_d;

always @* begin
  inval_sel_d = 4'b0;
  casez(val_out_d)
    4'b0???: inval_sel_d[3] = 1'b1;
    4'b10??: inval_sel_d[2] = 1'b1;
    4'b110?: inval_sel_d[1] = 1'b1;
    4'b1110: inval_sel_d[0] = 1'b1;
    default: inval_sel_d    = 4'b0;
  endcase
end
// --------------------------------------------------------------------------
wire [3:0] fill_way_sel;
assign fill_way_sel[0] = (way_sel_d[0] | (inval_sel_d[0] & fsm_cc_fill_d));
assign fill_way_sel[1] = (way_sel_d[1] | (inval_sel_d[1] & fsm_cc_fill_d));
assign fill_way_sel[2] = (way_sel_d[2] | (inval_sel_d[2] & fsm_cc_fill_d));
assign fill_way_sel[3] = (way_sel_d[3] | (inval_sel_d[3] & fsm_cc_fill_d));

always @* begin
  dary_write[0] = fsm_cc_ary_write_d & fill_way_sel[0];
  dary_write[1] = fsm_cc_ary_write_d & fill_way_sel[1];
  dary_write[2] = fsm_cc_ary_write_d & fill_way_sel[2];
  dary_write[3] = fsm_cc_ary_write_d & fill_way_sel[3];

  tag_write[0]  = fsm_cc_tag_write_d & fill_way_sel[0];
  tag_write[1]  = fsm_cc_tag_write_d & fill_way_sel[1];
  tag_write[2]  = fsm_cc_tag_write_d & fill_way_sel[2];
  tag_write[3]  = fsm_cc_tag_write_d & fill_way_sel[3];

  val_way_sel_d[0]  = fsm_cc_val_write_d & fill_way_sel[0];
  val_way_sel_d[1]  = fsm_cc_val_write_d & fill_way_sel[1];
  val_way_sel_d[2]  = fsm_cc_val_write_d & fill_way_sel[2];
  val_way_sel_d[3]  = fsm_cc_val_write_d & fill_way_sel[3];

  mod_way_sel_d[0]  = fsm_cc_mod_write_d & fill_way_sel[0];
  mod_way_sel_d[1]  = fsm_cc_mod_write_d & fill_way_sel[1];
  mod_way_sel_d[2]  = fsm_cc_mod_write_d & fill_way_sel[2];
  mod_way_sel_d[3]  = fsm_cc_mod_write_d & fill_way_sel[3];

  lru_way_sel_d[0]  = fsm_cc_lru_write_d & fill_way_sel[0];
  lru_way_sel_d[1]  = fsm_cc_lru_write_d & fill_way_sel[1];
  lru_way_sel_d[2]  = fsm_cc_lru_write_d & fill_way_sel[2];
  lru_way_sel_d[3]  = fsm_cc_lru_write_d & fill_way_sel[3];
end
// --------------------------------------------------------------------------
reg [255:0] line_data;

always @(fsm_cc_fill_d,way_sel,dary_out[0],dary_out[1],
                       dary_out[2],dary_out[3])
begin
  casez({fsm_cc_fill_d,way_sel})
    5'b1????: line_data = mm_readdata;
    5'b0???1: line_data = dary_out[0];
    5'b0??1?: line_data = dary_out[1];
    5'b0?1??: line_data = dary_out[2];
    5'b01???: line_data = dary_out[3];
    default: line_data = 256'bx;
  endcase

end
reg [31:0] ard; //array read data
reg [31:0] frd; //fill read data

always @* begin
  case(pe_offset)
    3'b000: ard = line_data[ 31:  0];
    3'b001: ard = line_data[ 63: 32];
    3'b010: ard = line_data[ 95: 64];
    3'b011: ard = line_data[127: 96];
    3'b100: ard = line_data[159:128];
    3'b101: ard = line_data[191:160];
    3'b110: ard = line_data[223:192];
    3'b111: ard = line_data[255:224];
  endcase

  case(pe_offset_q)
    3'b000: frd = mm_rd_q[ 31:  0];
    3'b001: frd = mm_rd_q[ 63: 32];
    3'b010: frd = mm_rd_q[ 95: 64];
    3'b011: frd = mm_rd_q[127: 96];
    3'b100: frd = mm_rd_q[159:128];
    3'b101: frd = mm_rd_q[191:160];
    3'b110: frd = mm_rd_q[223:192];
    3'b111: frd = mm_rd_q[255:224];
  endcase

  rd = !rd_valid_d ? 32'bx : (!fsm_cc_rd_fill ? ard : frd);
end

// --------------------------------------------------------------------------
// FSM
// --------------------------------------------------------------------------
fsm #(.IDX_BITS(IDX_BITS),
      .TAG_BITS(TAG_BITS),
      .READ_HIT_LAT(READ_HIT_LAT)
) fsm0 (

  //inputs
  .pe_read_d    (pe_read_d),
  .pe_write_d   (pe_write_d),
  .pe_req_hit_d (req_hit_d),
  .pe_req_miss_d(req_miss_d),
  .pe_req_mod_d (req_mod_d),

  .fsm_cc_ary_write_d(fsm_cc_ary_write_d),
  .fsm_cc_tag_write_d(fsm_cc_tag_write_d),
  .fsm_cc_val_write_d(fsm_cc_val_write_d),
  .fsm_cc_mod_write_d(fsm_cc_mod_write_d),
  .fsm_cc_lru_write_d(fsm_cc_lru_write_d),

  .fsm_cc_is_val_d (fsm_cc_is_val_d),
  .fsm_cc_is_mod_d (fsm_cc_is_mod_d),
  .fsm_cc_rd_fill_d(fsm_cc_rd_fill_d),
  .fsm_cc_wr_fill_d(fsm_cc_wr_fill_d),

  .fsm_cc_readdata_valid(fsm_cc_readdata_valid),

  .fsm_mm_read_d(fsm_mm_read_d),
  .fsm_mm_write_d(mm_write_d),

  .mm_readdata_valid(mm_readdata_valid),

  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
// STATUS BITS
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
bitrf valid0
(
  .q(val_out_d),
  .way_sel(val_way_sel_d),

  .ra(pe_index_d),
  .wa(pe_index_d),

  .wr(fsm_cc_val_write_d),
  .d(fsm_cc_is_val_d),

  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
bitrf dirty0
(
  .q(mod_out_d),
  .way_sel(mod_way_sel_d),

  .ra(pe_index_d),
  .wa(pe_index_d),
  .wr(fsm_cc_mod_write_d),
  .d(fsm_cc_is_mod_d),

  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
wire [1:0] lru_way_d;
lrurf lrurf0
(
  .q(lru_out_d),
  .lru_way(lru_way_d),

  .way_sel(lru_way_sel_d),
  .ra(pe_index_d),
  .wa(pe_index_d),
  .wr(fsm_cc_lru_write_d),

  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
// TAGS
// --------------------------------------------------------------------------
genvar WAYVAR;
generate for(WAYVAR=0;WAYVAR<4;WAYVAR=WAYVAR+1) begin : tags
  sram #(.DATA_WIDTH(TAG_BITS),.ADDR_WIDTH(IDX_BITS)) tag (
    .a    (pe_index_d),
    .wd   (pe_tag_d),
    .rd   (tag_out_d[WAYVAR]),
    .write(tag_write[WAYVAR]),
    .read (cache_read_d),
    .clk  (clk)
  ); 
end
endgenerate  

// --------------------------------------------------------------------------
// DATA ARRAYS
//
// I removed the generate statement as part of debugging an initialization 
// problem in the test bench. The problem was state from a previous test 
// remained in the data arrays even when the current test's readmemh's 
// should have cleared the state from the previous test.
//
// The path to these modules is everywhere so I'm not going to change it back.
// --------------------------------------------------------------------------
//generate for(WAYVAR=0;WAYVAR<4;WAYVAR=WAYVAR+1) begin : data
//  dsram #(.ADDR_WIDTH(IDX_BITS)) dsram (
//    .a     (pe_index_d),
//    .wd    (line_wd),
//    .be    (line_be),
//    .rd    (dary_out[WAYVAR]),
//    .write (dary_write[WAYVAR]),
//    .read  (cache_read_d),
//    .clk   (clk)
//  );
//end
//endgenerate
// --------------------------------------------------------------------------
dsram #(.ADDR_WIDTH(IDX_BITS)) dsram0 (
    .a     (pe_index_d),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[0]),
    .write (dary_write[0]),
    .read  (cache_read_d),
    .clk   (clk)
);

dsram #(.ADDR_WIDTH(IDX_BITS)) dsram1 (
    .a     (pe_index_d),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[1]),
    .write (dary_write[1]),
    .read  (cache_read_d),
    .clk   (clk)
);

dsram #(.ADDR_WIDTH(IDX_BITS)) dsram2 (
    .a     (pe_index_d),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[2]),
    .write (dary_write[2]),
    .read  (cache_read_d),
    .clk   (clk)
);

dsram #(.ADDR_WIDTH(IDX_BITS)) dsram3 (
    .a     (pe_index_d),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[3]),
    .write (dary_write[3]),
    .read  (cache_read_d),
    .clk   (clk)
);

endmodule
