`include "sim_cntrl.h"
// -----------------------------------------------------------------------
// Bit regfile
//
// FIXME: CHECK IF I NEED THE 2nd rd port
// 8192 x 12b, 1WR, 2RD port (2nd is internal only)
//
// 13b address
// 12b data
//
// -----------------------------------------------------------------------
module bitrf
(
  output reg  [3:0]  q,

  input  wire [12:0] index,
  input  wire [3:0]  way,

  input  wire        wr,
  input  wire        d,
  input  wire        reset,
  input  wire        clk
);
// ----------------------------------------------------------------------
localparam int ENTRIES = 8192;
// ----------------------------------------------------------------------
task clearregs;
integer i;
begin for(i=0;i<ENTRIES;++i) regs[i] = 4'b0; end
endtask
// ---------------------------------------------------------------------------
reg  [3:0] regs[0:8192];
reg  [3:0] wr_data;
// ---------------------------------------------------------------------------
assign q = regs[index];
// ---------------------------------------------------------------------------
always @* begin
  casez(way)
    4'b???1: wr_data = { q[3], q[2], q[1], d    };
    4'b??1?: wr_data = { q[3], q[2], d,    q[0] };
    4'b?1??: wr_data = { q[3], d,    q[1], q[0] };
    4'b1???: wr_data = { d,    q[2], q[1], q[0] };
    default: wr_data = 4'bx;
  endcase
end
// ---------------------------------------------------------------------------
always @(posedge clk) begin
  if(reset) clearregs;
end
// ---------------------------------------------------------------------------
always @(posedge clk) begin
  regs[index] <= wr ? wr_data : regs[index];
end
endmodule
