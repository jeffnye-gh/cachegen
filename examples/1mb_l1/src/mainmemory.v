// ---------------------------------------------------------------------------
// Main memory model
// Write timing has 1 cycle delay
// Read  timing has 2 cycle delay
//
// This supports single cycle fills and byte writes
// FIXME: this is close enough to dsram that it could be done in one 
//        parameterized model
// ---------------------------------------------------------------------------
module mainmemory #(
  parameter ENTRIES = 256,
  parameter READ_LAT   = 2, //FIXME: this is placeholder
  parameter WRITE_TPUT = 2  //FIXME: this is placeholder
)
(
  output wire [255:0] rd,
  output reg          valid,

  input  wire [31:0]  a,
  input  wire [31:0]  be,
  input  wire [255:0] wd,
  input  wire         write,
  input  wire         read,
  input  wire         clk
);

//localparam ENTRIES = 2 ** ADDR_WIDTH;

reg [255:0] ram[0:ENTRIES-1];
reg [255:0] rd_q,rd_q2;
reg [31:0] aq;
reg read_q,read_q2;

assign rd = read_q2 ? rd_q2 : {256{1'bx}};
assign valid = read_q2;
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
  aq   <= a;
  rd_q <= ram[a];
  rd_q2<= rd_q;

  read_q  <= read & !read_q;
  read_q2 <= read_q;

  ram[aq][  7:  0] <= write & be[0]  ? wd[  7:  0] : ram[aq][  7:  0];
  ram[aq][ 15:  8] <= write & be[1]  ? wd[ 15:  8] : ram[aq][ 15:  8];
  ram[aq][ 23: 16] <= write & be[2]  ? wd[ 23: 16] : ram[aq][ 23: 16];
  ram[aq][ 31: 24] <= write & be[3]  ? wd[ 31: 24] : ram[aq][ 31: 24];
  ram[aq][ 39: 32] <= write & be[4]  ? wd[ 39: 32] : ram[aq][ 39: 32];
  ram[aq][ 47: 40] <= write & be[5]  ? wd[ 47: 40] : ram[aq][ 47: 40];
  ram[aq][ 55: 48] <= write & be[6]  ? wd[ 55: 48] : ram[aq][ 55: 48];
  ram[aq][ 63: 56] <= write & be[7]  ? wd[ 63: 56] : ram[aq][ 63: 56];
  ram[aq][ 71: 64] <= write & be[8]  ? wd[ 71: 64] : ram[aq][ 71: 64];
  ram[aq][ 79: 72] <= write & be[9]  ? wd[ 79: 72] : ram[aq][ 79: 72];
  ram[aq][ 87: 80] <= write & be[10] ? wd[ 87: 80] : ram[aq][ 87: 80];
  ram[aq][ 95: 88] <= write & be[11] ? wd[ 95: 88] : ram[aq][ 95: 88];
  ram[aq][103: 96] <= write & be[12] ? wd[103: 96] : ram[aq][103: 96];
  ram[aq][111:104] <= write & be[13] ? wd[111:104] : ram[aq][111:104];
  ram[aq][119:112] <= write & be[14] ? wd[119:112] : ram[aq][119:112];
  ram[aq][127:120] <= write & be[15] ? wd[127:120] : ram[aq][127:120];
  ram[aq][135:128] <= write & be[16] ? wd[135:128] : ram[aq][135:128];
  ram[aq][143:136] <= write & be[17] ? wd[143:136] : ram[aq][143:136];
  ram[aq][151:144] <= write & be[18] ? wd[151:144] : ram[aq][151:144];
  ram[aq][159:152] <= write & be[19] ? wd[159:152] : ram[aq][159:152];
  ram[aq][167:160] <= write & be[20] ? wd[167:160] : ram[aq][167:160];
  ram[aq][175:168] <= write & be[21] ? wd[175:168] : ram[aq][175:168];
  ram[aq][183:176] <= write & be[22] ? wd[183:176] : ram[aq][183:176];
  ram[aq][191:184] <= write & be[23] ? wd[191:184] : ram[aq][191:184];
  ram[aq][199:192] <= write & be[24] ? wd[199:192] : ram[aq][199:192];
  ram[aq][207:200] <= write & be[25] ? wd[207:200] : ram[aq][207:200];
  ram[aq][215:208] <= write & be[26] ? wd[215:208] : ram[aq][215:208];
  ram[aq][223:216] <= write & be[27] ? wd[223:216] : ram[aq][223:216];
  ram[aq][231:224] <= write & be[28] ? wd[231:224] : ram[aq][231:224];
  ram[aq][239:232] <= write & be[29] ? wd[239:232] : ram[aq][239:232];
  ram[aq][247:240] <= write & be[30] ? wd[247:240] : ram[aq][247:240];
  ram[aq][255:248] <= write & be[31] ? wd[255:248] : ram[aq][255:248];
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
