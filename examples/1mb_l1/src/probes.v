module _probes;
//synthesis translate_off
`include "bitcmds.h"
`include "fsm_state.h"

//wire [255:0] p_d0_ram_0 = top.dut0.dsram0.ram[0];
wire [255:0] dary_index0_way0 = top.dut0.dsram0.ram[0];
wire [255:0] dary_index0_way1 = top.dut0.dsram1.ram[0];
wire [255:0] dary_index0_way2 = top.dut0.dsram2.ram[0];
wire [255:0] dary_index0_way3 = top.dut0.dsram3.ram[0];
//wire [255:0] dary_index0_way2 = 
//wire [255:0] dary_index0_way3 = 
// ---------------------------------------------------------------------
// Icarus verilog does not dump strings to the vcd
// ---------------------------------------------------------------------
reg [10*8-1:0] ps_fsm_st;
//string ps_fsm_st;
always @(top.dut0.fsm0.state) begin
  case(top.dut0.fsm0.state)
    IDLE:      ps_fsm_st = "IDLE";
    WR_ALLOC:  ps_fsm_st = "WR_ALLOC";
    RD_ALLOC:  ps_fsm_st = "RD_ALLOC";
    ALLOC_FILL:ps_fsm_st = "FILL";
    WR_EVICT:  ps_fsm_st = "WR_EVICT";
    RD_EVICT:  ps_fsm_st = "RD_EVICT";
    READ:      ps_fsm_st = "READ";
    INVAL_ALL: ps_fsm_st = "INVAL_ALL";
  endcase
end
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
string ps_way_sel;
always @(top.dut0.way_sel_d) begin
  case(top.dut0.way_sel_d)
    4'b0001: ps_way_sel = "WAY0";
    4'b0010: ps_way_sel = "WAY1";
    4'b0100: ps_way_sel = "WAY2";
    4'b1000: ps_way_sel = "WAY3";
    default: ps_way_sel = "x";
  endcase
end
// ---------------------------------------------------------------------
//LRU bit probes, 1st 8 indexes
// ---------------------------------------------------------------------
wire [2:0]   p_lru_0 = top.dut0.lrurf0.regs[0];
wire [2:0]   p_lru_1 = top.dut0.lrurf0.regs[1];
wire [2:0]   p_lru_2 = top.dut0.lrurf0.regs[2];
wire [2:0]   p_lru_3 = top.dut0.lrurf0.regs[3];
wire [2:0]   p_lru_4 = top.dut0.lrurf0.regs[4];
wire [2:0]   p_lru_5 = top.dut0.lrurf0.regs[5];
wire [2:0]   p_lru_6 = top.dut0.lrurf0.regs[6];
wire [2:0]   p_lru_7 = top.dut0.lrurf0.regs[7];
// ---------------------------------------------------------------------
//Mod bit probes, 1st 8 indexes
// ---------------------------------------------------------------------
wire [3:0]   p_mod_0 = top.dut0.dirty0.regs[0];
wire [3:0]   p_mod_1 = top.dut0.dirty0.regs[1];
wire [3:0]   p_mod_2 = top.dut0.dirty0.regs[2];
wire [3:0]   p_mod_3 = top.dut0.dirty0.regs[3];
wire [3:0]   p_mod_4 = top.dut0.dirty0.regs[4];
wire [3:0]   p_mod_5 = top.dut0.dirty0.regs[5];
wire [3:0]   p_mod_6 = top.dut0.dirty0.regs[6];
wire [3:0]   p_mod_7 = top.dut0.dirty0.regs[7];

// ---------------------------------------------------------------------
//Val bit probes, 1st 4 addresses
// ---------------------------------------------------------------------
wire [3:0]   p_val_0 = top.dut0.valid0.regs[0];
wire [3:0]   p_val_1 = top.dut0.valid0.regs[1];
wire [3:0]   p_val_2 = top.dut0.valid0.regs[2];
wire [3:0]   p_val_3 = top.dut0.valid0.regs[3];

// ---------------------------------------------------------------------
//Tag array probes, 4 ways, 1st 4 addresses
// ---------------------------------------------------------------------
wire [13:0]  p_tag_out_d0 = top.dut0.tag_out_d[0];
wire [13:0]  p_tag_out_d1 = top.dut0.tag_out_d[1];
wire [13:0]  p_tag_out_d2 = top.dut0.tag_out_d[2];
wire [13:0]  p_tag_out_d3 = top.dut0.tag_out_d[3];

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

wire         p_tag0_wr = top.dut0.tags[0].tag.write;
wire         p_tag1_wr = top.dut0.tags[1].tag.write;
wire         p_tag2_wr = top.dut0.tags[2].tag.write;
wire         p_tag3_wr = top.dut0.tags[3].tag.write;
// ---------------------------------------------------------------------
// Darray probes, 4 ways, 1st 4 indexes
// ---------------------------------------------------------------------
wire [255:0] p_d0_ram_0 = top.dut0.dsram0.ram[0];
wire [255:0] p_d0_ram_1 = top.dut0.dsram0.ram[1];
wire [255:0] p_d0_ram_2 = top.dut0.dsram0.ram[2];
wire [255:0] p_d0_ram_3 = top.dut0.dsram0.ram[3];
                                         
wire [255:0] p_d1_ram_0 = top.dut0.dsram1.ram[0];
wire [255:0] p_d1_ram_1 = top.dut0.dsram1.ram[1];
wire [255:0] p_d1_ram_2 = top.dut0.dsram1.ram[2];
wire [255:0] p_d1_ram_3 = top.dut0.dsram1.ram[3];
                                         
wire [255:0] p_d2_ram_0 = top.dut0.dsram2.ram[0];
wire [255:0] p_d2_ram_1 = top.dut0.dsram2.ram[1];
wire [255:0] p_d2_ram_2 = top.dut0.dsram2.ram[2];
wire [255:0] p_d2_ram_3 = top.dut0.dsram2.ram[3];
                                         
wire [255:0] p_d3_ram_0 = top.dut0.dsram3.ram[0];
wire [255:0] p_d3_ram_1 = top.dut0.dsram3.ram[1];
wire [255:0] p_d3_ram_2 = top.dut0.dsram3.ram[2];
wire [255:0] p_d3_ram_3 = top.dut0.dsram3.ram[3];

wire [31:0] p_line_wd_word0 = top.dut0.line_wd[ 31:  0];
wire [31:0] p_line_wd_word1 = top.dut0.line_wd[ 63: 32];
wire [31:0] p_line_wd_word2 = top.dut0.line_wd[ 95: 64];
wire [31:0] p_line_wd_word3 = top.dut0.line_wd[127: 96];
wire [31:0] p_line_wd_word4 = top.dut0.line_wd[159:128];
wire [31:0] p_line_wd_word5 = top.dut0.line_wd[191:160];
wire [31:0] p_line_wd_word6 = top.dut0.line_wd[223:192];
wire [31:0] p_line_wd_word7 = top.dut0.line_wd[255:224];

// ---------------------------------------------------------------------
// Darray Words of way 0, index 0
// ---------------------------------------------------------------------
wire [31:0] p_w0_i0_wd00 = top.dut0.dsram0.ram[0][ 31:  0];
wire [31:0] p_w0_i0_wd01 = top.dut0.dsram0.ram[0][ 63: 32];
wire [31:0] p_w0_i0_wd02 = top.dut0.dsram0.ram[0][ 95: 64];
wire [31:0] p_w0_i0_wd03 = top.dut0.dsram0.ram[0][127: 96];
wire [31:0] p_w0_i0_wd04 = top.dut0.dsram0.ram[0][159:128];
wire [31:0] p_w0_i0_wd05 = top.dut0.dsram0.ram[0][191:160];
wire [31:0] p_w0_i0_wd06 = top.dut0.dsram0.ram[0][223:192];
wire [31:0] p_w0_i0_wd07 = top.dut0.dsram0.ram[0][255:224];
// ---------------------------------------------------------------------
// Darray Words of way 0, index 1
// ---------------------------------------------------------------------
wire [31:0] p_w0_i1_wd00 = top.dut0.dsram0.ram[1][ 31:  0];
wire [31:0] p_w0_i1_wd01 = top.dut0.dsram0.ram[1][ 63: 32];
wire [31:0] p_w0_i1_wd02 = top.dut0.dsram0.ram[1][ 95: 64];
wire [31:0] p_w0_i1_wd03 = top.dut0.dsram0.ram[1][127: 96];
wire [31:0] p_w0_i1_wd04 = top.dut0.dsram0.ram[1][159:128];
wire [31:0] p_w0_i1_wd05 = top.dut0.dsram0.ram[1][191:160];
wire [31:0] p_w0_i1_wd06 = top.dut0.dsram0.ram[1][223:192];
wire [31:0] p_w0_i1_wd07 = top.dut0.dsram0.ram[1][255:224];
// ---------------------------------------------------------------------
// Main memory First 4 locations for each main memory bank, by word
// ---------------------------------------------------------------------
wire [31:0] p_mm_ram0_0 = top.dut0.mm0.ram0[0];
wire [31:0] p_mm_ram0_1 = top.dut0.mm0.ram0[1];
wire [31:0] p_mm_ram0_2 = top.dut0.mm0.ram0[2];
wire [31:0] p_mm_ram0_3 = top.dut0.mm0.ram0[3];

wire [31:0] p_mm_rd_word0 = top.dut0.mm0.rd[ 31:  0];
wire [31:0] p_mm_rd_word1 = top.dut0.mm0.rd[ 63: 32];
wire [31:0] p_mm_rd_word2 = top.dut0.mm0.rd[ 95: 64];
wire [31:0] p_mm_rd_word3 = top.dut0.mm0.rd[127: 96];
wire [31:0] p_mm_rd_word4 = top.dut0.mm0.rd[159:128];
wire [31:0] p_mm_rd_word5 = top.dut0.mm0.rd[191:160];
wire [31:0] p_mm_rd_word6 = top.dut0.mm0.rd[223:192];
wire [31:0] p_mm_rd_word7 = top.dut0.mm0.rd[255:224];

// ---------------------------------------------------------------------
// Capture data and address
// ---------------------------------------------------------------------
wire [31:0] p_cap_a_00 = top.mm_actual_capture_addr[ 0];
wire [31:0] p_cap_a_01 = top.mm_actual_capture_addr[ 1];
wire [31:0] p_cap_a_02 = top.mm_actual_capture_addr[ 2];
wire [31:0] p_cap_a_03 = top.mm_actual_capture_addr[ 3];
wire [31:0] p_cap_a_04 = top.mm_actual_capture_addr[ 4];
wire [31:0] p_cap_a_05 = top.mm_actual_capture_addr[ 5];
wire [31:0] p_cap_a_06 = top.mm_actual_capture_addr[ 6];
wire [31:0] p_cap_a_07 = top.mm_actual_capture_addr[ 7];
wire [31:0] p_cap_a_08 = top.mm_actual_capture_addr[ 8];
wire [31:0] p_cap_a_09 = top.mm_actual_capture_addr[ 9];
wire [31:0] p_cap_a_10 = top.mm_actual_capture_addr[10];
wire [31:0] p_cap_a_11 = top.mm_actual_capture_addr[11];
wire [31:0] p_cap_a_12 = top.mm_actual_capture_addr[12];
wire [31:0] p_cap_a_13 = top.mm_actual_capture_addr[13];
wire [31:0] p_cap_a_14 = top.mm_actual_capture_addr[14];
wire [31:0] p_cap_a_15 = top.mm_actual_capture_addr[15];

wire [31:0] p_cap_d_00 = top.mm_actual_capture_data[ 0];
wire [31:0] p_cap_d_01 = top.mm_actual_capture_data[ 1];
wire [31:0] p_cap_d_02 = top.mm_actual_capture_data[ 2];
wire [31:0] p_cap_d_03 = top.mm_actual_capture_data[ 3];
wire [31:0] p_cap_d_04 = top.mm_actual_capture_data[ 4];
wire [31:0] p_cap_d_05 = top.mm_actual_capture_data[ 5];
wire [31:0] p_cap_d_06 = top.mm_actual_capture_data[ 6];
wire [31:0] p_cap_d_07 = top.mm_actual_capture_data[ 7];
wire [31:0] p_cap_d_08 = top.mm_actual_capture_data[ 8];
wire [31:0] p_cap_d_09 = top.mm_actual_capture_data[ 9];
wire [31:0] p_cap_d_10 = top.mm_actual_capture_data[10];
wire [31:0] p_cap_d_11 = top.mm_actual_capture_data[11];
wire [31:0] p_cap_d_12 = top.mm_actual_capture_data[12];
wire [31:0] p_cap_d_13 = top.mm_actual_capture_data[13];
wire [31:0] p_cap_d_14 = top.mm_actual_capture_data[14];
wire [31:0] p_cap_d_15 = top.mm_actual_capture_data[15];

//synthesis translate_on
endmodule
