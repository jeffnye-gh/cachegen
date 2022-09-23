`include "sim_cntrl.h"
// -----------------------------------------------------------------------
// 8192 x 12b, 1WR, 2RD port (2nd is internal only)
//
// 13b address
// 12b data
// -----------------------------------------------------------------------
module regfile
(
  output wire [11:0] rdallways,
  output reg  [2:0]  rd,

  input  wire [12:0] wa,
  input  wire [3:0]  wwaysel,

  input  wire [12:0] ra,
  input  wire [3:0]  rwaysel,
  input  wire wr,
  input  clk
);
// ---------------------------------------------------------------------------
reg  [2:0] array[0:8192];
// ---------------------------------------------------------------------------
assign rdallways = array[ra];
assign rmwdata   = array[wa];

reg [2:0] wd;
// ---------------------------------------------------------------------------
always @*
begin
  casez(rwaysel)  
    4'b???1: rd = rdallways[2:0];
    4'b??1?: rd = rdallways[5:3];
    4'b?1??: rd = rdallways[8:6];
    4'b1???: rd = rdallways[11:9];
    default: rd = 3'bx;
  endcase
end
// ---------------------------------------------------------------------------
always @(posedge clk) begin
  array[wa][ 2:0] <= wr & wwaysel[0] ? wd : array[wa][ 2: 0];
  array[wa][ 5:3] <= wr & wwaysel[1] ? wd : array[wa][ 5: 3];
  array[wa][ 8:6] <= wr & wwaysel[2] ? wd : array[wa][ 8: 6];
  array[wa][11:9] <= wr & wwaysel[3] ? wd : array[wa][11: 9];
end
endmodule
