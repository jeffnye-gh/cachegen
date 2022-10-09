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
  parameter IDX_BITS     = 13,
  parameter TAG_BITS     = 14,
  parameter READ_HIT_LAT = 0
)
(

  input  wire pe_read_d,
  input  wire pe_write_d,

  input  wire  pe_req_hit_d,
  input  wire  pe_req_miss_d,
  input  wire  pe_req_mod_d,

  output reg   fsm_cc_ary_write_d,
  output reg   fsm_cc_tag_write_d,
  output reg   fsm_cc_val_write_d,
  output reg   fsm_cc_mod_write_d,
  output reg   fsm_cc_lru_write_d,

  output reg   fsm_cc_is_val_d,
  output reg   fsm_cc_is_mod_d,
  output reg   fsm_cc_fill_d,

  output reg   fsm_cc_readdata_valid,

  output reg   fsm_mm_read_d,
  output reg   fsm_mm_write_d,

  input  wire mm_readdata_valid,

  input reset,
  input clk
);
`include "bitcmds.h"
`include "fsm_state.h"
// ----------------------------------------------------------------------
// temps
// ----------------------------------------------------------------------
//n/a
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
reg [3:0] state,next;
reg fsm_cc_readdata_valid_d;
// ----------------------------------------------------------------------
always @(posedge clk) begin
  state <= reset ? IDLE : next;
  fsm_cc_readdata_valid <= fsm_cc_readdata_valid_d;
end
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
always @* begin

  fsm_cc_ary_write_d   = 1'b0;
  fsm_cc_tag_write_d   = 1'b0;
  fsm_cc_val_write_d   = 1'b0;
  fsm_cc_mod_write_d   = 1'b0;
  fsm_cc_lru_write_d   = 1'b0;
  fsm_cc_fill_d        = 1'b0;

  fsm_cc_is_mod_d      = 1'bx;
  fsm_cc_is_val_d      = 1'bx;

  fsm_cc_readdata_valid_d = 1'b0;

  fsm_mm_read_d  = 1'b0;
  fsm_mm_write_d = 1'b0;

  next = IDLE;

  casez(state) 
    IDLE: begin
      // READ HIT
      if(pe_read_d & pe_req_hit_d) begin 
        fsm_cc_lru_write_d = 1'b1;
        fsm_cc_readdata_valid_d = 1'b1;
        next = IDLE;
      end

      // WRITE HIT
      else if(pe_write_d & pe_req_hit_d)  begin
        fsm_cc_ary_write_d = 1'b1;
        fsm_cc_lru_write_d = 1'b1;
        fsm_cc_mod_write_d = 1'b1;
        fsm_cc_is_mod_d    = 1'b1;
        next = IDLE;
      end

//      // READ MISS CLEAN
//      else if(pe_read_d & pe_req_miss_d & !pe_req_mod_d) begin
//        fsm_mm_read_d = 1'b1;
//        next = RD_ALLOC;
//      end
//
//      // WRITE MISS CLEAN
//      else if(pe_write_d & !pe_req_miss_d & !pe_req_mod_d) begin
//        next = WR_ALLOC;
//      end
//
//      // READ MISS DIRTY
//      else if(pe_read_d & !pe_req_miss_d &  pe_req_mod_d) begin
//        next = RD_EVICT;
//      end
//
//      // WRITE MISS DIRTY
//      else if(pe_write_d & !pe_req_miss_d &  pe_req_mod_d) begin
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
//      else if(tb_ram_test & pe_read_d) begin
//        next = IDLE;
//      end
//
//      // RAM TEST WRITE
//      else if(tb_ram_test & pe_write_d) begin
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

//    RD_ALLOC: begin
//      if(mm_readdata_valid) begin
//        fsm_cc_readdata_valid_d = 1'b1;
//        next = IDLE;
//      end else begin
//        next = RD_ALLOC;
//      end
//    end

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

    default: begin
//      //synthesis translate_off
//      $display("fsm state fall through"):
//      //synthesis translate_on
      next = TEMP;
    end
  endcase

end
endmodule
