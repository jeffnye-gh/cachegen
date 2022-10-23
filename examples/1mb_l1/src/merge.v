`include "sim_cntrl.h"
module merge
(
  output reg [255:0] y,

  input wire [255:0] rd, //read data
  input wire [31:0]  wd, //pe_wd_d

  input wire [3:0] be,
  input wire [2:0] sel
);

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

wire [31:0] r0 = rd[ 31:  0];
wire [31:0] r1 = rd[ 63: 32];
wire [31:0] r2 = rd[ 95: 64];
wire [31:0] r3 = rd[127: 96];
wire [31:0] r4 = rd[159:128];
wire [31:0] r5 = rd[191:160];
wire [31:0] r6 = rd[223:192];
wire [31:0] r7 = rd[255:224];

wire [31:0] _0 = mux_bytes(be,wd,r0);
wire [31:0] _1 = mux_bytes(be,wd,r1);
wire [31:0] _2 = mux_bytes(be,wd,r2);
wire [31:0] _3 = mux_bytes(be,wd,r3);
wire [31:0] _4 = mux_bytes(be,wd,r4);
wire [31:0] _5 = mux_bytes(be,wd,r5);
wire [31:0] _6 = mux_bytes(be,wd,r6);
wire [31:0] _7 = mux_bytes(be,wd,r7);

always @* begin
  case(sel)
    3'b000: y = { r7,r6,r5,r4,r3,r2,r1,_0 };
    3'b001: y = { r7,r6,r5,r4,r3,r2,_1,r0 };
    3'b010: y = { r7,r6,r5,r4,r3,_2,r1,r0 }; 
    3'b011: y = { r7,r6,r5,r4,_3,r2,r1,r0 };
    3'b100: y = { r7,r6,r5,_4,r3,r2,r1,r0 };
    3'b101: y = { r7,r6,_5,r4,r3,r2,r1,r0 };
    3'b110: y = { r7,_6,r5,r4,r3,r2,r1,r0 };
    3'b111: y = { _7,r6,r5,r4,r3,r2,r1,r0 };
  endcase
end

endmodule

