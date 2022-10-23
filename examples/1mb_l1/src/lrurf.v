`include "sim_cntrl.h"
// -----------------------------------------------------------------------
// LRU regfile
//
// 8192 x 3b
//
// 13b address
// 3b  data {b2,b1,b0}
//
// -----------------------------------------------------------------------
module lrurf
(
  output reg  [2:0]  q,
//  output reg  [1:0]  lru_way,

  input  wire [12:0] index,
  input  wire [3:0]  way,

  input  wire        wr,

  input  wire        reset,
  input  wire        clk
);
// ----------------------------------------------------------------------
localparam int ENTRIES = 8192;
// ----------------------------------------------------------------------
task clearlru;
integer i;
begin for(i=0;i<ENTRIES;++i) regs[i] = 3'b0; end
endtask
// ---------------------------------------------------------------------------
reg  [2:0] regs[0:8192];
reg  [2:0]  wd;
// ---------------------------------------------------------------------------
assign q = regs[index];
// ---------------------------------------------------------------------------
// LRU rules - see README.txt
// ---------------------------------------------------------------------------
always @* begin
  casez(way)
    4'b???1: wd =  { 1'b0, q[1], 1'b0       }; //WAY0
    4'b??1?: wd =  { 1'b0, q[1], 1'b1       }; //WAY1
    4'b?1??: wd =  { 1'b1, 1'b0,  q[0] }; //WAY2
    4'b1???: wd =  { 1'b1, 1'b1,  q[0] }; //WAY3
    default: wd = 3'bx;
  endcase
end
//// ---------------------------------------------------------------------------
//// LRU decoder - see README.txt
//// ---------------------------------------------------------------------------
//always @* begin
//  case(q)
//    3'b000 : lru_way = 2'd3;
//    3'b001 : lru_way = 2'd3;
//    3'b010 : lru_way = 2'd2;
//    3'b011 : lru_way = 2'd2;
//    3'b100 : lru_way = 2'd1;
//    3'b101 : lru_way = 2'd0;
//    3'b110 : lru_way = 2'd1;
//    3'b111 : lru_way = 2'd0;
//    default: lru_way = 2'bx;
//  endcase
//end
// ---------------------------------------------------------------------------
always @(posedge clk) begin
  if(reset) clearlru;
end

always @(posedge clk) begin
  regs[index] <= wr ? wd : regs[index];
end
endmodule
