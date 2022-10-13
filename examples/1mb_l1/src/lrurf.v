`include "sim_cntrl.h"
// -----------------------------------------------------------------------
// LRU regfile
//
// 8192 x 12b, 1WR, 2RD port (2nd is internal only)
//
// 13b address
// 12b data
//
// -----------------------------------------------------------------------
module lrurf
(
  output reg  [2:0]  rd,

  input  wire [12:0] wa,
  input  wire [3:0]  way_sel,

  input  wire [12:0] ra,
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
assign rd = regs[ra];
// ---------------------------------------------------------------------------
// LRU rules
// access to way0    b2=0  b1=b1  b0=0
// access to way1    b2=0  b1=b1  b0=1 
// access to way2    b2=1  b1=0   b0=b0
// access to way3    b2=1  b1=1   b0=b0
// ---------------------------------------------------------------------------
always @* begin
  casez(way_sel)
    4'b???1: wd =  { 1'b0, rd[1], 1'b0       }; //WAY0
    4'b??1?: wd =  { 1'b0, rd[1], 1'b1       }; //WAY1
    4'b?1??: wd =  { 1'b1, 1'b0,  rd[0] }; //WAY2
    4'b1???: wd =  { 1'b1, 1'b1,  rd[0] }; //WAY3
    default: wd = 3'bx;
  endcase
end
// ---------------------------------------------------------------------------
always @(posedge clk) begin
  if(reset) clearlru;
end

always @(posedge clk) begin
  regs[wa] <= wr ? wd : regs[wa];
end
endmodule
