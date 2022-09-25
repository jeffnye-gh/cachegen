`include "sim_cntrl.h"
// ----------------------------------------------------------------------
// FIXME: verify these equations work when hitting same index same way
//        bits will flip. Check the algorithm.
//
// In spite of the many pages which describe bit based P-LRU, I did not find 
// a Feynman clarity explanation of the algorithm. Here's my try at it.
//
// This is what I am implementing for a 4 way associative cache, 
// where ways are organized in pairs for the purpose of PLRU. This is 
// typical, allowing 3 bits to represent the LRU state for 4 ways.
//
// The bits are labeled as:
//
//          b2            LRU bits {b2,b1,b0}
//         /  \           bN = zero points left, {0,0,0} = points to way3
//       b1    b0
//      /  \  /  \
//    w3  w2  w1  w0      WAY labels, w0 -> way0
//
// By definition access to a way defines the most recently used way entry of
// all entries, so it follows that it also defines the least recently used 
// way of that pair.
//
//     access to way0 means that, of way1 and way0, way1 one is now least
//     recently used.
//
// By the same reasoning a way access also defines the most recently used
// pair of ways.
//
//     access to way0 means that ways on the other side, way3 and way2 are
//     least recently used as compared to way0.
//
// You can reason then that accessing a way in one pair should not change 
// the LRU state of the unaccessed pair.
//
// Implementation of updates to the LRU state is mostly based
// on the accessed way, and two bits of previous state.
//
// Truth table showing the new LRU state derived from the access. b1=b1 means
// b1 is unchanged from previous state.
//
// LRU update rules:
//
// access to way0    b2=0  b1=b1  b0=0    b2 points left, b0 points left
// access to way1    b2=0  b1=b1  b0=1 
// access to way2    b2=1  b1=0   b0=b0
// access to way3    b2=1  b1=1   b0=b0
// 
// Sanity check: access to way0 or way1 causes b2 to point left(0), since
// way3 and way2 are now least recently used compared to way0 or way1.
//
// Quasi-verilog:
//
// way_access is one hot, way_access[3]=1 means way3 was accessed.
//
// reg  [2:0] next_lru;   //value to be written to LRU array
// wire [2:0] prev_lru;  //prevous lru, prev_lru[2] = b2, etc
//
// casez(way_access[3:0])
//   4'b???1:  next_lru = { 0, prev_lru[1], 0 };
//   4'b??1?:  next_lru = { 0, prev_lru[1], 1 };
//   4'b?1??:  next_lru = { 1, 0,           prev_lru[0] };
//   4'b1???:  next_lru = { 1, 1,           prev_lru[0] };
//   default:  next_lru = 3'bx;
// endcase
//
// Single index example, start with lru = 3'b000 
//
// read/write       prev_lru |   next LRU   | result
// --------------------------|--------------|--------
// access way3      0  0  0  |  1  1    b0  |  1 1 0
// access way1      1  1  0  |  0  b1   1   |  0 1 1
// access way2      0  1  1  |  1  0    b0  |  1 0 1             
// access way0      1  0  1  |  0  b1   0   |  0 0 0
// access way3      0  0  0  |  1  1    b0  |  1 1 0
//
// ----------------------------------------------------------------------
// Victim select
//                        <same as before>
//          b2            LRU bits {b2,b1,b0}
//         /  \           bN = zero points left, {0,0,0} = way3
//       b1    b0
//      /  \  /  \
//    w3  w2  w1  w0      WAY labels, w0 -> way0
//
// LRU selected victim, assuming all ways are valid, {b2,b1,b0};
//
// LRU  | valid bits | LRU selected victim
// -----|------------|--------------------------
// 00x  | 1111       | way 3 
// 01x  | 1111       | way 2
// 1x0  | 1111       | way 1
// 1x1  | 1111       | way 0
//
// Victim selection due to invalid, left most invalid is selected. LRU does
// not select the victim if there are invalid ways
//
// LRU  | valid bits | Invalid selected victim
// -----|------------|--------
// ???  | 0---       | way 3 
// ???  | 10--       | way 2
// ???  | 110-       | way 1
// ???  | 1110       | way 0
//
// ----------------------------------------------------------------------------
module bitarray #(
  parameter IDX_BITS=13
)
(
  output reg  [3:0] val_out,
  output reg  [3:0] mod_out,
  output reg  [2:0] lru_out,
  output reg        lru_wr,

  input wire [IDX_BITS-1:0] pe_index_d,
  input wire [IDX_BITS-1:0] pe_index_q,
  input wire [3:0] way_hit,

  input wire pe_read_d,
  input wire pe_write_d,

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
//wire [2:0] _lru_0 = lbits[0];
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
//task clearlru;
//integer i;
//begin for(i=0;i<ENTRIES;++i) lbits[i] = 4'b0; end
//endtask
// ----------------------------------------------------------------------
// FIXME: save typing, not synthesizable, fix it later
// ----------------------------------------------------------------------
always @(posedge reset) begin invalidate(); end
// ======================================================================
// begin design
// ======================================================================

// ----------------------------------------------------------------------
// Bit arrays
// ----------------------------------------------------------------------
reg [3:0] vbits[0:ENTRIES-1];
reg [3:0] mbits[0:ENTRIES-1];
//reg [2:0] lbits[0:ENTRIES-1];
// ----------------------------------------------------------------------
// Raw bit data, shared
// ----------------------------------------------------------------------
wire [3:0] val_bits_d = vbits[pe_index_d];
wire [3:0] mod_bits_d = mbits[pe_index_d];
//wire [3:0] lru_bits_d = lbits[pe_index_d];
// ----------------------------------------------------------------------
reg [1:0] victim;           //way selected for fill or eviction by lru
reg [1:0] invalid_victim;   //way selected for fill by (in)valid bits
// ----------------------------------------------------------------------
reg val_wr,val_wd,mod_wr,mod_wd;
reg  [2:0] lru_wd;
reg  [3:0] cmd_q;
reg  access_q;

wire access_d = (pe_read_d | pe_write_d) & !reset;
wire all_valid_d = &vbits[pe_index_d];  
// ----------------------------------------------------------------------
// Invalid 'victim' determination
// ----------------------------------------------------------------------
//always @* begin
//  casez(val_bits_d)
//    4'b0???: invalid_victim = WAY3;
//    4'b10??: invalid_victim = WAY2;
//    4'b110?: invalid_victim = WAY1;
//    4'b1110: invalid_victim = WAY0;
//    default: begin
//      invalid_victim = 2'bx;
//      //synthesis translate off
//      //if(access) $display("-E: fall through in invalid_victim case");
//      //synthesis translate on
//    end
//  endcase
//end
// ----------------------------------------------------------------------
// Victim select
// ----------------------------------------------------------------------
//always @* begin
//  casez(lru_bits_d) 
//    3'b?00:  victim = all_valid_d ? WAY3 : invalid_victim;
//    3'b?10:  victim = all_valid_d ? WAY2 : invalid_victim;
//    3'b0?1:  victim = all_valid_d ? WAY1 : invalid_victim;
//    3'b1?1:  victim = all_valid_d ? WAY0 : invalid_victim;
//    default: victim = 2'bx;
//  endcase
//end
// ----------------------------------------------------------------------
// LRU update data
//
//   lru is read data from bit array, aka prev_lru
//   lru_byp is the bypass enabled data
//
//   lru_wd is write data to bit array, aka next_lru
//
//  Since the LRU is updated after hit detection there is a need to bypass
//  LRU array write data when there is a back to back access to the same
//  index
//
// LRU rules
// access to way0    b2=0  b1=b1  b0=0
// access to way1    b2=0  b1=b1  b0=1 
// access to way2    b2=1  b1=0   b0=b0
// access to way3    b2=1  b1=1   b0=b0
// ----------------------------------------------------------------------
//wire bypass_lru;
//reg access_q2;
//reg  [2:0] last_lru_wd;
//reg  [IDX_BITS-1:0] pe_index_q2;
//assign bypass_lru = access_q2 & access_q & (pe_index_q == pe_index_q2);
//
//wire [2:0] lru_byp = bypass_lru ? last_lru_wd : lru_out; 
//
//always @(way_hit or lru_out) begin
//  casez(way_hit)
//    4'b???1: lru_wd =  { 1'b0, lru_byp[1], 1'b0       }; //WAY0
//    4'b??1?: lru_wd =  { 1'b0, lru_byp[1], 1'b1       }; //WAY1
//    4'b?1??: lru_wd =  { 1'b1, 1'b0,       lru_byp[0] }; //WAY2
//    4'b1???: lru_wd =  { 1'b1, 1'b1,       lru_byp[0] }; //WAY3
//    default: lru_wd = 3'bx;
//  endcase
//end
//
//// ----------------------------------------------------------------------
always @(posedge clk) begin
//  last_lru_wd <= lru_wd;
//
//  pe_index_q2 <= pe_index_q;
//
  val_out <= {4{access_d}} & vbits[pe_index_d];
  mod_out <= {4{access_d}} & mbits[pe_index_d];
//  lru_out <= {3{access_d}} & lbits[pe_index_d];
 
  access_q <= access_d; 
//  access_q2 <= access_q; 
  cmd_q <= cmd;

  vbits[pe_index_d][0] <= val_wr & way_hit[0] ? val_wd : vbits[pe_index_d][0];
  vbits[pe_index_d][1] <= val_wr & way_hit[1] ? val_wd : vbits[pe_index_d][1];
  vbits[pe_index_d][2] <= val_wr & way_hit[2] ? val_wd : vbits[pe_index_d][2];
  vbits[pe_index_d][3] <= val_wr & way_hit[3] ? val_wd : vbits[pe_index_d][3];

  mbits[pe_index_d][0] <= mod_wr & way_hit[0] ? mod_wd : mbits[pe_index_d][0];
  mbits[pe_index_d][1] <= mod_wr & way_hit[1] ? mod_wd : mbits[pe_index_d][1];
  mbits[pe_index_d][2] <= mod_wr & way_hit[2] ? mod_wd : mbits[pe_index_d][2];
  mbits[pe_index_d][3] <= mod_wr & way_hit[3] ? mod_wd : mbits[pe_index_d][3];

end
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
always @* begin
  val_wr = 1'b0;
  mod_wr = 1'b0;
  lru_wr = 1'b0;

  val_wd = 1'bx;
  mod_wd = 1'bx;

  case(cmd)
    B_CMD_LRU_UP: begin 
      lru_wr = 1'b1;
    end

    B_CMD_LRU_MOD_UP: begin 
      lru_wr = 1'b1;
      mod_wr = 1'b1;
    end

    B_CMD_READ: begin //get/check the values of the bits, and so update lru
      lru_wr = 1'b1;
    end

    B_CMD_VAL: begin //set valid bit
      val_wr = 1'b1;
      val_wd = 1'b1;
      lru_wr = 1'b1;
    end

    B_CMD_INVAL: begin //clear valid bit
      val_wr = 1'b1;
      val_wd = 1'b0;
    end

    B_CMD_MOD: begin //set dirty bit
      mod_wr = 1'b1;
      mod_wd = 1'b1;
      lru_wr = 1'b1;
    end

    B_CMD_CLEAN: begin //clear dirty bit
      mod_wr = 1'b1;
      mod_wd = 1'b0;
    end

    B_CMD_VAL_MOD: begin //write allocate, set valid,  set dirty
      val_wr = 1'b1;
      mod_wr = 1'b1;
      lru_wr = 1'b1;

      val_wd = 1'b1;
      mod_wd = 1'b0;
    end

    B_CMD_ALLOC: begin //read allocate, set valid, clear dirty
      val_wr = 1'b1;
      mod_wr = 1'b1;
      lru_wr = 1'b1;

      val_wd = 1'b1;
      mod_wd = 1'b0;
    end

    B_CMD_INVAL_ALL: begin //gang invalidate
      invalidate();
      //clearlru();
    end

    default: begin
      val_wr = 1'b0;
      mod_wr = 1'b0;
      lru_wr = 1'b0;
      val_wd = 1'bx;
      mod_wd = 1'bx;
    end

  endcase
end
// ----------------------------------------------------------------------
// Probes
// ----------------------------------------------------------------------
//synthesis translate_off
//string str_bcmd;
//always @* begin
//  case(cmd_q)
//    B_CMD_NOP:     str_bcmd = "NOP";
//    B_CMD_LRU_UP:  str_bcmd = "LRU_UP";
//    default:       str_bcmd = "UNKN";
//  endcase
//end
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
