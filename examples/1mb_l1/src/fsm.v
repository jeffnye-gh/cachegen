// -------------------------------------------------------------------------
// Cache control state machine
//
// Operations that hit should complete in one cycle, and have sustainable
// single cycle throughput. There will be 1 cycle of read latency due to the
// clocked address inputs of the rams. So reads will be 1+1/1, stores will
// be 1/1, latency/throughput
//
// Policies are read allocate and write allocate.
//
// Writes honor byte enables
//        sub-word write data is assumed to be in it's proper lane
// Reads ignore byte enables and return the whole word
//
// There are some ram test states. FIXME: I'll document them later
//
// Every operation accesses the tags, valid/dirty bits
//
// 
// Read hit
//            align the word of read data to the return bus, update LRU
//          
// Write hit  
//            align the write data to the word, use byte enables to 
//            conditionally change the data in the word, set the modify bit
//
// Read miss clean
//            - there is at least one way with unmodified data
//            send read request of a line from main memory
//            on mm data valid, forward critical word to PE
//            write line to clean way
//            update tags, valid, clear modify bit
//            update LRU
//
// Read miss dirty
//            - all ways have dirty bit set, so evict and allocate
//            send write request of LRU selected way to main memory
//            wait for mm to acknowledge the write request
//            read request of a line from main memory
//            on mm data valid, forward critical word to PE
//            write line to clean way
//            update tags, valid, clear modify bit
//            update LRU
//
// Write miss clean
//            - there is at least one way with unmodified data
//            send read request of a line from main memory
//            on mm data valid, merge word with mm line data
//            write line to clean way
//            update tags, valid, set modify bit
//            update LRU
//
// Write miss dirty
//            - all ways have dirty bit set, so evict and allocate
//            send write request of LRU selected way to main memory
//            wait for mm to acknowledge the write request
//            read request of a line from main memory
//            on mm data valid, merge word with mm line data
//            write line to clean way
//            update tags, valid, set modify bit
//            update LRU
//
// Flush 
//            - if specified line is dirty write it to main memory
//            read the tag/valid/mod bits
//            if all invalid do nothing
//            if all clean do nothing
//            for each  valid and modified way 
//              send write request way to main memory
//            mark all ways as clean
//            do not modify lru 
//
// Invalidate 
//            - clear the valid bit, ignore any modified state
//            do not modify lru 
//
// Flush all 
//            - walk the indexes and perform a Flush as above
//
// Invalidate all
//            - assert reset on the valid bit "register file"
//
// -------------------------------------------------------------------------
`include "functions.h"
module fsm #(
  parameter IDX_BITS = 13,
  parameter TAG_BITS = 14
)
(
//  input  wire        tb_ram_test,  //from TB

//  input  wire [31:0] pe_a,         //input from CPU
//  input  wire [3:0]  pe_be_d,        //input from CPU
//  input  wire        pe_read_d,      //input from CPU
//  input  wire        pe_write_d,     //input from CPU
//  input  wire [31:0] pe_writedata, //input from CPU, for ram test

  input  wire pe_read,
  input  wire pe_write,

  input  wire  pe_req_hit,
//  output reg  [31:0] fsm_pe_readdata, // to CPU
//  output reg  fsm_pe_readdata_valid,  // to CPU
//  output reg  fsm_pe_req_hit,         // to CPU

  output reg  [3:0] fsm_bit_cmd,
  output reg        fsm_bit_cmd_valid,
  output wire       fsm_cc_ary_write,
  output wire       fsm_cc_lru_write,
  output wire       fsm_cc_mod_write,
//
////  output reg  [TAG_BITS-1:0] fsm_cc_tag,
////  output reg  [IDX_BITS-1:0] fsm_cc_index,
////  output reg  [2:0]          fsm_cc_offset,
////  output reg  [3:0]          fsm_cc_be,
////  output wire [3:0]          fsm_cc_fill,
//
////  output wire                fsm_cc_tag_read,
//  output wire [3:0]          fsm_cc_tag_write,
//
////  output wire                fsm_cc_ary_read,
//
//  output reg  [3:0]          fsm_cc_way_match_q,
//
//  input wire [3:0]           cc_val_bits,
//  input wire [3:0]           cc_mod_bits,
//  input wire [TAG_BITS-1:0]  cc_tag_readdata[3:0],
//  input wire [255:0]         cc_ary_readdata[3:0],
//
//  input wire pe_flush,
//  input wire pe_flush_all,
//  input wire pe_invalidate,
//  input wire pe_invalidate_all,
//
//  input  wire mm_readdata_valid,

  input reset,
  input clk
);
`include "bitcmds.h"
// ----------------------------------------------------------------------
// States - FIXME: reduce state width as needed once fsm works
// ----------------------------------------------------------------------
localparam [3:0] IDLE       = 4'h0;
localparam [3:0] WR_ALLOC   = 4'h1;
localparam [3:0] RD_ALLOC   = 4'h2;
localparam [3:0] WR_EVICT   = 4'h3;
localparam [3:0] RD_EVICT   = 4'h4;
localparam [3:0] FLUSH_ALL  = 4'h5;
localparam [3:0] INVAL_ALL  = 4'h6;
localparam [3:0] TEMP       = 4'h7;
//FAKE states for debug
localparam [3:0] RD_HIT     = 4'h8;
localparam [3:0] WR_HIT     = 4'h9;
localparam [3:0] TST_READ   = 4'ha;
localparam [3:0] TST_WRITE  = 4'hb;
// ----------------------------------------------------------------------
// temps
// ----------------------------------------------------------------------
wire FIXME_mm_readdata_valid = 1'b1;
//assign fsm_bit_cmd = B_CMD_NOP;
//assign fsm_bit_cmd_valid = 1'b0;
assign fsm_cc_tag_write = 4'b0;
assign fsm_cc_fill = 4'b0;
// ----------------------------------------------------------------------
//reg fsm_readdata_valid_d;fsm_readdata_valid;
reg fsm_cc_ary_write_d;
reg fsm_cc_lru_write_d;
reg fsm_cc_mod_write_d;

assign fsm_cc_ary_write = fsm_cc_ary_write_d;
assign fsm_cc_lru_write = fsm_cc_lru_write_d;
assign fsm_cc_mod_write = fsm_cc_mod_write_d;
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
reg [3:0] state,next;
// ----------------------------------------------------------------------
always @(posedge clk) begin
  state <= reset ? IDLE : next;
end
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
always @* begin

  //fsm_readdata_valid_d = 1'b0;
  fsm_cc_ary_write_d   = 1'b0;
  fsm_cc_lru_write_d   = 1'b0;
  fsm_cc_mod_write_d   = 1'b0;

  fsm_bit_cmd          = B_CMD_NOP;
  fsm_bit_cmd_valid    = 1'b0;

  next = IDLE;

  casez(state) 
    IDLE: begin
      // READ HIT
      if(pe_read & pe_req_hit) begin //&  fsm_pe_req_hit)  begin
        //fsm_pe_readdata_valid_d = 1'b1;
        //fsm_bit_cmd = B_CMD_LRU_UP;
        //fsm_bit_cmd_valid = 1'b1;
        fsm_cc_lru_write_d = 1'b1;
        next = IDLE; //RD_HIT;
      end

      // WRITE HIT
      else if(pe_write & pe_req_hit)  begin
        //fsm_bit_cmd = B_CMD_LRU_UP; //B_CMD_LRU_MOD_UP;
        //fsm_bit_cmd_valid = 1'b1;
        fsm_cc_ary_write_d  = 1'b1;
        fsm_cc_lru_write_d = 1'b1;
        fsm_cc_mod_write_d = 1'b1;
        next = IDLE;
      end

//      // READ MISS CLEAN
//      else if(pe_read & !fsm_pe_req_hit & !pe_req_mod) begin
//        next = RD_ALLOC;
//      end
//
//      // WRITE MISS CLEAN
//      else if(pe_write & !fsm_pe_req_hit &  pe_req_mod) begin
//        next = WR_ALLOC;
//      end
//
//      // READ MISS DIRTY
//      else if(pe_read & !fsm_pe_req_hit &  pe_req_mod) begin
//        next = RD_EVICT;
//      end
//
//      // WRITE MISS DIRTY
//      else if(pe_write & !fsm_pe_req_hit &  pe_req_mod) begin
//        next = WR_EVICT;
//      end
//
//      // FLUSH
//      else if(pe_flush) begin
//        next = IDLE;
//      end
//
//      // FLUSH ALL
//      else if(pe_flush_all) begin
//        next = FLUSH_ALL;
//      end
//
//      // INVALIDATE
//      else if(pe_invalidate) begin
//        next = IDLE;
//      end
//
//      // INVALIDATE ALL
//      else if(pe_invalidate_all) begin
//        next = INVAL_ALL;
//      end
//
//      // RAM TEST READ
//      else if(tb_ram_test & pe_read) begin
//        next = IDLE;
//      end
//
//      // RAM TEST WRITE
//      else if(tb_ram_test & pe_write) begin
//        next = IDLE;
//      end
//      
//      // FALL THROUGH IDLE CONDITION
//      else begin
//        next = IDLE;
//        //$display("-E: fall through in fsm IDLE state");      
//      end
//
    end //end of IDLE
//
////    RD_HIT: begin
////      next = IDLE;
////      fsm_bit_cmd = B_CMD_NOP;
////      fsm_bit_cmd_valid = 1'b1;
////    end
//
////    WR_HIT: begin next = IDLE; end
//
//    RD_ALLOC: begin
//      if(FIXME_mm_readdata_valid) begin
//        next = IDLE;
//      end else begin
//        next = RD_ALLOC;
//      end
//    end
//
//    RD_EVICT: begin
//      if(FIXME_mm_readdata_valid) begin
//        next = RD_ALLOC;
//      end else begin
//        next = RD_EVICT;
//      end
//    end
//
//    WR_ALLOC: begin
//      if(FIXME_mm_readdata_valid) begin
//        next = IDLE;
//      end else begin
//        next = WR_ALLOC;
//      end
//    end
//
//    WR_EVICT: begin
//      if(FIXME_mm_readdata_valid) begin
//        next = WR_ALLOC;
//      end else begin
//        next = WR_EVICT;
//      end
//    end
//
//    FLUSH_ALL: begin
//      next = IDLE;
//    end
//
//    INVAL_ALL: begin
//      next = IDLE;
//    end
//
//    TEMP: begin
//      next = TEMP;
//    end
//
    default: begin
//      //synthesis translate_off
//      $display("fsm state fall through"):
//      //synthesis translate_on
      next = TEMP;
    end
  endcase

end
// ----------------------------------------------------------------------
// Probes
// ----------------------------------------------------------------------
////synthesis translate_off
//reg [(8*8)-1:0] str_state;
//always @* begin
//  case(state)
//    IDLE:       str_state = "IDLE";
//    WR_ALLOC:   str_state = "WR_AL";
//    RD_ALLOC:   str_state = "RD_AL";
//    WR_EVICT:   str_state = "WR_EV";
//    RD_EVICT:   str_state = "RD_EV";
//    FLUSH_ALL:  str_state = "FLUSH";
//    INVAL_ALL:  str_state = "INVAL";
//    TEMP:       str_state = "TEMP";
////debug
//    RD_HIT:     str_state = "RD_HIT";
//    WR_HIT:     str_state = "WR_HIT";
//    TST_READ:   str_state = "TST_RD";
//    TST_WRITE:  str_state = "TST_WR";
//    default:    str_state = "UNKWN";
//  endcase
//end
//
//////$isunknown does not exist in icarus verilog
//////$onehot does not exist in icarus verilog
////function isunknown(input [3:0] in);
////begin isunknown = (^in == 1'bX); end
////endfunction
////
////function onehot(input [3:0] in);
////integer i;
////integer cnt;
////begin 
////  cnt = 0;
////  for(i=0;i<4;i=i+1) begin
////    cnt += in[i];
////  end
////  onehot = (cnt === 1);
////end
////endfunction
//
////reg [(4*8)-1:0] str_way;
////always @* begin
////  case(fsm_cc_way_match)
////    4'b0001: str_way = "WAY0";
////    4'b0010: str_way = "WAY1";
////    4'b0100: str_way = "WAY2";
////    4'b1000: str_way = "WAY3";
////    default: str_way = "x";
////  endcase
////end
////synthesis translate_on
endmodule
