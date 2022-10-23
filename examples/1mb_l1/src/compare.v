`include "sim_cntrl.h"
module compare #(
  parameter WIDTH = 14
)
(
  output wire  [3:0] way_hit_d,
  output wire  way_is_selected_d,
  output wire  victim_way_is_dirty_d,
  output wire  lru_way_is_dirty_d,
  output wire  req_clean_d,

  output reg   [3:0] lru_selected_way_d,
  output wire  [3:0] fill_or_victim_way_d,

  input  wire [WIDTH-1:0] tag_way0_d,
  input  wire [WIDTH-1:0] tag_way1_d,
  input  wire [WIDTH-1:0] tag_way2_d,
  input  wire [WIDTH-1:0] tag_way3_d,

  input  wire [WIDTH-1:0] pe_tag_d,

  input  wire [3:0] lru_output_d,
  input  wire [3:0] val_output_d,
  input  wire [3:0] mod_output_d,

  input wire pe_access_d
);

wire [3:0] tag_compare_d;
assign tag_compare_d[0] = tag_way0_d == pe_tag_d;
assign tag_compare_d[1] = tag_way1_d == pe_tag_d;
assign tag_compare_d[2] = tag_way2_d == pe_tag_d;
assign tag_compare_d[3] = tag_way3_d == pe_tag_d;

//Way is valid and tag matches 
assign way_hit_d[0] = pe_access_d & val_output_d[0] & tag_compare_d[0];
assign way_hit_d[1] = pe_access_d & val_output_d[1] & tag_compare_d[1];
assign way_hit_d[2] = pe_access_d & val_output_d[2] & tag_compare_d[2];
assign way_hit_d[3] = pe_access_d & val_output_d[3] & tag_compare_d[3];

wire [3:0] way_mod_d;
assign way_mod_d[0] = fill_or_victim_way_d[0] & mod_output_d[0];
assign way_mod_d[1] = fill_or_victim_way_d[1] & mod_output_d[1];
assign way_mod_d[2] = fill_or_victim_way_d[2] & mod_output_d[2];
assign way_mod_d[3] = fill_or_victim_way_d[3] & mod_output_d[3];

wire [3:0] lru_dirty_d;
assign lru_dirty_d[0]  = mod_output_d[0] & lru_output_d == 2'b00;
assign lru_dirty_d[1]  = mod_output_d[1] & lru_output_d == 2'b01;
assign lru_dirty_d[2]  = mod_output_d[2] & lru_output_d == 2'b10;
assign lru_dirty_d[3]  = mod_output_d[3] & lru_output_d == 2'b11;

assign way_is_selected_d = |way_hit_d;
wire no_victim = !way_is_selected_d;
assign victim_way_is_dirty_d = no_victim ? 1'b0 : |way_mod_d;
assign lru_way_is_dirty_d    = |lru_dirty_d;

// req is clean if there is one invalid, or the lru selected way is not dirty
assign req_clean_d = one_invalid_d | !lru_way_is_dirty_d;
// -------------------------------------------------------------------
// Victim selection
//  - the left most invalid way 
//  - or LRU picks
// -------------------------------------------------------------------
reg [3:0] select_by_invalid_d;
always @* begin
  select_by_invalid_d = 4'b0;
  casez(val_output_d)
    4'b0???: select_by_invalid_d[3] = 1'b1;
    4'b10??: select_by_invalid_d[2] = 1'b1;
    4'b110?: select_by_invalid_d[1] = 1'b1;
    4'b1110: select_by_invalid_d[0] = 1'b1;
    default: select_by_invalid_d    = 4'b0;
  endcase
end
// ---------------------------------------------------------------------------
// LRU decoder - see README.txt
// ---------------------------------------------------------------------------
always @* begin
  case(lru_output_d)
    3'b000 : lru_selected_way_d = 4'b1000; //2'd3;
    3'b001 : lru_selected_way_d = 4'b1000; //2'd3;
    3'b010 : lru_selected_way_d = 4'b0100; //2'd2;
    3'b011 : lru_selected_way_d = 4'b0100; //2'd2;
    3'b100 : lru_selected_way_d = 4'b0010; //2'd1;
    3'b101 : lru_selected_way_d = 4'b0001; //2'd0;
    3'b110 : lru_selected_way_d = 4'b0010; //2'd1;
    3'b111 : lru_selected_way_d = 4'b0001; //2'd0;
    default: lru_selected_way_d = 2'bx;
  endcase
end
// ---------------------------------------------------------------------------
// Way selection
//
// - if all_valid then output is lru else select_by_invalid_d;
// - 
// ---------------------------------------------------------------------------
wire one_invalid_d = |select_by_invalid_d;
assign fill_or_victim_way_d = one_invalid_d ? select_by_invalid_d 
                                            : lru_selected_way_d;
endmodule
