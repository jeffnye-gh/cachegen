`include "sim_cntrl.h"
// -----------------------------------------------------------------------
// MOD bit regfile
//
// FIXME: CHECK IF I NEED THE 2nd rd port
// 8192 x 12b, 1WR, 2RD port (2nd is internal only)
//
// 13b address
// 12b data
//
// -----------------------------------------------------------------------
module dirty
(
  output reg  [3:0]  rd,

  input  wire [12:0] wa,
  input  wire [3:0]  way_hit,

  input  wire [12:0] ra,
  input  wire        wr,
  input  wire        in,
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
reg  [3:0] wd;
// ---------------------------------------------------------------------------
assign rd = regs[ra];
// ---------------------------------------------------------------------------
always @* begin
  casez(way_hit)
    4'b???1: wd = { rd[3], rd[2], rd[1], in    };
    4'b??1?: wd = { rd[3], rd[2], in,    rd[0] };
    4'b?1??: wd = { rd[3], in,    rd[1], rd[0] };
    4'b1???: wd = { in,    rd[2], rd[1], rd[0] };
    default: wd = 4'bx;
  endcase
end
// ---------------------------------------------------------------------------
always @(posedge clk) begin
  if(reset) clearregs;
end
// ---------------------------------------------------------------------------
always @(posedge clk) begin
  regs[wa] <= wr ? wd : regs[wa];
end
endmodule
