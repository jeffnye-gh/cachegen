module _probes;
//synthesis translate_off

//reg [(4*8)-1:0] str_way;
//always @(top.dut0.way_hit) begin
//  case(top.dut0.way_hit)
//    4'b0001: str_way = "WAY0";
//    4'b0010: str_way = "WAY1";
//    4'b0100: str_way = "WAY2";
//    4'b1000: str_way = "WAY3";
//    default: str_way = "x";
//  endcase
//end

//LRU bit probes, 1st 4 addresses
wire [2:0]   p_lru_0 = top.dut0.bits0.lbits[0];
wire [2:0]   p_lru_1 = top.dut0.bits0.lbits[1];
wire [2:0]   p_lru_2 = top.dut0.bits0.lbits[2];
wire [2:0]   p_lru_3 = top.dut0.bits0.lbits[3];

//Mod bit probes, 1st 4 addresses
wire [3:0]   p_mod_0 = top.dut0.bits0.mbits[0];
wire [3:0]   p_mod_1 = top.dut0.bits0.mbits[1];
wire [3:0]   p_mod_2 = top.dut0.bits0.mbits[2];
wire [3:0]   p_mod_3 = top.dut0.bits0.mbits[3];

//Val bit probes, 1st 4 addresses
wire [3:0]   p_val_0 = top.dut0.bits0.vbits[0];
wire [3:0]   p_val_1 = top.dut0.bits0.vbits[1];
wire [3:0]   p_val_2 = top.dut0.bits0.vbits[2];
wire [3:0]   p_val_3 = top.dut0.bits0.vbits[3];

//Tag array probes, 4 ways, 1st 4 addresses
wire [13:0]  p_tag0_ram_0 = top.dut0.tags[0].tag.ram[0];
wire [13:0]  p_tag0_ram_1 = top.dut0.tags[0].tag.ram[1];
wire [13:0]  p_tag0_ram_2 = top.dut0.tags[0].tag.ram[2];
wire [13:0]  p_tag0_ram_3 = top.dut0.tags[0].tag.ram[3];

wire [13:0]  p_tag1_ram_0 = top.dut0.tags[1].tag.ram[0];
wire [13:0]  p_tag1_ram_1 = top.dut0.tags[1].tag.ram[1];
wire [13:0]  p_tag1_ram_2 = top.dut0.tags[1].tag.ram[2];
wire [13:0]  p_tag1_ram_3 = top.dut0.tags[1].tag.ram[3];

wire [13:0]  p_tag2_ram_0 = top.dut0.tags[2].tag.ram[0];
wire [13:0]  p_tag2_ram_1 = top.dut0.tags[2].tag.ram[1];
wire [13:0]  p_tag2_ram_2 = top.dut0.tags[2].tag.ram[2];
wire [13:0]  p_tag2_ram_3 = top.dut0.tags[2].tag.ram[3];

wire [13:0]  p_tag3_ram_0 = top.dut0.tags[3].tag.ram[0];
wire [13:0]  p_tag3_ram_1 = top.dut0.tags[3].tag.ram[1];
wire [13:0]  p_tag3_ram_2 = top.dut0.tags[3].tag.ram[2];
wire [13:0]  p_tag3_ram_3 = top.dut0.tags[3].tag.ram[3];

//Data array probes, 4 ways, 1st 4 addresses
wire [255:0] p_d0_ram_0 = top.dut0.data[0].dsram.ram[0];
wire [255:0] p_d0_ram_1 = top.dut0.data[0].dsram.ram[1];
wire [255:0] p_d0_ram_2 = top.dut0.data[0].dsram.ram[2];
wire [255:0] p_d0_ram_3 = top.dut0.data[0].dsram.ram[3];

wire [255:0] p_d1_ram_0 = top.dut0.data[1].dsram.ram[0];
wire [255:0] p_d1_ram_1 = top.dut0.data[1].dsram.ram[1];
wire [255:0] p_d1_ram_2 = top.dut0.data[1].dsram.ram[2];
wire [255:0] p_d1_ram_3 = top.dut0.data[1].dsram.ram[3];

wire [255:0] p_d2_ram_0 = top.dut0.data[2].dsram.ram[0];
wire [255:0] p_d2_ram_1 = top.dut0.data[2].dsram.ram[1];
wire [255:0] p_d2_ram_2 = top.dut0.data[2].dsram.ram[2];
wire [255:0] p_d2_ram_3 = top.dut0.data[2].dsram.ram[3];

wire [255:0] p_d3_ram_0 = top.dut0.data[3].dsram.ram[0];
wire [255:0] p_d3_ram_1 = top.dut0.data[3].dsram.ram[1];
wire [255:0] p_d3_ram_2 = top.dut0.data[3].dsram.ram[2];
wire [255:0] p_d3_ram_3 = top.dut0.data[3].dsram.ram[3];

//First 4 locations for each main memory bank, by word
wire [31:0] p_mm_ram0_0 = top.dut0.mm0.ram0[0];
wire [31:0] p_mm_ram0_1 = top.dut0.mm0.ram0[1];
wire [31:0] p_mm_ram0_2 = top.dut0.mm0.ram0[2];
wire [31:0] p_mm_ram0_3 = top.dut0.mm0.ram0[3];


//synthesis translate_on
endmodule
