
module dut #(
  parameter L1_READ_HIT_LAT = 1,
  parameter L1_WRITE_HIT_TPUT = 1,
  parameter EXP_MM_ENTRIES = 1,
  parameter MM_READ_LAT = 1,
  parameter MM_WRITE_TPUT = 1
)
(
  output wire [31:0] cc_tb_readdata,
  output wire        cc_tb_readdata_valid,
  output wire        cc_tb_ready,

  input wire [31:0]  tb_cc_address,
  input wire [3:0]   tb_cc_byteenable,
  input wire         tb_cc_read,
  input wire         tb_cc_write,
  input wire [31:0]  tb_cc_writedata
);

wire [31:0]  cc_mm_address;
wire [255:0] cc_mm_writedata;
wire         cc_mm_write;
wire         cc_mm_read;
wire [255:0] mm_cc_readdata;
wire [255:0] xmm_cc_readdata;
wire         mm_cc_readdatavalid;


cache #(.READ_HIT_LAT(L1_READ_HIT_LAT),
        .WRITE_HIT_TPUT(L1_WRITE_HIT_TPUT)
) l1(
  //outputs
  .rd        (cc_tb_readdata),
  .rd_valid_d(cc_tb_readdata_valid),
  .ready_d   (cc_tb_ready),

  //from TB 
  .a    (tb_cc_address),
  .be   (tb_cc_byteenable),
  .read (tb_cc_read),
  .write(tb_cc_write),
  .wd   (tb_cc_writedata),

  //from cache to mainmemory
  .mm_a        (cc_mm_address),
  .mm_read_d   (cc_mm_read),
  .mm_write_d  (cc_mm_write),
  //from L1 to main, evict
  .mm_writedata(cc_mm_writedata),

  //from main to L1, fill
  .mm_readdata(mm_cc_readdata),

  .mm_readdata_valid(mm_cc_readdatavalid),
  .mm_ready(mm_cc_ready),

  .reset(reset),
  .clk(clk)
);
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
mainmemory #(.ENTRIES(EXP_MM_ENTRIES),
             .READ_LAT(MM_READ_LAT),
             .WRITE_TPUT(MM_WRITE_TPUT)
) mm0(

  .rd   (mm_cc_readdata),
  .valid(mm_cc_readdatavalid),
  .ready(mm_cc_ready),

  //from CC/TB control
  .a     (cc_mm_address[31:5]), //only line access
  .wd    (cc_mm_writedata),
  .read  (cc_mm_read),
  .write (cc_mm_write),

  .clk(clk)
);

endmodule
