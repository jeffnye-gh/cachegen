// ---------------------------------------------------------------------------
// More or less generic single port ram model, similar to compiled rams
// found in std cell libs. There are no byte enables.
//
// 1 cycle load/use
// ---------------------------------------------------------------------------
module sram #(
  parameter DATA_WIDTH = 14,
  parameter ADDR_WIDTH = 13
)
(
  output reg  [DATA_WIDTH-1:0] rd,

  input  wire [ADDR_WIDTH-1:0] a,
  input  wire [DATA_WIDTH-1:0] wd,
  input  wire          write,
  input  wire          read,
  input  wire          clk
);

localparam ENTRIES = 2 ** ADDR_WIDTH;

reg [DATA_WIDTH-1:0] ram[0:ENTRIES-1];

reg [DATA_WIDTH-1:0] rd_d;
always @(posedge clk) rd     <= read  ? rd_d : {DATA_WIDTH{1'bx}};
always @(posedge clk) ram[a] <= write ? wd   : ram[a];
always @(a)           rd_d    = ram[a];

// -----------------------------------------------------------------------
// debug Probes
// ------------------------------------------------------------------------
wire [DATA_WIDTH-1] ram0 = ram[0];
wire [DATA_WIDTH-1] ram1 = ram[1];
wire [DATA_WIDTH-1] ram2 = ram[2];
wire [DATA_WIDTH-1] ram3 = ram[3];
wire [DATA_WIDTH-1] ram4 = ram[4];
wire [DATA_WIDTH-1] ram5 = ram[5];
wire [DATA_WIDTH-1] ram6 = ram[6];
wire [DATA_WIDTH-1] ram7 = ram[7];
endmodule
