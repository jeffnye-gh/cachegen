module _probes;
//synthesis translate_off
`include "bitcmds.h"

reg [(4*8)-1:0] p_str_way;
always @(top.dut0.lrurf0.way_hit) begin
  case(top.dut0.way_hit)
    4'b0001: p_str_way = "WAY0";
    4'b0010: p_str_way = "WAY1";
    4'b0100: p_str_way = "WAY2";
    4'b1000: p_str_way = "WAY3";
    default: p_str_way = "x";
  endcase
end

reg [(11*8)-1:0] p_bit_cmd;
always @(top.dut0.fsm_bit_cmd) begin
  case(top.dut0.fsm_bit_cmd)
    B_CMD_NOP:        p_bit_cmd = "NOP";
    B_CMD_VAL:        p_bit_cmd = "VAL";
    B_CMD_INVAL:      p_bit_cmd = "INVAL";
    B_CMD_MOD:        p_bit_cmd = "MOD";
    B_CMD_CLEAN:      p_bit_cmd = "CLEAN";
    B_CMD_INVAL_ALL:  p_bit_cmd = "INVAL_ALL";
    B_CMD_ALLOC:      p_bit_cmd = "ALLOC";
    B_CMD_VAL_MOD:    p_bit_cmd = "VAL_MOD";
    B_CMD_READ:       p_bit_cmd = "READ";
    B_CMD_LRU_UP:     p_bit_cmd = "LRU_UP";
    B_CMD_LRU_MOD_UP: p_bit_cmd = "LRU_MOD_UP";
    default: p_str_way = "x";
  endcase
end

//LRU bit probes, 1st 4 addresses
wire [2:0]   p_lru_0 = top.dut0.lrurf0.regs[0];
wire [2:0]   p_lru_1 = top.dut0.lrurf0.regs[1];
wire [2:0]   p_lru_2 = top.dut0.lrurf0.regs[2];
wire [2:0]   p_lru_3 = top.dut0.lrurf0.regs[3];
wire [2:0]   p_lru_4 = top.dut0.lrurf0.regs[4];
wire [2:0]   p_lru_5 = top.dut0.lrurf0.regs[5];
wire [2:0]   p_lru_6 = top.dut0.lrurf0.regs[6];
wire [2:0]   p_lru_7 = top.dut0.lrurf0.regs[7];

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

//Data array probes, 4 ways, 1st 4 indexes
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
// ---------------------------------------------------------------------
// Words of way 0, index 0
//                                    way           index  bits
// ---------------------------------------------------------------------
wire [31:0] p_w0_i0_wd00 = top.dut0.data[0].dsram.ram[0][ 31:  0];
wire [31:0] p_w0_i0_wd01 = top.dut0.data[0].dsram.ram[0][ 63: 32];
wire [31:0] p_w0_i0_wd02 = top.dut0.data[0].dsram.ram[0][ 95: 64];
wire [31:0] p_w0_i0_wd03 = top.dut0.data[0].dsram.ram[0][127: 96];
wire [31:0] p_w0_i0_wd04 = top.dut0.data[0].dsram.ram[0][159:128];
wire [31:0] p_w0_i0_wd05 = top.dut0.data[0].dsram.ram[0][191:160];
wire [31:0] p_w0_i0_wd06 = top.dut0.data[0].dsram.ram[0][223:192];
wire [31:0] p_w0_i0_wd07 = top.dut0.data[0].dsram.ram[0][255:224];

//wire [3:0] p_d0_ram_0_be00 = top.dut0.data[0].dsram.be[ 3: 0];
//wire [3:0] p_d0_ram_0_be01 = top.dut0.data[0].dsram.be[ 7: 4];
//wire [3:0] p_d0_ram_0_be02 = top.dut0.data[0].dsram.be[11: 8];
//wire [3:0] p_d0_ram_0_be03 = top.dut0.data[0].dsram.be[15:12];
//wire [3:0] p_d0_ram_0_be04 = top.dut0.data[0].dsram.be[19:16];
//wire [3:0] p_d0_ram_0_be05 = top.dut0.data[0].dsram.be[23:20];
//wire [3:0] p_d0_ram_0_be06 = top.dut0.data[0].dsram.be[27:24];
//wire [3:0] p_d0_ram_0_be07 = top.dut0.data[0].dsram.be[31:28];
//
// ---------------------------------------------------------------------
// Words of way 0, index 1
//                                    way           index  bits
// ---------------------------------------------------------------------
wire [31:0] p_w0_i1_wd00 = top.dut0.data[0].dsram.ram[1][ 31:  0];
wire [31:0] p_w0_i1_wd01 = top.dut0.data[0].dsram.ram[1][ 63: 32];
wire [31:0] p_w0_i1_wd02 = top.dut0.data[0].dsram.ram[1][ 95: 64];
wire [31:0] p_w0_i1_wd03 = top.dut0.data[0].dsram.ram[1][127: 96];
wire [31:0] p_w0_i1_wd04 = top.dut0.data[0].dsram.ram[1][159:128];
wire [31:0] p_w0_i1_wd05 = top.dut0.data[0].dsram.ram[1][191:160];
wire [31:0] p_w0_i1_wd06 = top.dut0.data[0].dsram.ram[1][223:192];
wire [31:0] p_w0_i1_wd07 = top.dut0.data[0].dsram.ram[1][255:224];

//wire [3:0] p_w0_ram_0_be00 = top.dut0.data[0].dsram.be[ 3: 0];
//wire [3:0] p_w0_ram_0_be01 = top.dut0.data[0].dsram.be[ 7: 4];
//wire [3:0] p_w0_ram_0_be02 = top.dut0.data[0].dsram.be[11: 8];
//wire [3:0] p_w0_ram_0_be03 = top.dut0.data[0].dsram.be[15:12];
//wire [3:0] p_w0_ram_0_be04 = top.dut0.data[0].dsram.be[19:16];
//wire [3:0] p_w0_ram_0_be05 = top.dut0.data[0].dsram.be[23:20];
//wire [3:0] p_w0_ram_0_be06 = top.dut0.data[0].dsram.be[27:24];
//wire [3:0] p_w0_ram_0_be07 = top.dut0.data[0].dsram.be[31:28];

//First 4 locations for each main memory bank, by word
wire [31:0] p_mm_ram0_0 = top.dut0.mm0.ram0[0];
wire [31:0] p_mm_ram0_1 = top.dut0.mm0.ram0[1];
wire [31:0] p_mm_ram0_2 = top.dut0.mm0.ram0[2];
wire [31:0] p_mm_ram0_3 = top.dut0.mm0.ram0[3];


//synthesis translate_on
endmodule
