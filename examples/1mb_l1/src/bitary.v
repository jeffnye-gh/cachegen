`include "sim_cntrl.h"

//FIXME: baked in assumption of 4 ways
module bitarray #(
  parameter IDX_BITS=13
)
(
  output reg  [3:0] val,
  output reg  [3:0] mod,
  output reg  [3:0] lru, //one hot

  input wire [IDX_BITS-1:0] index,
  input wire [3:0] way_match,

  input wire pe_read,
  input wire pe_write,

  input wire [3:0] cmd,
  input wire       cmd_valid,

  input wire reset,
  input wire clk
);
`include "bitcmds.h"

localparam integer ENTRIES = 2**IDX_BITS;

localparam [1:0] WAY0 = 2'b00;
localparam [1:0] WAY1 = 2'b01;
localparam [1:0] WAY2 = 2'b10;
localparam [1:0] WAY3 = 2'b11;
// ----------------------------------------------------------------------
// Probes
// ----------------------------------------------------------------------
//synthesis translate_off
wire [2:0] _lru_0 = lbits[0];
//synthesis translate_on
// ======================================================================
// FIXME: save typing, not synthesizable, fix it later
// ======================================================================
// ----------------------------------------------------------------------
task invalidate;
integer i;
begin for(i=0;i<ENTRIES;++i) vbits[i] = 4'b0; end
endtask
// ----------------------------------------------------------------------
task clearlru;
integer i;
begin for(i=0;i<ENTRIES;++i) lbits[i] = 4'b0; end
endtask
// ----------------------------------------------------------------------
// FIXME: save typing, not synthesizable, fix it later
// ----------------------------------------------------------------------
always @(posedge reset) begin invalidate(); clearlru(); end
// ======================================================================
// begin design
// ======================================================================

// ----------------------------------------------------------------------
// Bit arrays
// ----------------------------------------------------------------------
reg [3:0] vbits[0:ENTRIES-1];
reg [3:0] mbits[0:ENTRIES-1];
reg [2:0] lbits[0:ENTRIES-1];
// ----------------------------------------------------------------------
// Raw bit data, shared
// ----------------------------------------------------------------------
wire [3:0] val_bits_d = vbits[index];
wire [3:0] mod_bits_d = mbits[index];
wire [3:0] lru_bits_d = lbits[index];

// ----------------------------------------------------------------------
reg [1:0] victim;           //way selected for fill or eviction by lru
reg [1:0] invalid_victim;   //way selected for fill by (in)valid bits
// ----------------------------------------------------------------------
reg val_wr,val_wd,mod_wr,mod_wd,lru_up;
reg  [2:0] lru_wd;

reg  [3:0] cmd_q;

wire access      = (pe_read | pe_write) & !reset;
wire all_valid_d = &vbits[index];  
// ----------------------------------------------------------------------
// LRU scheme is 'tree-LRU' 4 way so 3 bits
//
// LRU = 3'b000  - way3 is LRU 
//
//         b0
//        /  \
//      b1    b2
//      / \  /  \
//     3  2  1  0 
//
// LRU    valid bits victim or left most valid
//     
// x00    1111  way 3 
// x10    1111  way 2
// 0x1    1111  way 1
// 1x1    1111  way 0
//
// If !all_valid then left most invalid is used as destination for fill
//        0---  way 3 
//        10--  way 2
//        110-  way 1
//        1110  way 0
//
// ----------------------------------------------------------------------
// Invalid 'victim' determination
// ----------------------------------------------------------------------
always @* begin
  casez(val_bits_d)
    4'b0???: invalid_victim = WAY3;
    4'b10??: invalid_victim = WAY2;
    4'b110?: invalid_victim = WAY1;
    4'b1110: invalid_victim = WAY0;
    default: begin
      invalid_victim = 2'bx;
      //synthesis translate off
      //if(access) $display("-E: fall through in invalid_victim case");
      //synthesis translate on
    end
  endcase
end
// ----------------------------------------------------------------------
// Victim select
// ----------------------------------------------------------------------
always @* begin
  casez(lru_bits_d) 
    3'b?00:  victim = all_valid_d ? WAY3 : invalid_victim;
    3'b?10:  victim = all_valid_d ? WAY2 : invalid_victim;
    3'b0?1:  victim = all_valid_d ? WAY1 : invalid_victim;
    3'b1?1:  victim = all_valid_d ? WAY0 : invalid_victim;
    default: victim = 2'bx;
  endcase
end
// ----------------------------------------------------------------------
//
// LRU update equations
//
// access |  new lru
// way3   |  !b1 !b0
// way2   |  !b1 !b0
// way1   |  !b2 !b0
// way0   |  !b2 !b0
//
// Example, start with lru = 3'b000 
//
// read/write        boolean w   next LRU
// ---------------------------------------
// access way3      b2 !b1 !b0   = 0 1 1
// access way1     !b2  b1 !b0   = 1 1 0
// access way2      b2 !b1 !b0   = 1 0 1
// access way0     !b2  b1 !b0   = 0 0 0
// access way3      b2 !b1 !b0   = 0 1 1
// ----------------------------------------------------------------------
// LRU update data
// ----------------------------------------------------------------------
reg [2:0] lru_rd;
always @* begin
  lru_rd = lbits[index];
  casez(way_match)
    4'b1???: lru_wd =  {  lru_rd[2], !lru_rd[1], !lru_rd[0] }; //WAY3
    4'b?1??: lru_wd =  {  lru_rd[2], !lru_rd[1], !lru_rd[0] }; //WAY2
    4'b??1?: lru_wd =  { !lru_rd[2],  lru_rd[1], !lru_rd[0] }; //WAY1
    4'b???1: lru_wd =  { !lru_rd[2],  lru_rd[1], !lru_rd[0] }; //WAY0
    default: lru_wd = 3'bx;
  endcase
end

// ----------------------------------------------------------------------
always @(posedge clk) begin
  val <= {4{access}} & vbits[index];
  mod <= {4{access}} & mbits[index];
  lru <= {3{access}} & lbits[index];
  
  cmd_q <= cmd;

  vbits[index][0] <= val_wr & way_match[0] ? val_wd : vbits[0];
  vbits[index][1] <= val_wr & way_match[1] ? val_wd : vbits[1];
  vbits[index][2] <= val_wr & way_match[2] ? val_wd : vbits[2];
  vbits[index][3] <= val_wr & way_match[3] ? val_wd : vbits[3];

  mbits[index][0] <= mod_wr & way_match[0] ? mod_wd : mbits[0];
  mbits[index][1] <= mod_wr & way_match[1] ? mod_wd : mbits[1];
  mbits[index][2] <= mod_wr & way_match[2] ? mod_wd : mbits[2];
  mbits[index][3] <= mod_wr & way_match[3] ? mod_wd : mbits[3];

//  lbits[index] <= lru_up ? lru_wd : lru_bits_d[index];
  lbits[index] <= lru_up ? lru_wd : lbits[index];
end
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
always @* begin
  val_wr = 1'b0;
  mod_wr = 1'b0;
  lru_up = 1'b0;

  val_wd = 1'bx;
  mod_wd = 1'bx;

  case(cmd_q)
    B_CMD_LRU_UP: begin //get/check the values of the bits, and so update lru
      lru_up = 1'b1;
    end

    B_CMD_READ: begin //get/check the values of the bits, and so update lru
      lru_up = 1'b1;
    end

    B_CMD_VAL: begin //set valid bit
      val_wr = 1'b1;
      val_wd = 1'b1;
      lru_up = 1'b1;
    end

    B_CMD_INVAL: begin //clear valid bit
      val_wr = 1'b1;
      val_wd = 1'b0;
    end

    B_CMD_MOD: begin //set dirty bit
      mod_wr = 1'b1;
      mod_wd = 1'b1;
      lru_up = 1'b1;
    end

    B_CMD_CLEAN: begin //clear dirty bit
      mod_wr = 1'b1;
      mod_wd = 1'b0;
    end

    B_CMD_VAL_MOD: begin //write allocate, set valid,  set dirty
      val_wr = 1'b1;
      mod_wr = 1'b1;
      lru_up = 1'b1;

      val_wd = 1'b1;
      mod_wd = 1'b0;
    end

    B_CMD_ALLOC: begin //read allocate, set valid, clear dirty
      val_wr = 1'b1;
      mod_wr = 1'b1;
      lru_up = 1'b1;

      val_wd = 1'b1;
      mod_wd = 1'b0;
    end

    B_CMD_INVAL_ALL: begin //gang invalidate
      invalidate();
      clearlru();
    end

    default: begin
      val_wr = 1'b0;
      mod_wr = 1'b0;
      lru_up = 1'b0;
      val_wd = 1'bx;
      mod_wd = 1'bx;
    end

  endcase
end
// ----------------------------------------------------------------------
// Probes
// ----------------------------------------------------------------------
//synthesis translate_off
reg [(8*8)-1:0] str_bcmd;
always @* begin
  case(cmd_q)
    B_CMD_NOP:     str_bcmd = "NOP";
    B_CMD_LRU_UP:  str_bcmd = "LRU_UP";
    default:       str_bcmd = "UNKN";
  endcase
end
//B_CMD_VAL:   VAL
//B_CMD_INVAL: INVAL
//B_CMD_MOD:   MOD
//B_CMD_CLEAN: CLEAN
//B_CMD_INVAL_ALL:INVAL_ALL
//B_CMD_ALLOC:ALLOC
//B_CMD_VAL_MOD:VAL_MOD
//B_CMD_READ:READ
//
//    IDLE:       str_bcmd = "IDLE";
//    B_CMD_INVAL_ALL:   str_bcmd = "WR_AL";
//    B_CMD_ALLOC:   str_bcmd = "RD_AL";
//    :   str_bcmd = "WR_EV";
//    RD_EVICT:   str_bcmd = "RD_EV";
//    FLUSH_ALL:  str_bcmd = "FLUSH";
//    INVAL_ALL:  str_bcmd = "INVAL";
//    TEMP:       str_bcmd = "TEMP";
////debug
//    RD_HIT:     str_bcmd = "RD_HIT";
//    WR_HIT:     str_bcmd = "WR_HIT";
//    TST_READ:   str_bcmd = "TST_RD";
//    TST_WRITE:  str_bcmd = "TST_WR";
//    default:    str_bcmd = "UNKWN";
//  endcase
//end
//synthesis translate_on

endmodule
