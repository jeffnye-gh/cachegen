// ---------------------------------------------------------------------------
//A cache line is 256b wide, there are 8 word width arrays
// FIXME: can't seem to get iverilog to like associative arrays with 
// typedef indexes (per the docs should be possible).
// ---------------------------------------------------------------------------
//Physically instantiating 8 banks 
//
// ram7 ram6 ram5 ram4 ram3 ram2 ram1 ram0
//
// Byte addressing for bypass
//
//  3322222222221111111111           bbbb
//  109876543210987654321098765432-- 3210
//  iiiiiiiiiiiiiiiiiAAAAAAAAAAWWW   bbbb
//
// b = byte select
// W = word select (one of the 8 word with banks)
// A = index with a bank
// i = these are trapped, anything other than 0 is reported 
//     otherwise these bits are ignored
//
// MEM_RANGE instantiates 1MB entries of 4 bytes each
//
// Timing is simply 1 cycle of load use penalty keyed off the _read signal
// ---------------------------------------------------------------------------
// Bypass can perform word or byte writes
// Bypass write data is taken from wd[31:0]
// Bypass read data is driven on rd[31:0]
//
// Non-bypass writes and reads are full cache line
//
// ---------------------------------------------------------------------------
module mainmemory #(
  parameter MEM_RANGE = 256
)
(
  output wire [255:0] rd,
  output reg          valid,

  input wire [31:0]   a,
  input wire [3:0]    be,
  input wire          write,
  input wire          read,
  input wire [255:0]  wd,

  input wire          bypass,

  input wire          reset,
  input wire          clk
);

initial begin
`ifdef INIT_MM
  integer i;
  imsg("Initializing main memory");
  for(i=0;i<MEM_RANGE;++i) begin
    ram0[i] = {4'h0,i[27:0]};
    ram1[i] = {4'h1,i[27:0]};
    ram2[i] = {4'h2,i[27:0]};
    ram3[i] = {4'h3,i[27:0]};
    ram4[i] = {4'h4,i[27:0]};
    ram5[i] = {4'h5,i[27:0]};
    ram6[i] = {4'h6,i[27:0]};
    ram7[i] = {4'h7,i[27:0]};
  end
`endif
end 

reg [31:0] ram0[0:MEM_RANGE];
reg [31:0] ram1[0:MEM_RANGE];
reg [31:0] ram2[0:MEM_RANGE];
reg [31:0] ram3[0:MEM_RANGE];
reg [31:0] ram4[0:MEM_RANGE];
reg [31:0] ram5[0:MEM_RANGE];
reg [31:0] ram6[0:MEM_RANGE];
reg [31:0] ram7[0:MEM_RANGE];

wire [9:0] entry = a[14:5];
wire [2:0] bank  = a[4:2];

wire [31:0] byp_writedata = wd[31:0];
wire        byp_read      = bypass   & rd;
wire        valid_d       = byp_read | rd;

wire [3:0]  byp_be_guard = {4{write}} & be;

wire [3:0]  ram_be = {4{write}};
wire [3:0]  byp_be = bypass ? byp_be_guard : ram_be;

// -----------------------------------------------------------------------
// Create the bank selects
// -----------------------------------------------------------------------
wire [7:0] bnksel;

assign bnksel[0] = bank == 3'h0;
assign bnksel[1] = bank == 3'h1;
assign bnksel[2] = bank == 3'h2;
assign bnksel[3] = bank == 3'h3;
assign bnksel[4] = bank == 3'h4;
assign bnksel[5] = bank == 3'h5;
assign bnksel[6] = bank == 3'h6;
assign bnksel[7] = bank == 3'h7;

// -----------------------------------------------------------------------
// Create the byte writes for each word in each bank
// -----------------------------------------------------------------------
wire [3:0] bnk0_be,bnk1_be,bnk2_be,bnk3_be,
           bnk4_be,bnk5_be,bnk6_be,bnk7_be;
bnk_be u0(.y(bnk0_be),.be(byp_be),.sel(bnksel[0]));
bnk_be u1(.y(bnk1_be),.be(byp_be),.sel(bnksel[1]));
bnk_be u2(.y(bnk2_be),.be(byp_be),.sel(bnksel[2]));
bnk_be u3(.y(bnk3_be),.be(byp_be),.sel(bnksel[3]));
bnk_be u4(.y(bnk4_be),.be(byp_be),.sel(bnksel[4]));
bnk_be u5(.y(bnk5_be),.be(byp_be),.sel(bnksel[5]));
bnk_be u6(.y(bnk6_be),.be(byp_be),.sel(bnksel[6]));
bnk_be u7(.y(bnk7_be),.be(byp_be),.sel(bnksel[7]));
// -----------------------------------------------------------------------
// Create the write data for each bank
// -----------------------------------------------------------------------
wire[31:0] bnk0_wd = bypass ? byp_writedata : wd[ 31:  0];
wire[31:0] bnk1_wd = bypass ? byp_writedata : wd[ 63: 32];
wire[31:0] bnk2_wd = bypass ? byp_writedata : wd[ 95: 64];
wire[31:0] bnk3_wd = bypass ? byp_writedata : wd[127: 96];
wire[31:0] bnk4_wd = bypass ? byp_writedata : wd[159:128];
wire[31:0] bnk5_wd = bypass ? byp_writedata : wd[191:160];
wire[31:0] bnk6_wd = bypass ? byp_writedata : wd[223:192];
wire[31:0] bnk7_wd = bypass ? byp_writedata : wd[225:224];
// -----------------------------------------------------------------------
always @(posedge clk) begin
  ram0[entry][ 7: 0] <= bnk0_be[0] ? bnk0_wd[ 7: 0] : ram0[entry][ 7: 0];
  ram0[entry][15: 8] <= bnk0_be[1] ? bnk0_wd[15: 8] : ram0[entry][15: 8];
  ram0[entry][23:16] <= bnk0_be[2] ? bnk0_wd[23:16] : ram0[entry][23:16];
  ram0[entry][31:24] <= bnk0_be[3] ? bnk0_wd[31:24] : ram0[entry][31:24];
                                                                 
  ram1[entry][ 7: 0] <= bnk0_be[0] ? bnk0_wd[ 7: 0] : ram1[entry][ 7: 0];
  ram1[entry][15: 8] <= bnk0_be[1] ? bnk0_wd[15: 8] : ram1[entry][15: 8];
  ram1[entry][23:16] <= bnk0_be[2] ? bnk0_wd[23:16] : ram1[entry][23:16];
  ram1[entry][31:24] <= bnk0_be[3] ? bnk0_wd[31:24] : ram1[entry][31:24];
                                                                 
  ram2[entry][ 7: 0] <= bnk0_be[0] ? bnk0_wd[ 7: 0] : ram2[entry][ 7: 0];
  ram2[entry][15: 8] <= bnk0_be[1] ? bnk0_wd[15: 8] : ram2[entry][15: 8];
  ram2[entry][23:16] <= bnk0_be[2] ? bnk0_wd[23:16] : ram2[entry][23:16];
  ram2[entry][31:24] <= bnk0_be[3] ? bnk0_wd[31:24] : ram2[entry][31:24];
                                                                 
  ram3[entry][ 7: 0] <= bnk0_be[0] ? bnk0_wd[ 7: 0] : ram3[entry][ 7: 0];
  ram3[entry][15: 8] <= bnk0_be[1] ? bnk0_wd[15: 8] : ram3[entry][15: 8];
  ram3[entry][23:16] <= bnk0_be[2] ? bnk0_wd[23:16] : ram3[entry][23:16];
  ram3[entry][31:24] <= bnk0_be[3] ? bnk0_wd[31:24] : ram3[entry][31:24];
                                                                 
  ram4[entry][ 7: 0] <= bnk0_be[0] ? bnk0_wd[ 7: 0] : ram4[entry][ 7: 0];
  ram4[entry][15: 8] <= bnk0_be[1] ? bnk0_wd[15: 8] : ram4[entry][15: 8];
  ram4[entry][23:16] <= bnk0_be[2] ? bnk0_wd[23:16] : ram4[entry][23:16];
  ram4[entry][31:24] <= bnk0_be[3] ? bnk0_wd[31:24] : ram4[entry][31:24];
                                                                 
  ram5[entry][ 7: 0] <= bnk0_be[0] ? bnk0_wd[ 7: 0] : ram5[entry][ 7: 0];
  ram5[entry][15: 8] <= bnk0_be[1] ? bnk0_wd[15: 8] : ram5[entry][15: 8];
  ram5[entry][23:16] <= bnk0_be[2] ? bnk0_wd[23:16] : ram5[entry][23:16];
  ram5[entry][31:24] <= bnk0_be[3] ? bnk0_wd[31:24] : ram5[entry][31:24];
                                                                 
  ram6[entry][ 7: 0] <= bnk0_be[0] ? bnk0_wd[ 7: 0] : ram6[entry][ 7: 0];
  ram6[entry][15: 8] <= bnk0_be[1] ? bnk0_wd[15: 8] : ram6[entry][15: 8];
  ram6[entry][23:16] <= bnk0_be[2] ? bnk0_wd[23:16] : ram6[entry][23:16];
  ram6[entry][31:24] <= bnk0_be[3] ? bnk0_wd[31:24] : ram6[entry][31:24];
                                                                 
  ram7[entry][ 7: 0] <= bnk0_be[0] ? bnk0_wd[ 7: 0] : ram7[entry][ 7: 0];
  ram7[entry][15: 8] <= bnk0_be[1] ? bnk0_wd[15: 8] : ram7[entry][15: 8];
  ram7[entry][23:16] <= bnk0_be[2] ? bnk0_wd[23:16] : ram7[entry][23:16];
  ram7[entry][31:24] <= bnk0_be[3] ? bnk0_wd[31:24] : ram7[entry][31:24];

end

always @(posedge clk) valid <= valid_d;

// ------------------------------------------------------------------------
// debug Probes
// ------------------------------------------------------------------------
////First 8 locations in main memory, re as cache lines
//wire [255:0] mm0 = {ram7[0],ram6[0],ram5[0],ram4[0],
//                    ram3[0],ram2[0],ram1[0],ram0[0]};
//wire [255:0] mm1 = {ram7[1],ram6[1],ram5[1],ram4[1],
//                    ram3[1],ram2[1],ram1[1],ram0[1]};
//wire [255:0] mm2 = {ram7[2],ram6[2],ram5[2],ram4[2],
//                    ram3[2],ram2[2],ram1[2],ram0[2]};
//wire [255:0] mm3 = {ram7[3],ram6[3],ram5[3],ram4[3],
//                    ram3[3],ram2[3],ram1[3],ram0[3]};
//wire [255:0] mm4 = {ram7[4],ram6[4],ram5[4],ram4[4],
//                    ram3[4],ram2[4],ram1[4],ram0[4]};
//wire [255:0] mm5 = {ram7[5],ram6[5],ram5[5],ram4[5],
//                    ram3[5],ram2[5],ram1[5],ram0[5]};
//wire [255:0] mm6 = {ram7[6],ram6[6],ram5[6],ram4[6],
//                    ram3[6],ram2[6],ram1[6],ram0[6]};
//wire [255:0] mm7 = {ram7[7],ram6[7],ram5[7],ram4[7],
//                    ram3[7],ram2[7],ram1[7],ram0[7]};
//
////First 8 locations for each bank, by word
//wire [31:0] mm_ram0_0 = ram0[0];
//wire [31:0] mm_ram0_1 = ram0[1];
//wire [31:0] mm_ram0_2 = ram0[2];
//wire [31:0] mm_ram0_3 = ram0[3];
//wire [31:0] mm_ram0_4 = ram0[4];
//wire [31:0] mm_ram0_5 = ram0[5];
//wire [31:0] mm_ram0_6 = ram0[6];
//wire [31:0] mm_ram0_7 = ram0[7];
//
//wire [31:0] mm_ram1_0 = ram1[0];
//wire [31:0] mm_ram1_1 = ram1[1];
//wire [31:0] mm_ram1_2 = ram1[2];
//wire [31:0] mm_ram1_3 = ram1[3];
//wire [31:0] mm_ram1_4 = ram1[4];
//wire [31:0] mm_ram1_5 = ram1[5];
//wire [31:0] mm_ram1_6 = ram1[6];
//wire [31:0] mm_ram1_7 = ram1[7];
//
//wire [31:0] mm_ram2_0 = ram2[0];
//wire [31:0] mm_ram2_1 = ram2[1];
//wire [31:0] mm_ram2_2 = ram2[2];
//wire [31:0] mm_ram2_3 = ram2[3];
//wire [31:0] mm_ram2_4 = ram2[4];
//wire [31:0] mm_ram2_5 = ram2[5];
//wire [31:0] mm_ram2_6 = ram2[6];
//wire [31:0] mm_ram2_7 = ram2[7];
//
//wire [31:0] mm_ram3_0 = ram3[0];
//wire [31:0] mm_ram3_1 = ram3[1];
//wire [31:0] mm_ram3_2 = ram3[2];
//wire [31:0] mm_ram3_3 = ram3[3];
//wire [31:0] mm_ram3_4 = ram3[4];
//wire [31:0] mm_ram3_5 = ram3[5];
//wire [31:0] mm_ram3_6 = ram3[6];
//wire [31:0] mm_ram3_7 = ram3[7];
//
//wire [31:0] mm_ram4_0 = ram4[0];
//wire [31:0] mm_ram4_1 = ram4[1];
//wire [31:0] mm_ram4_2 = ram4[2];
//wire [31:0] mm_ram4_3 = ram4[3];
//wire [31:0] mm_ram4_4 = ram4[4];
//wire [31:0] mm_ram4_5 = ram4[5];
//wire [31:0] mm_ram4_6 = ram4[6];
//wire [31:0] mm_ram4_7 = ram4[7];
//
//wire [31:0] mm_ram5_0 = ram5[0];
//wire [31:0] mm_ram5_1 = ram5[1];
//wire [31:0] mm_ram5_2 = ram5[2];
//wire [31:0] mm_ram5_3 = ram5[3];
//wire [31:0] mm_ram5_4 = ram5[4];
//wire [31:0] mm_ram5_5 = ram5[5];
//wire [31:0] mm_ram5_6 = ram5[6];
//wire [31:0] mm_ram5_7 = ram5[7];
//
//wire [31:0] mm_ram6_0 = ram6[0];
//wire [31:0] mm_ram6_1 = ram6[1];
//wire [31:0] mm_ram6_2 = ram6[2];
//wire [31:0] mm_ram6_3 = ram6[3];
//wire [31:0] mm_ram6_4 = ram6[4];
//wire [31:0] mm_ram6_5 = ram6[5];
//wire [31:0] mm_ram6_6 = ram6[6];
//wire [31:0] mm_ram6_7 = ram6[7];
//
//wire [31:0] mm_ram7_0 = ram7[0];
//wire [31:0] mm_ram7_1 = ram7[1];
//wire [31:0] mm_ram7_2 = ram7[2];
//wire [31:0] mm_ram7_3 = ram7[3];
//wire [31:0] mm_ram7_4 = ram7[4];
//wire [31:0] mm_ram7_5 = ram7[5];
//wire [31:0] mm_ram7_6 = ram7[6];
//wire [31:0] mm_ram7_7 = ram7[7];

endmodule
