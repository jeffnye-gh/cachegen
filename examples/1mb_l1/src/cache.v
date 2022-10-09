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
  output wire          rd_valid,

  output wire         req_hit,   //status output
  output wire         req_miss,
  output wire         req_mod,

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
  output wire [31:0]   mm_be,    //to MM, byte enables, FIXME: needed?
  output wire [255:0]  mm_wd,    // line eviction data
  output wire          mm_write, //to MM, write command
  output wire          mm_read,  //to MM, read command

  input  wire [255:0]  mm_rd,    // line fill data
  input  wire          mm_readdata_valid, //fill data valid

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
wire       fsm_cc_fill = 1'b0;
wire [3:0] fsm_cc_tag_write = 4'b0;
assign mm_be = 32'hFFFFFFFF;

assign mm_a = a;  
// ------------------------------------------------------------------------
wire [CACHELINE_BITS-1:0] fsm_cc_wd;
wire [CACHELINE_BITS-1:0] dary_out[3:0];

reg  [TAG_BITS-1:0] ag_wd;
wire [TAG_BITS-1:0] tag_out_d[3:0];

wire [3:0] tag_write;

wire lru_wr,mod_wr;
wire [3:0] val_out_d;
wire [3:0] mod_out_d;
wire [2:0] lru_out_d;

wire req_hit_d;
wire req_miss_d;
wire req_mod_d;
wire fsm_mm_read_d;
wire fsm_mm_write_d;

wire fsm_cc_readdata_valid;

wire fsm_cc_ary_write_d;
wire fsm_cc_tag_write_d;
wire fsm_cc_val_write_d;
wire fsm_cc_mod_write_d;
wire fsm_cc_lru_write_d;

wire fsm_cc_is_val_d;
wire fsm_cc_is_mod_d;
wire fsm_cc_fill_d;

wire pe_flush;
wire pe_flush_all;
wire pe_invalidate;
wire pe_invalidate_all;

wire [3:0] way_hit_d;
reg  [3:0] way_hit;
wire [3:0] way_mod_d;
wire [3:0] pe_compare_d;
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
reg  [TAG_BITS-1:0] pe_tag;
reg  [IDX_BITS-1:0] pe_index;
reg  [2:0]          pe_offset;
reg  [3:0]          pe_be;
reg  [31:0]         pe_wd;

wire pe_access_d = (pe_read_d | pe_write_d) & !reset;
always @(posedge clk) begin
  pe_tag    <= pe_tag_d;
  pe_offset <= pe_offset_d;
  pe_index  <= pe_index_d;
  pe_be     <= pe_be_d;
  pe_wd     <= pe_wd_d;
  way_hit   <= way_hit_d;
end
// --------------------------------------------------------------------------
// select logic
// --------------------------------------------------------------------------
wire [3:0] tag_compare_d;
assign tag_compare_d[0] = tag_out_d[0] == pe_tag_d;
assign tag_compare_d[1] = tag_out_d[1] == pe_tag_d;
assign tag_compare_d[2] = tag_out_d[2] == pe_tag_d;
assign tag_compare_d[3] = tag_out_d[3] == pe_tag_d;

assign pe_compare_d[0] = val_out_d[0] & tag_compare_d[0];
assign pe_compare_d[1] = val_out_d[1] & tag_compare_d[1];
assign pe_compare_d[2] = val_out_d[2] & tag_compare_d[2];
assign pe_compare_d[3] = val_out_d[3] & tag_compare_d[3];

assign way_hit_d[0] = pe_access_d & pe_compare_d[0];
assign way_hit_d[1] = pe_access_d & pe_compare_d[1];
assign way_hit_d[2] = pe_access_d & pe_compare_d[2]; 
assign way_hit_d[3] = pe_access_d & pe_compare_d[3];

assign way_mod_d[0] = way_hit_d[0] & mod_out_d[0];
assign way_mod_d[1] = way_hit_d[1] & mod_out_d[1];
assign way_mod_d[2] = way_hit_d[2] & mod_out_d[2];
assign way_mod_d[3] = way_hit_d[3] & mod_out_d[3];

//external name - just renaming
//assign req_hit  = req_hit_d;
assign req_hit  = 1'b1;
assign req_miss = req_miss_d;
assign req_mod  = req_mod_d;
assign rd_valid = fsm_cc_readdata_valid;
assign mm_read  = fsm_mm_read_d;
assign mm_write = fsm_mm_write_d;

assign req_hit_d  = |way_hit_d & pe_access_d;
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
reg [255:0] pe_line_wd;
reg [31:0]  pe_line_be;

reg [3:0] dary_write;

always @* begin

  pe_line_wd = 256'bx;
  pe_line_be =  32'b0;

  case(pe_offset)
    3'b000: pe_line_wd[ 31:  0] = pe_wd;
    3'b001: pe_line_wd[ 63: 32] = pe_wd;
    3'b010: pe_line_wd[ 95: 64] = pe_wd;
    3'b011: pe_line_wd[127: 96] = pe_wd;
    3'b100: pe_line_wd[159:128] = pe_wd;
    3'b101: pe_line_wd[191:160] = pe_wd;
    3'b110: pe_line_wd[223:192] = pe_wd;
    3'b111: pe_line_wd[255:224] = pe_wd;
  endcase

  case(pe_offset)
    3'b000: pe_line_be[ 3: 0] = pe_be;
    3'b001: pe_line_be[ 7: 4] = pe_be;
    3'b010: pe_line_be[11: 8] = pe_be;
    3'b011: pe_line_be[15:12] = pe_be;
    3'b100: pe_line_be[19:16] = pe_be;
    3'b101: pe_line_be[23:20] = pe_be;
    3'b110: pe_line_be[27:24] = pe_be;
    3'b111: pe_line_be[31:28] = pe_be;
  endcase
end
// --------------------------------------------------------------------------
// Mux in FILL data if active
// --------------------------------------------------------------------------
wire  [255:0] line_wd;
wire  [31:0]  line_be;
assign line_wd  = fsm_cc_fill ? mm_rd        : pe_line_wd; 
assign line_be  = fsm_cc_fill ? 32'hFFFFFFFF : pe_line_be; 
// --------------------------------------------------------------------------
always @* begin
  dary_write[0] = way_hit_d[0] & fsm_cc_ary_write_d;
  dary_write[1] = way_hit_d[1] & fsm_cc_ary_write_d;
  dary_write[2] = way_hit_d[2] & fsm_cc_ary_write_d;
  dary_write[3] = way_hit_d[3] & fsm_cc_ary_write_d;
end
// --------------------------------------------------------------------------
reg [255:0] line_data;

always @(way_hit,dary_out[0],dary_out[1],
                 dary_out[2],dary_out[3])
begin
  casez(way_hit)
    4'b???1: line_data = dary_out[0];
    4'b??1?: line_data = dary_out[1];
    4'b?1??: line_data = dary_out[2];
    4'b1???: line_data = dary_out[3];
    default: line_data = 256'bx;
  endcase

end

always @(pe_offset,line_data) begin
  case(pe_offset)
    3'b000: rd = line_data[ 31:  0];
    3'b001: rd = line_data[ 63: 32];
    3'b010: rd = line_data[ 95: 64];
    3'b011: rd = line_data[127: 96];
    3'b100: rd = line_data[159:128];
    3'b101: rd = line_data[191:160];
    3'b110: rd = line_data[223:192];
    3'b111: rd = line_data[255:224];
  endcase
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

  .fsm_cc_is_val_d   (fsm_cc_is_val_d),
  .fsm_cc_is_mod_d   (fsm_cc_is_mod_d),
  .fsm_cc_fill_d     (fsm_cc_fill_d),

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
  .rd(val_out_d),
  .wa(pe_index),
  .way_hit(way_hit_d),
  .ra(pe_index_d),
  .wr(fsm_cc_val_write_d),
  .in(fsm_cc_is_val_d),
  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
bitrf dirty0
(
  .rd(mod_out_d),
  .wa(pe_index),
  .way_hit(way_hit_d),
  .ra(pe_index_d),
  .wr(fsm_cc_mod_write_d),
  .in(fsm_cc_is_mod_d),
  .reset(reset),
  .clk(clk)
);
// --------------------------------------------------------------------------
lrurf lrurf0
(
  .rd(lru_out_d),
  .wa(pe_index_d),
  .way_hit(way_hit_d),
  .ra(pe_index_d),
  .wr(fsm_cc_lru_write_d),
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
    .a    (pe_index_d),
    .wd   (tag_wd),
    .rd   (tag_out_d[WAYVAR]),
    .write(fsm_cc_tag_write[WAYVAR]),
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
//    .aq    (pe_index),
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
    .aq    (pe_index),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[0]),
    .write (dary_write[0]),
    .read  (cache_read_d),
    .clk   (clk)
);

dsram #(.ADDR_WIDTH(IDX_BITS)) dsram1 (
    .a     (pe_index_d),
    .aq    (pe_index),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[1]),
    .write (dary_write[1]),
    .read  (cache_read_d),
    .clk   (clk)
);

dsram #(.ADDR_WIDTH(IDX_BITS)) dsram2 (
    .a     (pe_index_d),
    .aq    (pe_index),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[2]),
    .write (dary_write[2]),
    .read  (cache_read_d),
    .clk   (clk)
);

dsram #(.ADDR_WIDTH(IDX_BITS)) dsram3 (
    .a     (pe_index_d),
    .aq    (pe_index),
    .wd    (line_wd),
    .be    (line_be),
    .rd    (dary_out[3]),
    .write (dary_write[3]),
    .read  (cache_read_d),
    .clk   (clk)
);

endmodule
