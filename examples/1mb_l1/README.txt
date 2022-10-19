// vi:syntax=verilog
// ----------------------------------------------------------------------
//  Cache parameters
//
//  1MB  total size
//    4  way set associative
//  32B  line/block size
//
//  I am assuming a 32b address space, with cache backed by an abstracted
//  4GB main memory model using SV associative array where a key looks up
//    a cache line. Read from unallocated memory will generate a message
//    but I don't plan on a hardware handshake at the moment.
//
//  NOTE: associative array idea is a problem. I am having trouble 
//        convincing iverilog to allow data types in the array declarations.
//        See the source files for more.
//
//  Capacity    2^20 bytes
//  Block size  2^5
//  Assoc.      2^2
//
//  Offset size is 5 bits - from Block size      = 2^5  bytes
//  #Blocks = Capacity / Block size = 2^20 / 2^5 = 2^15 blocks
//  #Sets   = #Blocks  / #Ways      = 2^15 / 2^2 = 2^13 sets
//
//  With 32b address
//     tag width is 32 - 13 - 5 = 14b,  31:18
//     set index is 13 bits             17:5     8KB entries
//     offset width is  5 bits           4:0
//
//  Address fields:
//
//  33222222222211 11111111          
//  10987654321098 7654321098765 43210
//  TTTTTTTTTTTTTT SSSSSSSSSSSSS ooooo
//
//  With byte addressability this becomes
//                                     bbbb
//  33222222222211 11111111            eeee
//  10987654321098 7654321098765 432-- 3210
//  TTTTTTTTTTTTTT SSSSSSSSSSSSS ooo-- bbbb
//
//  The tags are    8KB x 4 x 14b
//  Valid bits are  8KB x 4 x  1b
//  Dirty bits are  8KB x 4 x  1b
//  LRU   bits are  8KB   x    3b
//
//  The dirty bits could be part of the tag ram. Early decision to keep them
//  separate.
//
//  The valid bits are in an array of FF's to avoid walking the array during
//  reset or mass invalidate. 
//
//  RAMs are modeled behaviorally without the usual conditional insertion
//  of compile RAM instances 
//
//  Arrays:
//    Tag way0 8KB x 14         cache/sram/tag0
//    Tag way1 8KB x 14         cache/sram/tag1
//    Tag way2 8KB x 14         cache/sram/tag2
//    Tag way3 8KB x 14         cache/sram/tag3
//    Valid way0 8KB x 1        cache/bitary/vbits
//    Valid way1 8KB x 1        cache/bitary/vbits
//    Valid way2 8KB x 1        cache/bitary/vbits
//    Valid way3 8KB x 1        cache/bitary/vbits
//    Dirty way0 8KB x 1        cache/bitary/mbits
//    Dirty way1 8KB x 1        cache/bitary/mbits
//    Dirty way2 8KB x 1        cache/bitary/mbits
//    Dirty way3 8KB x 1        cache/bitary/mbits
//    LRU bits   8KB x 3        cache/bitary/lbits
//
//  Policies
//    Read miss policy is allocate, of course
//    Write miss policy is allocate
//    Write hit policy is no-write-through
//    Initial eviction policy is psuedo-LRU, 3 bit tree
//    There are no memory attributes or other MPU-like regions
//
//  Control
//    A case style FSM implements control. This control interprets access
//    signals and status bits. The FSM drives the control signals on RAMs
//    etc. For testing the FSM supports commands from the testbench, e.g.
//    tag/status/darray read/write, and bypasses the typical sequence.
//
//    NOTE: below is just a thought, see the implementation files for more.
//
//    Possible commands are below. I may not implement all of these. Allocating
//    4 bits for the command signals from the TB.
//
//    CMD_NOP         idle cycle
//    CMD_NORMAL      conventional operation
//    CMD_BYPASS      do not modify L1, perform the access directly on MM
//    CMD_INVAL       clear valid bit for a single entry
//    CMD_INVAL_ALL   mass invalidate
//    CMD_DIRTY       set dirty bit for a single entry
//    CMD_CLEAN       clear dirty bit for a single entry
//    CMD_FLUSH       write back indicated entry and invalidate
//    CMD_FLUSH_ALL   walk the indexes, write back any dirty entries 
//    CMD_WBACK       force write back even if clean
//
// Timing
//    There is not a lot of timing related staging except the RAMs operate
//    with a single cycle of read latency, even main memory. 
//    A real design would use a waitstate handshake. I will reconsider this
//    once I have functional tests and coverage.
//
//    There is a read data valid from the cgcache for capture TB purposes
//
//    FIXME: this might have changed, re: eviction write back might be
//    visible to controller. Revisit this command when it works.
//
// Testbench
//    see src/top.v, see inc/tests.h and inc/utils.h
//
//    readmemh files are in ./input, used for sim initialization and some
//    golden reference files.
//
// Verilog structure is:
//
//    top
//      cache    dut0
//        tags
//        data array's
//        status bits
//        fsm
//        FIXME: add the rest
//      mm       main memory
// ----------------------------------------------------------------------
// LRU 
//
// 3 bits - initial condition is 000, 0 points left
//
//         b2
//       /    \
//     b1      b0
//   /   \   /    \
//  w3  w2   w1  w0
//
//    b2    either {w3,w2} or {w1,w0}
//    b1    w3 | w2
//    b0    w1 | w0
//
// LRU truth table
//
// access to way0    b2=0  b1=b1  b0=0
// access to way1    b2=0  b1=b1  b0=1  
// access to way2    b2=1  b1=0   b0=b0    
// access to way3    b2=1  b1=1   b0=b0
//
// Summary: starting from 000 (way3 is LRU), if way3 is accessed,
//    b2 is flipped to 1   - because comparing between {w3,w2} and {w1,w0}
//                           -- the {w1,w0} pair is least recently used
//                           -- the {w3,w2} pair is more  recently used
//    b1 is flipged to 1   - because comparing w3 and w2
//                           -- w2 is least recently used
//                           -- w3 is more recently used
//    b0 is not changed    - maintaining the history of access in the {w1,w0}
//                           pair
//
// Truth table sequence walk through
//
//   LRU state starts at 3'b000.
//   Only index 0 is accessed in this example
//   Ways are accessed accessed in this order: 3,1,2,0,3.
//
//                  previous
// read             b2  b1  b0  | next LRU   | next value
// -----------------------------|------------|-------------
// access way3      0   0   0   | 1   1  b0  |  1 1 0
// access way1      1   1   0   | 0  b1   1  |  0 1 1
// access way2      0   1   1   | 1   0  b0  |  1 0 1
// access way0      1   0   1   | 0  b1   0  |  0 0 0
// access way3      0   0   0   | 1   1  b0  |  1 1 0
//
// ...will result in the LRU at index 0 being 110 
//
// ----------------------------------------------------------------------
