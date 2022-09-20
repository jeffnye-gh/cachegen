// ---------------------------------------------------------------------------
// More or less generic single port ram model, similar to compiled rams
// found in std cell libs. There are no byte enables.
//
// 1 cycle load/use
// ---------------------------------------------------------------------------
module dsram #(
  parameter ADDR_WIDTH = 13
)
(
  output reg  [255:0] rd,

  input  wire [ADDR_WIDTH-1:0] a,
  input  wire [3:0]    be,
  input  wire [255:0]  wd,
  input  wire          fill,
  input  wire          write,
  input  wire          read,
  input  wire          clk
);

//initial begin
//  ram[0] = {32'h00000000, 32'h01111111, 32'h02222222, 32'h03333333,
//            32'h04444444, 32'h05555555, 32'h06666666, 32'h07777777 };
//
//  ram[1] = {32'h10000000, 32'h11111111, 32'h12222222, 32'h13333333,
//            32'h14444444, 32'h15555555, 32'h16666666, 32'h17777777 };
//
//  ram[2] = {32'h20000000, 32'h21111111, 32'h22222222, 32'h23333333,
//            32'h24444444, 32'h25555555, 32'h26666666, 32'h27777777 };
//
//  ram[3] = {32'h30000000, 32'h31111111, 32'h32222222, 32'h33333333,
//            32'h34444444, 32'h35555555, 32'h36666666, 32'h37777777 };
//end

localparam ENTRIES = 2 ** ADDR_WIDTH;

reg [255:0] ram[0:ENTRIES-1];

reg [255:0] rd_d;
always @(posedge clk) rd     <= read  ? rd_d : {256{1'bx}};
always @(a)           rd_d    = ram[a];
// -----------------------------------------------------------------------
always @(posedge clk) begin
  ram[a] <= write ? wd   : ram[a];
end
// -----------------------------------------------------------------------
// debug Probes
// ------------------------------------------------------------------------
wire [255:0] ram0 = ram[0];
wire [255:0] ram1 = ram[1];
wire [255:0] ram2 = ram[2];
wire [255:0] ram3 = ram[3];
wire [255:0] ram4 = ram[4];
wire [255:0] ram5 = ram[5];
wire [255:0] ram6 = ram[6];
wire [255:0] ram7 = ram[7];
endmodule
