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

  output wire          ready_d,   //status output

  //From PE/TB
  input  wire [31:0]  a,
  input  wire [3:0]   be,
  input  wire         read,
  input  wire         write,
  input  wire [31:0]  wd,

  //From cache to main memory
  output wire [31:0]   mm_a,
  output wire          mm_write_d, //to MM, write command
  output wire          mm_read_d,  //to MM, read command
  output reg  [255:0]  mm_writedata,      // line eviction data

  input  wire [255:0]  mm_readdata,    // line fill data
  input  wire          mm_readdata_valid, //fill data valid
  input  wire          mm_ready,  //can accept next access

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
assign mm_a = fsm_cc_use_evict_add_d ? {evict_tag_add_d,18'b0} : a;
// ------------------------------------------------------------------------
wire [CACHELINE_BITS-1:0] fsm_cc_wd;
wire [CACHELINE_BITS-1:0] dary_out[3:0];

reg  [TAG_BITS-1:0] ag_wd;
wire [TAG_BITS-1:0] tag_out_d[3:0];
reg  [TAG_BITS-1:0] evict_tag_add_d;

wire lru_wr,mod_wr;
wire [3:0] val_output_d;
wire [3:0] mod_output_d;
wire [2:0] lru_output_d;

wire fsm_mm_read_d;
wire fsm_mm_write_d;

wire fsm_cc_use_evict_add_d;
wire fsm_cc_readdata_valid;

wire fsm_cc_ary_write_d;
//wire fsm_cc_tag_write_d;
wire fsm_cc_val_write_d;
wire fsm_cc_mod_write_d;
wire fsm_cc_lru_write_d;

wire fsm_cc_is_val_d;
wire fsm_cc_is_mod_d;
wire cc_fill_d,fsm_cc_rd_fill_d,fsm_cc_wr_fill_d;
reg  fsm_cc_rd_fill,fsm_cc_evict;
wire fsm_cc_evict_d;

wire pe_flush;
wire pe_flush_all;
wire pe_invalidate;
wire pe_invalidate_all;

wire [3:0] way_hit_d;
reg  [3:0] way_hit;
wire [3:0] way_mod_d;
wire       way_is_selected_d;
wire [3:0] lru_selected_way_d;
//wire [3:0] select_by_invalid_d;
wire fsm_cc_ready_d;
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
reg [255:0]  mm_readdata_q;

wire pe_access_d = (pe_read_d | pe_write_d) & !reset;
assign cc_fill_d = fsm_cc_rd_fill_d | fsm_cc_wr_fill_d;

always @(posedge clk) begin
  fsm_cc_rd_fill <= fsm_cc_rd_fill_d;
  fsm_cc_evict   <= fsm_cc_evict_d;

  pe_offset   <= pe_offset_d;
  pe_offset_q <= pe_offset;

  pe_be   <= pe_be_d;
  pe_wd   <= pe_wd_d;
  way_hit <= way_hit_d;
  mm_readdata_q <= mm_readdata;
end
// --------------------------------------------------------------------------
// renaming
// --------------------------------------------------------------------------
assign rd_valid_d = fsm_cc_readdata_valid;
assign mm_read_d  = fsm_mm_read_d;
assign mm_write_d = fsm_mm_write_d;
assign ready_d    = fsm_cc_ready_d;

// --------------------------------------------------------------------------
//wire cc_fsm_req_hit_d =  way_is_selected_d & pe_access_d;
//assign req_hit_d = fsm_cc_ready_d;
//assign req_hit_d      =  cc_fsm_req_hit_d
//                      |  cc_fill_d
//                      |  (mm_ready & fsm_cc_evict_d);
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
wire [255:0] pe_merge_wd;
//reg [31:0]  pe_merge_be;

reg [255:0] pe_line_wd;
reg [31:0]  pe_line_be;

reg [3:0] dary_way_sel_d;
reg [3:0] tag_way_sel_d;
reg [3:0] val_way_sel_d;
reg [3:0] mod_way_sel_d;
reg [3:0] lru_way_sel_d;

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
//function [31:0] mux_bytes;
//input [3:0] be;
//input [31:0] wd,rd;
//reg [7:0] b0,b1,b2,b3;
//begin
//  b0 = be[0] ? wd[ 7: 0] : rd[ 7: 0];
//  b1 = be[1] ? wd[15: 8] : rd[15: 8];
//  b2 = be[2] ? wd[23:16] : rd[23:16];
//  b3 = be[3] ? wd[31:24] : rd[31:24];
//  mux_bytes = {b3,b2,b1,b0};
//end
//endfunction
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

end

// ------------------------------------------------------------------------
// lru_selected_way_d is always 1-hot 
// ------------------------------------------------------------------------
always @* begin
  casez(lru_selected_way_d)
    4'b1???: mm_writedata  = dary_out[3];
    4'b?1??: mm_writedata  = dary_out[2]; 
    4'b??1?: mm_writedata  = dary_out[1];
    4'b???1: mm_writedata  = dary_out[0];
    default  mm_writedata = 256'bx;
  endcase
end

always @* begin
  casez(lru_selected_way_d)
    4'b1???: evict_tag_add_d  = tag_out_d[3];
    4'b?1??: evict_tag_add_d  = tag_out_d[2]; 
    4'b??1?: evict_tag_add_d  = tag_out_d[1];
    4'b???1: evict_tag_add_d  = tag_out_d[0];
    default  evict_tag_add_d  = 14'bx;
  endcase
end
// --------------------------------------------------------------------------
// Mux in FILL data if active
// --------------------------------------------------------------------------
wire  [255:0] line_wd;
wire  [31:0]  line_be;

assign line_wd  =  fsm_cc_rd_fill_d ? mm_readdata
                : (fsm_cc_wr_fill_d ? pe_merge_wd  
                : (fsm_cc_evict_d   ? mm_readdata : pe_line_wd));

assign line_be  =  cc_fill_d    ? 32'hFFFFFFFF : pe_line_be;
// --------------------------------------------------------------------------
wire [3:0] fill_way_sel;
wire [3:0] fill_or_victim_way_d;

assign fill_way_sel[0]
  = way_hit_d[0] ? 1'b1 : fill_or_victim_way_d[0] & cc_fill_d;
assign fill_way_sel[1]
  = way_hit_d[1] ? 1'b1 : fill_or_victim_way_d[1] & cc_fill_d;
assign fill_way_sel[2]
  = way_hit_d[2] ? 1'b1 : fill_or_victim_way_d[2] & cc_fill_d;
assign fill_way_sel[3]
  = way_hit_d[3] ? 1'b1 : fill_or_victim_way_d[3] & cc_fill_d;

always @* begin
  dary_way_sel_d = {4{fsm_cc_ary_write_d}} & fill_way_sel;
  tag_way_sel_d  = {4{fsm_cc_tag_write_d}} & fill_way_sel;
  val_way_sel_d  = {4{fsm_cc_val_write_d}} & fill_way_sel;
  mod_way_sel_d  = {4{fsm_cc_mod_write_d}} & fill_way_sel;
  lru_way_sel_d  = {4{fsm_cc_lru_write_d}} & fill_way_sel;
end
// --------------------------------------------------------------------------
reg [255:0] line_data;

always @(cc_fill_d,way_hit,dary_out[0],dary_out[1],
                       dary_out[2],dary_out[3])
begin
  casez({cc_fill_d,way_hit})
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
    3'b000: frd = mm_readdata_q[ 31:  0];
    3'b001: frd = mm_readdata_q[ 63: 32];
    3'b010: frd = mm_readdata_q[ 95: 64];
    3'b011: frd = mm_readdata_q[127: 96];
    3'b100: frd = mm_readdata_q[159:128];
    3'b101: frd = mm_readdata_q[191:160];
    3'b110: frd = mm_readdata_q[223:192];
    3'b111: frd = mm_readdata_q[255:224];
  endcase

  rd = !rd_valid_d ? 32'bx : (!fsm_cc_rd_fill ? ard : frd);
end
// ------------------------------------------------------------------------
// MERGE
// ------------------------------------------------------------------------
merge mrg0
(
  .y(pe_merge_wd),

  .rd(mm_readdata),
  .wd(pe_wd_d),

  .be(pe_be_d),
  .sel(pe_offset_d)

);
// --------------------------------------------------------------------------
// COMPARE
// --------------------------------------------------------------------------
wire cmp_fsm_req_clean_d;
compare #(.WIDTH(TAG_BITS)
) cmp0 (
  .way_hit_d(way_hit_d),
  .way_is_selected_d(way_is_selected_d),
  .req_clean_d (cmp_fsm_req_clean_d),

  .victim_way_is_dirty_d(victim_way_is_dirty_d),
  .lru_way_is_dirty_d(lru_way_is_dirty_d),

  .lru_selected_way_d(lru_selected_way_d),
  .fill_or_victim_way_d(fill_or_victim_way_d),

  .tag_way0_d(tag_out_d[0]),
  .tag_way1_d(tag_out_d[1]),
  .tag_way2_d(tag_out_d[2]),
  .tag_way3_d(tag_out_d[3]),

  .pe_tag_d(pe_tag_d),
  
  .lru_output_d(lru_output_d),
  .val_output_d(val_output_d),
  .mod_output_d(mod_output_d),

  .pe_access_d(pe_access_d)
);
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

//  .cc_fsm_req_hit_d (cc_fsm_req_hit_d),
  .way_is_selected_d  (way_is_selected_d),
  .cmp_fsm_req_clean_d(cmp_fsm_req_clean_d),

  .fsm_cc_ary_write_d(fsm_cc_ary_write_d),
  .fsm_cc_tag_write_d(fsm_cc_tag_write_d),
  .fsm_cc_val_write_d(fsm_cc_val_write_d),
  .fsm_cc_mod_write_d(fsm_cc_mod_write_d),
  .fsm_cc_lru_write_d(fsm_cc_lru_write_d),

  .fsm_cc_is_val_d (fsm_cc_is_val_d),
  .fsm_cc_is_mod_d (fsm_cc_is_mod_d),
  .fsm_cc_rd_fill_d(fsm_cc_rd_fill_d),
  .fsm_cc_wr_fill_d(fsm_cc_wr_fill_d),

  .fsm_cc_evict_d  (fsm_cc_evict_d),

  .fsm_cc_ready_d(fsm_cc_ready_d),
  .fsm_cc_readdata_valid(fsm_cc_readdata_valid),

  .fsm_cc_use_evict_add_d(fsm_cc_use_evict_add_d),

  .fsm_mm_read_d(fsm_mm_read_d),
  .fsm_mm_write_d(mm_write_d),

  .mm_readdata_valid(mm_readdata_valid),
  .mm_ready(mm_ready),

  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
// STATUS BITS
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
bitrf valid0
(
  .q(val_output_d),

  .index(pe_index_d),
  .way(val_way_sel_d),

  .wr(fsm_cc_val_write_d),
  .d(fsm_cc_is_val_d),

  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
bitrf dirty0
(
  .q(mod_output_d),

  .index(pe_index_d),
  .way(mod_way_sel_d),

  .wr(fsm_cc_mod_write_d),
  .d(fsm_cc_is_mod_d),

  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
lrurf lrurf0
(
  .q(lru_output_d),           //these are the bit values

  .index(pe_index_d),
  .way(lru_way_sel_d), //input

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
    .write(tag_way_sel_d[WAYVAR]),
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
//    .write (dary_way_sel_d[WAYVAR]),
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
    .write (dary_way_sel_d[0]),
    .read  (cache_read_d),
    .clk   (clk)
);

dsram #(.ADDR_WIDTH(IDX_BITS)) dsram1 (
    .a     (pe_index_d),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[1]),
    .write (dary_way_sel_d[1]),
    .read  (cache_read_d),
    .clk   (clk)
);

dsram #(.ADDR_WIDTH(IDX_BITS)) dsram2 (
    .a     (pe_index_d),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[2]),
    .write (dary_way_sel_d[2]),
    .read  (cache_read_d),
    .clk   (clk)
);

dsram #(.ADDR_WIDTH(IDX_BITS)) dsram3 (
    .a     (pe_index_d),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[3]),
    .write (dary_way_sel_d[3]),
    .read  (cache_read_d),
    .clk   (clk)
);

endmodule
