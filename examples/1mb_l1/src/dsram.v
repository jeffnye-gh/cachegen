// ---------------------------------------------------------------------------
// Data array model, one for each way
//
// 1 cycle load/use
// ---------------------------------------------------------------------------
module dsram #(
  parameter ADDR_WIDTH = 13
)
(
  output wire [255:0] rd,

  input  wire [ADDR_WIDTH-1:0] a,
  input  wire [31:0]  be,
  input  wire [255:0] wd,
  input  wire         write,
  input  wire         read,
  input  wire         clk
);

localparam ENTRIES = 2 ** ADDR_WIDTH;

reg  [255:0] ram[0:ENTRIES-1];
reg  [255:0] rd_tmp;
reg  read_q;

wire  [255:0] xrd;
assign rd = read_q ? rd_tmp : {256{1'bx}};

// -----------------------------------------------------------------------
// FIXME: figure out how to write a generate statement icarus verilog likes
//        it does not like my multidimension syntax
// -----------------------------------------------------------------------
//genvar i;
//generate 
//  for(i=0;i<8;i=i+1) begin : ramwrite
//    always @(posedge clk) begin
//      ram[a][(i*8)-1: i*0] <= write & be[i] ? wd : ram[a][(i*8)-1: i*0];
//    end
//  end
//endgenerate

//For now a little bit of python , see hacks.py
always @(posedge clk) begin
  read_q <= read;
  rd_tmp <= ram[a];

  ram[a][  7:  0] <= `FF write & be[0]  ? wd[  7:  0] : ram[a][  7:  0];
  ram[a][ 15:  8] <= `FF write & be[1]  ? wd[ 15:  8] : ram[a][ 15:  8];
  ram[a][ 23: 16] <= `FF write & be[2]  ? wd[ 23: 16] : ram[a][ 23: 16];
  ram[a][ 31: 24] <= `FF write & be[3]  ? wd[ 31: 24] : ram[a][ 31: 24];
  ram[a][ 39: 32] <= `FF write & be[4]  ? wd[ 39: 32] : ram[a][ 39: 32];
  ram[a][ 47: 40] <= `FF write & be[5]  ? wd[ 47: 40] : ram[a][ 47: 40];
  ram[a][ 55: 48] <= `FF write & be[6]  ? wd[ 55: 48] : ram[a][ 55: 48];
  ram[a][ 63: 56] <= `FF write & be[7]  ? wd[ 63: 56] : ram[a][ 63: 56];
  ram[a][ 71: 64] <= `FF write & be[8]  ? wd[ 71: 64] : ram[a][ 71: 64];
  ram[a][ 79: 72] <= `FF write & be[9]  ? wd[ 79: 72] : ram[a][ 79: 72];
  ram[a][ 87: 80] <= `FF write & be[10] ? wd[ 87: 80] : ram[a][ 87: 80];
  ram[a][ 95: 88] <= `FF write & be[11] ? wd[ 95: 88] : ram[a][ 95: 88];
  ram[a][103: 96] <= `FF write & be[12] ? wd[103: 96] : ram[a][103: 96];
  ram[a][111:104] <= `FF write & be[13] ? wd[111:104] : ram[a][111:104];
  ram[a][119:112] <= `FF write & be[14] ? wd[119:112] : ram[a][119:112];
  ram[a][127:120] <= `FF write & be[15] ? wd[127:120] : ram[a][127:120];
  ram[a][135:128] <= `FF write & be[16] ? wd[135:128] : ram[a][135:128];
  ram[a][143:136] <= `FF write & be[17] ? wd[143:136] : ram[a][143:136];
  ram[a][151:144] <= `FF write & be[18] ? wd[151:144] : ram[a][151:144];
  ram[a][159:152] <= `FF write & be[19] ? wd[159:152] : ram[a][159:152];
  ram[a][167:160] <= `FF write & be[20] ? wd[167:160] : ram[a][167:160];
  ram[a][175:168] <= `FF write & be[21] ? wd[175:168] : ram[a][175:168];
  ram[a][183:176] <= `FF write & be[22] ? wd[183:176] : ram[a][183:176];
  ram[a][191:184] <= `FF write & be[23] ? wd[191:184] : ram[a][191:184];
  ram[a][199:192] <= `FF write & be[24] ? wd[199:192] : ram[a][199:192];
  ram[a][207:200] <= `FF write & be[25] ? wd[207:200] : ram[a][207:200];
  ram[a][215:208] <= `FF write & be[26] ? wd[215:208] : ram[a][215:208];
  ram[a][223:216] <= `FF write & be[27] ? wd[223:216] : ram[a][223:216];
  ram[a][231:224] <= `FF write & be[28] ? wd[231:224] : ram[a][231:224];
  ram[a][239:232] <= `FF write & be[29] ? wd[239:232] : ram[a][239:232];
  ram[a][247:240] <= `FF write & be[30] ? wd[247:240] : ram[a][247:240];
  ram[a][255:248] <= `FF write & be[31] ? wd[255:248] : ram[a][255:248];
end
// -----------------------------------------------------------------------
// debug Probes
// ------------------------------------------------------------------------
wire [255:0] p_ram0 = ram[0];
wire [31:0]  p_ram0_w0 = ram[0][ 31:  0];
wire [31:0]  p_ram0_w1 = ram[0][ 63: 32];
wire [31:0]  p_ram0_w2 = ram[0][ 95: 64];
wire [31:0]  p_ram0_w3 = ram[0][127: 96];
wire [31:0]  p_ram0_w4 = ram[0][159:128];
wire [31:0]  p_ram0_w5 = ram[0][191:160];
wire [31:0]  p_ram0_w6 = ram[0][223:192];
wire [31:0]  p_ram0_w7 = ram[0][255:224];

wire [255:0] p_ram1 = ram[1];
wire [255:0] p_ram2 = ram[2];
wire [255:0] p_ram3 = ram[3];
wire [255:0] p_ram4 = ram[4];
wire [255:0] p_ram5 = ram[5];
wire [255:0] p_ram6 = ram[6];
wire [255:0] p_ram7 = ram[7];
endmodule
