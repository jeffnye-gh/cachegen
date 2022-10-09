// ---------------------------------------------------------------------------
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

//reg [DATA_WIDTH-1:0] rd_d;
//always @(posedge clk) rd     <= read  ? rd_d : {DATA_WIDTH{1'bx}};

always @(posedge clk) ram[a] <= write ? wd   : ram[a];
always @(a) rd = ram[a];
endmodule
