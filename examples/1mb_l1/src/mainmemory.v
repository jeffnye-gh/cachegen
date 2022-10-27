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
  parameter READ_LAT = 2,   //FIXME: this is placeholder
  parameter WRITE_TPUT = 2  //FIXME: this is placeholder
)
(
  output wire [255:0] rd,
  output reg          valid,
  output reg          ready,

  input  wire [26:0]  a,
  input  wire [31:0]  be,
  input  wire [255:0] wd,
  input  wire         write,
  input  wire         read,
  input  wire         clk
);

reg [255:0] ram[0:64000-1];
reg [255:0] rd_q,rd_q2;
reg [26:0] aq;
reg read_q,read_q2;
reg write_q,write_q2;

assign rd = read_q2 ? rd_q2 : {256{1'bx}};
assign valid = read_q2;
assign ready = write_q2;
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
always @(posedge clk) begin
  if(a > ENTRIES & (read | write)) begin
    $display("-E: access exceeds main memory size %08x",a);
  end

  aq   <= a;
  rd_q <= ram[a];
  rd_q2<= rd_q;

  write_q  <= write & !write_q;
  write_q2 <= write_q;

  read_q   <= read & !read_q;
  read_q2  <= read_q;
  ram[aq]  <= write_q ? wd : ram[aq];
end
// ------------------------------------------------------------------------
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
