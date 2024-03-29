// vi:syntax=verilog
`include "sim_cntrl.h"
`include "bitcmds.h"
`include "functions.h"
// --------------------------------------------------------------------------
// Write miss evict dirty way and allocate
//
// The LRU way is dirty, write it back, allocate and merge write data
// --------------------------------------------------------------------------
task basicWrEvictTest(inout int errs,inout flag,input int verbose);
integer i,j,mod,n,num;
int v,enb;
reg [2:0] lru_exp,lru_act;
reg [31:0] addr;
string state;
begin
  enb   = 1;
  state = "OFF";
  if(enb) state = "ON";

  beginTestMsg("basicWrEvictTest",errs,flag);
  if(verbose) $display("-I: CHECK ENABLE IS %s",state);

  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,v);

  @(posedge clk);
  //load main memory
  $readmemh("data/basicWrEvict.mm.memh",top.mm0.ram);
  //load data arrays
  $readmemh("data/basicWrEvict.d0.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicWrEvict.d1.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicWrEvict.d2.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicWrEvict.d3.memh",top.dut0.dsram3.ram);
  //load tags
  load_initial_tags("data/basicWrEvict.tags.memh",v);
  //load control bits
  load_initial_bits("data/basicWrEvict.bits.memb",v);

//  @(posedge clk);
//  top.dut0.valid0.regs[0] <= 4'b1110;
//  top.dut0.dirty0.regs[0] <= 4'b0111;
//  top.dut0.valid0.regs[10] <= 4'b1101;

  @(posedge clk);

  nop(1);
  n = 0;
  num = 3;
  // ======================================================================
  // #0
  // Access is to 0000 0000 0000 0000 0000 0000 0000 0000  -> 0x00000000 hex
  // Line addr is       000 0000 0000 0000 0000 0000 0000  ->  0x0000000 hex
  // Index addr                          0 0000 0000 0000  ->     0x0000 hex
  // Word  addr                                       011  ->        0x3 hex
  // Byte  enbs                                      1010 ->        1010 bin
  // Write data                                               0x11111111 hex
  // LRU bits are 010 -> lru way is 2
  //
  // Tag at index 0/way 2 is: 14'h0002 (14'b00 0000 0000 0010)
  // The data at index 0/way 2  is:
  // 00207000_00206000_00205000_00204000_00203000_00202000_00201000_00200000
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // tag 14b idx 13b lin  5b
  //
  // <----- tag ----->   <--- index  --->  <-0->
  // 00 0000 0000 0010   0 0000 0000 0000  00000
  // 0000 0000 0000 1000 0000 0000 0000 0000
  // 0000 0000 0000 1000 0000 0000 0000 0000 -> 00080000 byte address
  // 000 0000 0000 0100 0000 0000 0000       ->  0004000 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h0004000 should contain:
  // 00207000_00206000_00205000_00204000_00203000_00202000_00201000_00200000 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 0 way 2 should contain:
  //   00 0000 0000 0000  -> 0000 (from upper 14b of the access that missed)
  //
  // data at index 0 way 2 should contain (the contents of mm @0000000):
  //   F0007000_00006000_00005000_00004000_11001100_00002000_00001000_00000000 
  //
  // control bits at index 0 should be 
  //   val = 1111 (no change) 
  //   mod = 1111 (all are modified)
  //   lru =  100 (was 010, after read allocate to way2 becomes [1 0 b0])
  //a:0x0000000c #0
  // ======================================================================
  //      tag/way   index  wrd byt      be       data
  wr_req({14'h000,13'h000,3'h3,2'h0},4'b1010,32'h11111111,v);
  nop(n);
  // ======================================================================
  // #1
  // Access is to 0000 0000 0000 0100 0000 0000 0011 1100  -> 0x0004003c hex
  // Line addr is       000 0000 0000 0010 0000 0000 0001  ->  0x0002001 hex
  // Index addr                          0 0000 0000 0001  ->     0x0001 hex
  // Word  addr                                       111  ->        0x7 hex
  // Byte  enbs                                      1111 ->        1111 bin
  // LRU bits are 100 -> lru way is 1
  //
  // Tag at index 1/way 1 is: 14'h0003 (14'b00 0000 0000 0011)
  // The data at index 1/way 1  is:
  // 00107001_00106001_00105001_00104001_00103001_00102001_00101001_00100001 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // 00000000000011 0000000000001 00000
  // 0000 0000 0000 1100 0000 0000 0010 0000 -> 000c0020 byte address
  //  000 0000 0000 0110 0000 0000 0001      ->  0006001 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h06001  should contain:
  // 00107001_00106001_00105001_00104001_00103001_00102001_00101001_00100001 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 1 way 1 should contain:
  //   00 0000 0000 0001  -> 0001 (from upper 14b of the access that missed)
  //
  // data at index 1 way 1 should contain (the contents of mm @0002001):
  //   22222222_2000607f_2000507f_2000407f_2000307f_2000207f_2000107f_2000007f 
  //
  // control bits at index x should be 
  //   val = 1111 (no change) 
  //   mod = 1111 (no change)
  //   lru =  001 (was 100, after read allocate to way1 becomes [0 b1 1])
  //a:0x0004003c #1
  // ======================================================================

  wr_req({14'h001,13'h001,3'h7,2'h0},4'b1111,32'h22222222,v);
  nop(n);
  // ======================================================================
  // #2
  // Access is to 0000 0000 0000 1000 0000 0000 0101 1000  -> 0x00080058 hex
  // Line addr is       000 0000 0000 0100 0000 0000 0010  ->  0x0004002 hex
  // Index addr                          0 0000 0000 0010  ->     0x0002 hex
  // Word  addr                                       110  ->        0x6 hex
  // Byte  enbs                                      1100 ->        1100 bin
  // LRU bits are 000 -> lru way is 3
  //
  // Tag at index 2/way 3 is: 14'h0001 (14'b00 0000 0000 0001)
  // The data at index 2/way 3  is:
  // 00307002_00306002_00305002_00304002_00303002_00302002_00301002_00300002 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // 00000000000001 0000000000010 00000
  // 0000 0000 0000 0100 0000 0000 0100 0000 -> 00040040 byte address
  //       000 0000 0000 0010 0000 0000 0010 ->  0002002 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h02002 should contain:
  // 00307002_00306002_00305002_00304002_00303002_00302002_00301002_00300002 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 2 way 3 should contain:
  //   00 0000 0000 0010  -> 0002 (from upper 14b of the access that missed)
  //
  // data at index 2 way 3 should contain (the contents of mm @004002):
  //   4000707f_3232607f_4000507f_4000407f_4000307f_4000407f_4000107f_4000007f 
  //
  // control bits at index x should be 
  //   val = 1111 (no change) 
  //   mod = 1111 (no change)
  //   lru =  110 (was 000, after read allocate to way3 becomes [1 1  b0])
  //a:0x00080058 #2
  // ======================================================================
  wr_req({14'h002,13'h002,3'h6,2'h0},4'b1100,32'h23232323,v);
  nop(n);
  // Do these as needed after writing the cpp and SystemC model
  //  //a:00006003
  //  wr_req({14'h003,13'h003,3'h6,2'h0},4'b1111,32'h34353637,v);
  //  //a:00006004
  //  wr_req({14'h003,13'h004,3'h5,2'h0},4'b1111,32'h45464748,v);
  //  //a:00002005  way 1 index 5 be 1010
  //  wr_req({14'h001,13'h005,3'h1,2'h0},4'b1010,32'hFFFFFFFF,v);
  //  //a:00004006  way 2 index 6 be 0101
  //  wr_req({14'h002,13'h006,3'h5,2'h0},4'b0101,32'h77777777,v);
  //  //a:00006007
  //  wr_req({14'h003,13'h007,3'h3,2'h0},4'b1111,32'h98979695,v);
  //  //a:00002008
  //  wr_req({14'h001,13'h008,3'h2,2'h0},4'b1111,32'habacadae,v);
  //  //a:00000009
  //  wr_req({14'h000,13'h009,3'h1,2'h0},4'b1111,32'hbeefb0da,v);
  //  //a:0000200a
  //  wr_req({14'h001,13'h00a,3'h1,2'h0},4'b1111,32'hab109876,v);

  nop(5);
  //load expect main memory
  load_expect_main_memory("./golden/basicWrEvict.mm.memh",v);
  //load expect data arrays
  load_expect_dary_data("./golden/basicWrEvict.d0.memh",
                        "./golden/basicWrEvict.d1.memh",
                        "./golden/basicWrEvict.d2.memh",
                        "./golden/basicWrEvict.d3.memh",v);
  //load expect tags
  load_expect_tags("./golden/basicWrEvict.tags.memh",v);
  //load expect control bits
  load_expect_bits("./golden/basicWrEvict.bits.memb",v);

  nop(4); //let state propagate

  if(v) $display("BEGIN Main memory checks");
  check_main_memory (errs,0,16384,0);
  if(v) $display("END   Main memory checks");

  if(v) $display("BEGIN Data array checks");
  check_data_arrays (errs,0,num,v);
  if(v) $display("END   Data array checks");

  //check tags and bits
  if(v) $display("BEGIN Tag array checks");
  check_tb_tags_bits(errs,0,num,v);
  if(v) $display("END   Tag array checks");

  endTestMsg(testName,errs,flag);
  nop(4);
end
endtask
// --------------------------------------------------------------------------
// Read miss evict dirty way and allocate
//
// The LRU way is dirty, write it back, allocate and return critical word
// to PE
// 
// --------------------------------------------------------------------------
task basicRdEvictTest(inout int errs,inout flag,input int verbose);
integer i,j,mod,n;
int v,enb;
reg [2:0] lru_exp,lru_act;
reg [31:0] addr;
string state;
begin
  enb   = 1;
  state = "OFF";
  if(enb) state = "ON";

  beginTestMsg("basicRdEvictTest",errs,flag);
  if(verbose) $display("-I: CHECK ENABLE IS %s",state);

  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,v);

  @(posedge clk);
  if(verbose) $display("-I: setting initial configuration ");
  //load main memory
  $readmemh("data/basicRdEvict.mm.memh",top.mm0.ram);
  //load data arrays
  $readmemh("data/basicRdEvict.d0.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicRdEvict.d1.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicRdEvict.d2.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicRdEvict.d3.memh",top.dut0.dsram3.ram);
  //load tags
  load_initial_tags("data/basicRdEvict.tags.memh",v,1);
  //load control bits
  load_initial_bits("data/basicRdEvict.bits.memb",v);
  top.dut0.valid0.regs[0]  <= 4'b1111;
  top.dut0.dirty0.regs[0]  <= 4'b1111;
  top.dut0.lrurf0.regs[10]   <= 3'b010;
  @(posedge clk);
  n = 0;
  nop(n);
  // ======================================================================
  // #0
  //  This first access should detect a miss, evict modified entry pointed
  //  to by the LRU of index 0
  //
  // Access is to 0000 0000 0001 0000 0000 0000 0000 1100 -> 0010000C hex
  // Line addr is 000 0000 0000 1000 0000 0000 0000       ->  0008000 hex
  // Index addr   0 0000 0000 0000                        ->     0000 hex
  // Word  addr                011                        ->        3 hex
  // LRU bits are 010 -> lru way is 2
  //
  // Tag at index 0/way 2 is: 14'h0002 (14'b00000000000010)
  // The data at index 0/way 2  is:
  // 00207000_00206000_00205000_00204000_00203000_00202000_00201000_00200000
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  //
  // <--- tag ----> <-- index --> <-0->
  // 00000000000010 0000000000000 00000
  // 0000 0000 0000 1000 0000 0000 0000 0000 -> 00080000 byte address
  // 000 0000 0000 0100 0000 0000 0000       ->  0004000 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h04000 should contain:
  // 00207000_00206000_00205000_00204000_00203000_00202000_00201000_00200000
  //
  // ----------------------------------------------------------------------
  // After allocation
  // tag at index 0 way 2 should contain: 
  //   00 0000 0000 0100  -> 0004  (from upper 14b of the access that missed)
  //
  // data at index 0 way 2 should contain (the contents of mm @0x8000):
  //   8000707f_8000607f_8000507f_8000407f_8000307f_8000407f_8000107f_8000007f
  //
  // control bits at index 0 should be 
  //   val = 1111 (no change) 
  //   mod = 1011 (w2 no longer modified)
  //   lru =  100 (was 010, after read allocate to way2 becomes [1 0 b0])
  //
  // The captured data should be the 
  //     address 32'h0010000C
  //     data    32'h8000307F (word 3 of the return data)
  //
  //a:00008000 #0 FIXME: shouldnt this be 0x4000 ?
  // ======================================================================
  rd_req({14'h004,13'h000,3'h3,2'h0},4'b1111,v);//miss, victim is dirty
  nop(n);
  // ======================================================================
  // #1
  // Access is to 0000 0000 0000 0100 0000 0000 0011 1100 -> 0004003C hex
  //              0000 0000 0000 01 tag
  //                               00 0000 0000 001 index
  // Line addr is 000 0000 0000 0010 0000 0000 0001       ->  0002001 hex
  // Index addr   0 0000 0000 0001                        ->     0001 hex
  // Word  addr                111                        ->        7 hex
  // LRU bits are 100 -> lru way is 1
  //
  // Tag at index 1/way 1 is: 14'h0011 (14'b00 0000 0000 0011)
  // The data at index 1/way 1  is:
  // 00107001_00106001_00105001_00104001_00103001_00102001_00101001_00100001
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // 00000000000011 0000000000001 00000
  // 0000 0000 0000 1100 0000 0000 0010 0000 -> 00060020 byte address
  // 000 0000 0000 0110 0000 0000 0001       ->  0006001 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h06001 should contain:
  // 00107001_00106001_00105001_00104001_00103001_00102001_00101001_00100001
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 1 way 1 should contain:
  //   00 0000 0000 0001  -> 0001 (from upper 14b of the access that missed)
  //
  // data at index 1 way 1 should contain (the contents of mm @0x2001):
  //   2000707f_2000607f_2000507f_2000407f_2000307f_2000207f_2000107f_2000007f
  //
  // control bits at index 1 should be 
  //   val = 1111 (no change) 
  //   mod = 1101 (w1 no longer modified)
  //   lru =  001 (was 100, after read allocate to way1 becomes [0 b1 1])
  //
  // The captured data should be the 
  //     address 32'h0004003C
  //     data    32'h2000707f (word 7 of the return data)
  //
  //a:00002001 #1
  // ======================================================================
  rd_req({14'h001,13'h001,3'h7,2'h0},4'b1111,v);
  nop(n);
  // ======================================================================
  // #2
  // Access is to 0000 0000 0000 1000 0000 0000 0101 1000 -> 00080058 hex
  // Line addr is  000 0000 0000 0100 0000 0000 0010      ->  0004002 hex
  // Index addr   0 0000 0000 0010                        ->     0002 hex
  // Word  addr                110                        ->        6 hex
  // LRU bits are 000 -> lru way is 3
  //
  // Tag at index 2/way 3 is: 14'h0003 (14'b00 0000 0000 0011)
  // The data at index 2/way 3  is:
  // 00307002_00306002_00305002_00304002_00303002_00302002_00301002_00300002 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // 00000000000011 0000000000010 00000
  // 0000 0000 0000 1100 0000 0000 0100 0000 -> 080C0040 byte address
  // 000 0000 0000 0110 0000 0000 0010       ->  0006002 line address
  //
  // At the end of the access:     
  //
  // main memory at 26'h0006002 should contain:
  // 00307002_00306002_00305002_00304002_00303002_00302002_00301002_00300002_
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 2 way 3 should contain:
  // Access is to 00 0000 0000 0010
  //   00 0000 0000 0010  -> 0002 (from upper 14b of the access that missed)
  //
  // data at index 2 way 3 should contain (the contents of mm @0004002):
  //   4000707f_4000607f_4000507f_4000407f_4000307f_4000407f_4000107f_4000007f 
  //
  // control bits at index 2 should be 
  //   val = 1111 (no change) 
  //   mod = 0111 (w3 no longer modified)
  //   lru =  110 (was 000, after read allocate to way3 becomes [1 1 b0])
  //
  // The captured data should be the 
  //     address 32'h00080058
  //     data    32'h4000607f (word 6 of the return data)
  //
  //a:00004002
  // ======================================================================
  rd_req({14'h002,13'h002,3'h6,2'h0},4'b1111,v);
  nop(n);
  // ======================================================================
  // #3
  //              0000 0000 0000 1100 0000 0000 0111 1000
  // Access is to 0000 0000 0000 1100 0000 0000 0111 1000 -> 000C0078 hex
  // Line addr is  000 0000 0000 0110 0000 0000 0011      ->  0006003 hex
  // Index addr   0 0000 0000 0011                        ->     0003 hex
  // Word  addr                110                        ->        6 hex
  // LRU bits are 011 -> lru way is 2
  //
  // Tag at index 3/way 2 is: 14'h0004 (14'b00 0000 0000 0100)
  // The data at index 3/way 2  is:
  // 00207003_00206003_00205003_00204003_00203003_00202003_00201003_00200003 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ------->    <-- index ----->    <-0->
  // 00 0000 0000 0100    0 0000 0000 0011    00000
  // 0000 0000 0001 0000 0000 0000 0110 0000
  // 0000 0000 0001 0000 0000 0000 0110 0000 -> 00100060 byte address
  //  000 0000 0000 1000 0000 0000 0011       -> 00100060 byte address
  //  000 0000 0000 1000 0000 0000 0011       ->  0001003 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h1003 should contain:
  // 00207003_00206003_00205003_00204003_00203003_00202003_00201003_00200003 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 3 way 2 should contain:
  //   00 0000 0000 0011  -> 0003 (from upper 14b of the access that missed)
  //
  // data at index 3 way 2 should contain (the contents of mm @0006003):
  //   6000707f_6000607f_6000507f_6000407f_6000307f_6000407f_6000107f_6000007f 
  //
  // control bits at index x should be 
  //   val = 1111 (no change) 
  //   mod = 1011 (w2 no longer modified)
  //   lru =  101 (was 011, after read allocate to way2 becomes [1 0 b0])
  //
  // The captured data should be the 
  //     address 32'h000C0078
  //     data    32'h6000607f (word 6 of the return data)
  //
  //a:00006003 #3
  // ======================================================================
  rd_req({14'h003,13'h003,3'h6,2'h0},4'b1111,v);
  nop(n);
  // ======================================================================
  // #4
  // Access is to 0000 0000 0000 1100 0000 0000 1000 0000 -> 000c0094 hex
  // Line addr is       000 0000 0000 0110 0000 0000 0100 ->  0006004 hex
  // Index addr   0 0000 0000 0100                        ->     0004 hex
  // Word  addr                101                        ->        5 hex
  // LRU bits are 001 -> lru way is 3
  //
  // Tag at index 4/way 3 is: 14'hxxxx (14'b00 0000 0000 0111)
  // The data at index 4/way 3  is:
  // 03307004_03306004_03305004_03304004_03303004_03302004_03301004_03300004 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // 00000000000111 0000000000100 00000
  // 0000 0000 0001 1100 0000 0000 1000 0000 -> 001c0080 byte address
  // 000 0000 0000 1110 0000 0000 0100       ->  000E004 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h0E004 should contain:
  // 03307004_03306004_03305004_03304004_03303004_03302004_03301004_03300004
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 4 way 3 should contain:
  //   00 0000 0000 0011  -> 0003 (from upper 14b of the access that missed)
  //
  // data at index 4 way 3 should contain (the contents of mm @06004):
  //   6400707f_6400607f_6400507f_6400407f_6400307f_6400407f_6400107f_6400007f 
  //
  // control bits at index x should be 
  //   val = 1111 (no change) 
  //   mod = 0111 (w3 no longer modified)
  //   lru =  111 (was 001, after read allocate to way3 becomes [1 1 b0])
  //
  // The captured data should be the 
  //     address 32'h000c0094
  //     data    32'h6400507f (word 5 of the return data)
  //
  //a:00006004
  // ======================================================================
  rd_req({14'h003,13'h004,3'h5,2'h0},4'b1111,v);
  nop(n);
  // ======================================================================
  // #5
  // Access is to 0000 0000 0000 0100 0000 0000 1010 0100 -> 000400a4 hex
  // Line addr is 000 0000 0000 0010 0000 0000 0101       ->  0002005 hex
  // Index addr   0 0000 0000 0101                        ->     0005 hex
  // Word  addr                001                        ->        1 hex
  // LRU bits are 101 -> lru way is 0
  //
  // Tag at index 5/way 0 is: 14'h0006 (14'b00 0000 0000 0110)
  // The data at index 5/way 0  is:
  // 00007005_00006005_00005005_00004005_00003005_00002005_00001005_00000005 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // 00000000000011 0000000000101 00000
  // 0000 0000 0000 1100 0000 0000 1010 0000 -> 000c00a0 byte address
  // 000 0000 0000 0110 0000 0000 0101       ->  0006005 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h6005 should contain:
  // 00007005_00006005_00005005_00004005_00003005_00002005_00001005_00000005_
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 5 way 0 should contain:
  //   00 0000 0000 0001  -> 0001 (from upper 14b of the access that missed)
  //
  // data at index x way x should contain (the contents of mm @02005):
  //   2500707f_2500607f_2500507f_2500407f_2500307f_2500407f_2500107f_2500007f 
  //
  // control bits at index x should be 
  //   val = 1111 (no change) 
  //   mod = 1110 (wx no longer modified)
  //   lru =  000 (was 101, after read allocate to way0 becomes [0 b1 0])
  //
  // The captured data should be the 
  //     address 32'h000400a4
  //     data    32'h2500107f (word 1 of the return data)
  //
  //a:00002005 #5
  // ======================================================================
  rd_req({14'h001,13'h005,3'h1,2'h0},4'b1111,v);
  nop(n);
  // ======================================================================
  // #6
  // Access is to 0000 0000 0000 1000 0000 0000 1101 0100 -> 000800d4 hex
  // Line addr is 000 0000 0000 0100 0000 0000 0110       ->  0004006 hex
  // Index addr   0 0000 0000 0110                        ->     0006 hex
  // Word  addr                101                        ->        5 hex
  // LRU bits are 010 -> lru way is 2
  //
  // Tag at index 6/way 2 is: 14'hxxxx (14'b00 0000 0000 0111)
  // The data at index 6/way 2  is:
  // 00207006_00206006_00205006_00204006_00203006_00202006_00201006_00200006 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // 00000000000111 0000000000101 00000
  // 0000 0000 0001 1100 0000 0000 1100 0000 -> 001c00c0 byte address
  // 000 0000 0000 1110 0000 0000 0110       ->  000e006 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h000e006 should contain:
  // 00207006_00206006_00205006_00204006_00203006_00202006_00201006_00200006 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 6 way 2 should contain:
  //   00 0000 0000 0010  -> 0002 (from upper 14b of the access that missed)
  //
  // data at index 6 way 2 should contain (the contents of mm @0004006):
  //   4600707f_4600607f_4600507f_4600467f_4600307f_4600467f_4600107f_4600007f 
  //
  // control bits at index 6 should be 
  //   val = 1111 (no change) 
  //   mod = 1011 (wx no longer modified)
  //   lru =  100 (was 010, after read allocate to way2 becomes [1 0 b0])
  //
  // The captured data should be the 
  //     address 32'h000800d4
  //     data    32'h4600507f (word 5 of the return data)
  //
  //a:00004006 #6
  // ======================================================================
  rd_req({14'h002,13'h006,3'h5,2'h0},4'b1111,v);
  nop(n);
  // ======================================================================
  // #7
  // Access is to 0000 0000 0000 1100 0000 0000 1110 1100 -> 000c00ec hex
  // Line addr is 000 0000 0000 0110 0000 0000 0111       ->  0006007 hex
  // Index addr   0 0000 0000 0111                        ->     0007 hex
  // Word  addr                011                        ->        3 hex
  // LRU bits are 001 -> lru way is 3
  //
  // Tag at index 7/way 3 is: 14'h0008 (14'b00 0000 0000 1000 )
  // The data at index 7/way 3  is:
  // 00307007_00306007_00305007_00304007_00303007_00302007_00301007_00300007 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // xxxxxxxxxxxxxx xxxxxxxxxxxxx xxxxx
  // 0000 0000 0010 0000 0000 0000 1110 1100 -> 002000ec byte address
  // 000 0000 0001 0000 0000 0000 0111       ->  0010007 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h0010007 should contain:
  // 00307007_00306007_00305007_00304007_00303007_00302007_00301007_00300007 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 7 way 3 should contain:
  //   00 0000 0000 1000  -> 0008 (from upper 14b of the access that missed)
  //
  // data at index 7 way 3 should contain (the contents of mm @0006007):
  //   6700707f_6700607f_6700507f_6700407f_6700307f_6700407f_6700107f_6700007f 
  //
  // control bits at index x should be 
  //   val = 1111 (no change) 
  //   mod = 0111 (w3 no longer modified)
  //   lru =  111 (was 011, after read allocate to way3 becomes [1 1 b0])
  //
  // The captured data should be the 
  //     address 32'h000c00ec
  //     data    32'h00303007 (word 3 of the return data)
  //
  //a:00006007 #7
  // ======================================================================
  rd_req({14'h003,13'h007,3'h3,2'h0},4'b1111,v);
  nop(n);
  // ======================================================================
  // #8
  // Access is to 0000 0000 0000 0100 0000 0001 0000 1000 -> 00040108 hex
  // Line addr is 000 0000 0000 0010 0000 0000 1000       ->  0002008 hex
  // Index addr   0 0000 0000 1000                        ->     xxxx hex
  // Word  addr                010                        ->        x hex
  // LRU bits are 111 -> lru way is 0
  //
  // Tag at index 8/way 0 is: 14'h0009 (14'b00 0000 0000 1001)
  // The data at index 8/way 0  is:
  // 00007008_00006008_00005008_00004008_00003008_00002008_00001008_00000008 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // xxxxxxxxxxxxxx xxxxxxxxxxxxx xxxxx
  // 0000 0000 0010 0100 0000 0001 0000 0000 -> 00240100 byte address
  // 000 0000 0001 0010 0000 0000 1000       ->  0012008 line address
  //
  // At the end of the access:
  //
  // main memory at 26'hxxxxx should contain:
  // 00007008_00006008_00005008_00004008_00003008_00002008_00001008_00000008 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 8 way 0 should contain:
  //   00 0000 0000 0001  -> 0001 (from upper 14b of the access that missed)
  //
  // data at index 8 way 0 should contain (the contents of mm @0002008):
  //   2800707f_2800607f_2800507f_2800407f_2800307f_2800407f_2800107f_2800007f 
  //
  // control bits at index 8 should be 
  //   val = 1111 (no change) 
  //   mod = 1110 (wx no longer modified)
  //   lru =  010 (was 111, after read allocate to way0 becomes [0 b1 0])
  //
  // The captured data should be the 
  //     address 32'h00040108
  //     data    32'h2800407f (word 2 of the return data)
  //
  //a:00002008 #8
  // ======================================================================
  rd_req({14'h001,13'h008,3'h2,2'h0},4'b1111,v);
  nop(n);
  // ======================================================================
  // #9
  // Access is to 0000 0000 0000 0000 0000 0001 0010 0100 -> 00000124 hex
  // Line addr is 000 0000 0000 0000 0000 0000 1001       ->  0000009 hex
  // Index addr   0 0000 0000 1001                        ->     0009 hex
  // Word  addr                001                        ->        1 hex
  // LRU bits are 101 -> lru way is 0
  //
  // Tag at index 9/way 0 is: 14'h0003 (14'b00 0000 0000 0011)
  // The data at index 9/way 0  is:
  // 00007009_00006009_00005009_00004009_00003009_00002009_00001009_00000009 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // xxxxxxxxxxxxxx xxxxxxxxxxxxx xxxxx
  // 0000 0000 0000 1100 0000 0001 0010 0000 -> 0x000c0120 byte address
  // 000 0000 0000 0110 0000 0000 1001       ->  0x0006009 line address
  //
  // At the end of the access:
  //
  // main memory at 26'h0006009 should contain:
  // 00007009_00006009_00005009_00004009_00003009_00002009_00001009_00000009 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 9 way 0 should contain:
  //   00 0000 0000 0000  -> 0000 (from upper 14b of the access that missed)
  //
  // data at index 9 way 0 should contain (the contents of mm @0000009):
  //   F0007009_00006009_00005009_00004009_00003009_00002009_00001009_00000009 
  //
  // control bits at index 9 should be 
  //   val = 1111 (no change) 
  //   mod = 1110 (w0 no longer modified)
  //   lru =  000 (was 101, after read allocate to way0 becomes [0 b1 0])
  //
  // The captured data should be the 
  //     address 32'h00000124
  //     data    32'h00001009 (word 1 of the return data)
  //
  //a:00000009 #9
  // ======================================================================
  rd_req({14'h000,13'h009,3'h1,2'h0},4'b1111,v);
  nop(n);
  // ======================================================================
  // #10
  // Access is to 0000 0000 0000 0100 0000 0001 0100 0100 -> 00040144 hex
  // Line addr is 000 0000 0000 0010 0000 0000 1010       ->  000200a hex
  // Index addr   0 0000 0000 1010                        ->     000a hex
  // Word  addr                001                        ->        1 hex
  // LRU bits are 010 -> lru way is 2
  //
  // Tag at index 10/way 2 is: 14'h000e (14'b00 0000 0000 1110)
  // The data at index 10/way 2  is:
  // 0020700a_0020600a_0020500a_0020400a_0020300a_0020200a_0020100a_0020000a 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // xxxxxxxxxxxxxx xxxxxxxxxxxxx xxxxx
  // 0000 0000 0011 1000 0000 0001 0100 0000 -> 00380140 byte address
  // 000 0000 0001 1100 0000 0000 1010       ->  001c00a line address
  //
  // At the end of the access:
  //
  // main memory at 26'h001c00a (writeback line address) should contain:
  // 0020700a_0020600a_0020500a_0020400a_0020300a_0020200a_0020100a_0020000a 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index 10 way 2 should contain:
  //   00 0000 0000 0001  -> 0001 (from upper 14b of the access that missed)
  //
  // data at index 10 way 2 should contain (the contents of mm @000200a):
  //   2a00707f_2a00607f_2a00507f_2a00407f_2a00307f_2a00407f_2a00107f_2a00007f 
  //
  // control bits at index 10 should be 
  //   val = 1111 (no change) 
  //   mod = 1011 (w2 no longer modified)
  //   lru =  100 (was 010, after read allocate to way2 becomes [1 0 b0])
  //
  // The captured data should be the 
  //     address 32'h00040144
  //     data    32'h2a00107f (word 1 of the return data)
  //
  //a:0000200a #10
  // ======================================================================
  rd_req({14'h001,13'h00a,3'h1,2'h0},4'b1111,v);
  nop(1);

  //load expect main memory
  load_expect_main_memory("./golden/basicRdEvict.mm.memh",v);
  //load expect data arrays
  load_expect_dary_data("./golden/basicRdEvict.d0.memh",
                        "./golden/basicRdEvict.d1.memh",
                        "./golden/basicRdEvict.d2.memh",
                        "./golden/basicRdEvict.d3.memh",v);
  //load expect tags
  load_expect_tags("./golden/basicRdEvict.tags.memh",v);
  //load expect control bits
  load_expect_bits("./golden/basicRdEvict.bits.memb",v);
  //load expect tb capture info
  load_expect_capture_data("./golden/basicRdEvict.capa.memh",
                           "./golden/basicRdEvict.capd.memh",v);

  nop(4); //let state propagate

  if(v) $display("BEGIN Main memory checks");
  check_main_memory (errs,0,16384,0);
  if(v) $display("END   Main memory checks");

  if(v) $display("BEGIN Data array checks");
  check_data_arrays (errs,0,11,v);
  if(v) $display("END   Data array checks");

  if(v) $display("BEGIN Tag array checks");
  check_tb_tags_bits(errs,0,11,v);
  if(v) $display("END   Tag array checks");

  if(v) $display("BEGIN Capture checks");
  check_tb_capture_info (errs,0,11,v); //only 11
  if(v) $display("END   Capture checks");

  endTestMsg(testName,errs,flag);
  nop(4);
end
endtask
// --------------------------------------------------------------------------
// Write miss allocate to invalid way
//
// Writes to invalid ways should allocate from main memory.
// The word in the retreived line is updated in the dary
//
// LRU bits are initially arbitrarily set
// Mod bits are initially arbitrarily set
// --------------------------------------------------------------------------
task basicWrAllocTest(inout int errs,inout flag,input int verbose);
integer i,j,mod;
int v,enb;
reg [2:0] lru_exp,lru_act;
reg [31:0] addr;
string state;
begin
  enb   = 1;
  state = "OFF";
  if(enb) state = "ON";

  beginTestMsg("basicWrAllocTest",errs,flag);
  if(verbose) $display("-I: CHECK ENABLE IS %s",state);

  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,v);

  @(posedge clk);
  if(verbose) $display("-I: setting initial configuration ");
  //load main memory
  $readmemh("data/basicWrAlloc.mm.memh",top.mm0.ram);
  //load data arrays
  $readmemh("data/basicWrAlloc.d0.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicWrAlloc.d1.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicWrAlloc.d2.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicWrAlloc.d3.memh",top.dut0.dsram3.ram);
  //load tags
  load_initial_tags("data/basicWrAlloc.tags.memh",v);
  //load control bits
  load_initial_bits("data/basicWrAlloc.bits.memb",v);
  top.dut0.valid0.regs[0] <= 4'b1110;
  top.dut0.dirty0.regs[0] <= 4'b0111;
  top.dut0.valid0.regs[10] <= 4'b1101;
  @(posedge clk);
  nop(1);

          //tag/way index   word
  //a:00000000
  wr_req({14'h000,13'h000,3'h3,2'h0},4'b1111,32'h11111111,v);
  //        way   index    tag      val     mod    lru
  chk_alloc(2'h0,13'h000,14'h000,4'b1111,4'b0111,3'b010,enb,errs,v);

  //a:00002001
  wr_req({14'h001,13'h001,3'h7,2'h0},4'b1111,32'h22222222,v);
  chk_alloc(2'h1,13'h001,14'h001,4'b1111,4'b0110,3'b001,enb,errs,v);

  //a:00004002
  wr_req({14'h002,13'h002,3'h6,2'h0},4'b1111,32'h23232323,v);
  chk_alloc(2'h2,13'h002,14'h002,4'b1111,4'b0110,3'b100,enb,errs,v);

  //a:00006003
  wr_req({14'h003,13'h003,3'h6,2'h0},4'b1111,32'h34353637,v);
  chk_alloc(2'h3,13'h003,14'h003,4'b1111,4'b1111,3'b111,enb,errs,v);

  //a:00006004
  wr_req({14'h003,13'h004,3'h5,2'h0},4'b1111,32'h45464748,v);
  chk_alloc(2'h3,13'h004,14'h003,4'b1111,4'b1111,3'b110,enb,errs,v);

  //a:00002005  way 1 index 5 be 1010
  wr_req({14'h001,13'h005,3'h1,2'h0},4'b1010,32'hFFFFFFFF,v);
  chk_alloc(2'h1,13'h005,14'h001,4'b1111,4'b0010,3'b001,enb,errs,v);

  //a:00004006  way 2 index 6 be 0101
  wr_req({14'h002,13'h006,3'h5,2'h0},4'b0101,32'h77777777,v);
  chk_alloc(2'h2,13'h006,14'h002,4'b1100,4'b0100,3'b100,enb,errs,v);

  //a:00006007
  wr_req({14'h003,13'h007,3'h3,2'h0},4'b1111,32'h98979695,v);
  chk_alloc(2'h3,13'h007,14'h003,4'b1101,4'b1110,3'b111,enb,errs,v);

  //a:00002008
  wr_req({14'h001,13'h008,3'h2,2'h0},4'b1111,32'habacadae,v);
  chk_alloc(2'h1,13'h008,14'h001,4'b1110,4'b0010,3'b011,enb,errs,v);

  //a:00000009
  wr_req({14'h000,13'h009,3'h1,2'h0},4'b1111,32'hbeefb0da,v);
  chk_alloc(2'h0,13'h009,14'h000,4'b1111,4'b1001,3'b000,enb,errs,v);

  //a:0000200a
  wr_req({14'h001,13'h00a,3'h1,2'h0},4'b1111,32'hab109876,v);
  chk_alloc(2'h1,13'h00a,14'h001,4'b1111,4'b1111,3'b011,enb,errs,v);

  //load expect main memory
  load_expect_main_memory("./golden/basicWrAlloc.mm.memh",v);
  //load expect data arrays
  load_expect_dary_data("./golden/basicWrAlloc.d0.memh",
                        "./golden/basicWrAlloc.d1.memh",
                        "./golden/basicWrAlloc.d2.memh",
                        "./golden/basicWrAlloc.d3.memh",v);
  //load expect tags
  load_expect_tags("./golden/basicWrAlloc.tags.memh",v);
  //load expect control bits
  load_expect_bits("./golden/basicWrAlloc.bits.memb",v);

  nop(4); //let state propagate

  check_main_memory (errs,0,15,v);
  check_data_arrays (errs,0,15,v);
  //check tags and bits
  check_tb_tags_bits(errs,0,15,v);

  endTestMsg(testName,errs,flag);
  nop(4);
end
endtask
// --------------------------------------------------------------------------
// Read miss allocate to invalid way
//
// Reads to invalid ways should allocate from main memory and return 
// the critical word in parallel. 
//
// LRU bits are initially arbitrarily set
// Mod bits are initially arbitrarily set
// --------------------------------------------------------------------------
task basicRdAllocTest(inout int errs,inout flag,input int verbose);
integer i,j,mod;
int v,enb;
reg [2:0] lru_exp,lru_act;
reg [31:0] addr;
string state;
begin
  enb   = 0;
  state = "OFF";
  if(enb) state = "ON";

  beginTestMsg("basicRdAllocTest",errs,flag);
  if(verbose) $display("-I: CHECK ENABLE IS %s",state);

  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,v);

  @(posedge clk);
  if(verbose) $display("-I: setting initial configuration ");
  //load main memory
  $readmemh("data/basicRdAlloc.mm.memh",top.mm0.ram);
  //load data arrays
  $readmemh("data/basicRdAlloc.d0.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicRdAlloc.d1.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicRdAlloc.d2.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicRdAlloc.d3.memh",top.dut0.dsram3.ram);
  //load tags
  load_initial_tags("data/basicRdAlloc.tags.memh",v);
  //load control bits
  load_initial_bits("data/basicRdAlloc.bits.memb",v);
  top.dut0.valid0.regs[0] <= 4'b1110;
  top.dut0.dirty0.regs[0] <= 4'b0111;
  @(posedge clk);
  nop(1);

          //tag/way index   word
  //a:00000000
  rd_req({14'h000,13'h000,3'h3,2'h0},4'b1111,v);//miss
  //        way   index    tag      val     mod    lru
  chk_alloc(2'h0,13'h000,14'h000,4'b1111,4'b0110,3'b010,enb,errs,v);

  //a:00002001
  rd_req({14'h001,13'h001,3'h7,2'h0},4'b1111,v);
  chk_alloc(2'h1,13'h001,14'h001,4'b1111,4'b0100,3'b001,enb,errs,v);

  //a:00004002
  rd_req({14'h002,13'h002,3'h6,2'h0},4'b1111,v);
  chk_alloc(2'h2,13'h002,14'h002,4'b1111,4'b0010,3'b100,enb,errs,v);

  //a:00006003
  rd_req({14'h003,13'h003,3'h6,2'h0},4'b1111,v);
  chk_alloc(2'h3,13'h003,14'h003,4'b1111,4'b0111,3'b111,enb,errs,v);

  //a:00006004
  rd_req({14'h003,13'h004,3'h5,2'h0},4'b1111,v);
  chk_alloc(2'h3,13'h004,14'h003,4'b1111,4'b0111,3'b110,enb,errs,v);

  //a:00002005
  rd_req({14'h001,13'h005,3'h1,2'h0},4'b1111,v);
  chk_alloc(2'h1,13'h005,14'h001,4'b1111,4'b0000,3'b001,enb,errs,v);

  //a:00004006
  rd_req({14'h002,13'h006,3'h5,2'h0},4'b1111,v);
  chk_alloc(2'h2,13'h006,14'h002,4'b1100,4'b0000,3'b100,enb,errs,v);

  //a:00006007
  rd_req({14'h003,13'h007,3'h3,2'h0},4'b1111,v);
  chk_alloc(2'h3,13'h007,14'h003,4'b1101,4'b0110,3'b111,enb,errs,v);

  //a:00002008
  rd_req({14'h001,13'h008,3'h2,2'h0},4'b1111,v);
  chk_alloc(2'h1,13'h008,14'h001,4'b1110,4'b0000,3'b011,enb,errs,v);

  //a:00000009
  rd_req({14'h000,13'h009,3'h1,2'h0},4'b1111,v);
  chk_alloc(2'h0,13'h009,14'h000,4'b1111,4'b1000,3'b000,enb,errs,v);

  //a:0000200a
  rd_req({14'h001,13'h00a,3'h1,2'h0},4'b1111,v);
  chk_alloc(2'h1,13'h00a,14'h001,4'b1111,4'b1101,3'b011,enb,errs,v);

  //load expect main memory
  load_expect_main_memory("./golden/basicRdAlloc.mm.memh",v);
  //load expect data arrays
  load_expect_dary_data("./golden/basicRdAlloc.d0.memh",
                        "./golden/basicRdAlloc.d1.memh",
                        "./golden/basicRdAlloc.d2.memh",
                        "./golden/basicRdAlloc.d3.memh",v);
  //load expect tags
  load_expect_tags("./golden/basicRdAlloc.tags.memh",v);
  //load expect control bits
  load_expect_bits("./golden/basicRdAlloc.bits.memb",v);
  //load expect tb capture info
  load_expect_capture_data("./golden/basicRdAlloc.capa.memh",
                           "./golden/basicRdAlloc.capd.memh",v);

  nop(4); //let state propagate

  check_main_memory (errs,0,15,v);
  check_data_arrays (errs,0,4,v);
  //check tags and bits
  check_tb_tags_bits(errs,0,15,v);
  //check capture address and data
  check_tb_capture_info (errs,0,11,v); //only 11

  endTestMsg(testName,errs,flag);
  nop(4);
end
endtask
// --------------------------------------------------------------------------
// LRU rules - see README.txt
//
// Example
// LRU state starts at 3'b000.
// All accesses are to index 0 w/ ways, accessed in this order: 3,1,2,0,3.
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
// arrays have been prefilled with sequential data/addresses
// so way order in this test is set by last two bits of tag @index 0
//
// This solely an LRU test. It does read/write requests but does not 
// check data only LRU bits, see basicRdHit, and basicWrHit tests for that.
// --------------------------------------------------------------------------
task basicLruTest(inout int errs,inout flag,input int verbose);
int i;
reg [1:0]  _byte;
reg [2:0]  word;
reg [12:0] index;

reg [31:0] addr;
reg [31:0] incr;
reg [2:0]  act_lru,exp_lru;
string pfx;
integer n;
begin
  beginTestMsg("basicLruTest",errs,flag);
  clear_tb_data(0,EXP_DATA_ENTRIES,verbose);

  if(verbose) $display("-I: setting initial configuration ");
  $readmemh("data/basicLru.d0.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicLru.d1.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicLru.d2.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicLru.d3.memh",top.dut0.dsram3.ram);

  load_initial_tags("data/basicLru.tags.memh",verbose);
  load_initial_bits("data/basicLru.bits.memb",verbose);

  nop(1);

  _byte = 2'b00;
  word  = 3'b000;
  index = 13'h000;

  //some arbitrary nops inserted, I found a case where back to back
  //worked and yet failed when i added a couple nops. good enough for
  //this quick basic test. random testing will fully test this.

  n = 0;
  //(read) access way 3 of index 0    plru 000 -> 110
  addr  = {14'h003,index,word,_byte};
  rd_req(addr,4'b1111,verbose);
  nop(n);

  //(write) access way 1 of index 0    plru 110 -> 011
  addr  = {14'h001,index,word,_byte};
  wr_req(addr,4'b1111,32'h11111111,verbose);
  nop(n);

  //(read) access way 2 of index 0     plru 011 -> 101
  addr  = {14'h002,index,word,_byte};
  rd_req(addr,4'b1111,verbose);
  nop(n);

  //(write) access way 0 of index 0    plru 101 -> 000
  addr  = {14'h000,index,word,_byte};
  wr_req(addr,4'b1111,32'h22222222,verbose);
  nop(n);

  //(read) access way 2 of index 0     plru 000 -> 100
  addr  = {14'h002,index,word,_byte};
  rd_req(addr,4'b1111,verbose);

  //let final state propagate
  nop(3);

  //Manually verify the resulting LRU bits
  //grab the contents of index 0 in the LRU array
  act_lru = top.dut0.lrurf0.regs[0];
  exp_lru = 3'b100;

  pfx = "-I:";
  if(act_lru !== exp_lru) begin
    pfx = "-E:";
    errs += 1;
  end

  if(errs > 0 || verbose) begin
    $display("%0s basicLruTest : exp:%03b  act:%03b",pfx,exp_lru,act_lru);
  end

  endTestMsg(testName,errs,flag);
  nop(1);
end
endtask
// --------------------------------------------------------------------------
// Read hit test
//
// directed walk of words within the indexes, way = tag due to initialzing
// data
//
// initial lru state is 000
// See golden/basicRdHit.bits.memb for derivation of expect data
// and README.txt for the LRU update truth table
//
// --------------------------------------------------------------------------
task basicRdHitTest(inout int errs,inout flag,input int verbose);
integer i,j,mod;
int v;
reg [2:0] lru_exp,lru_act;
reg [31:0] addr;

begin
  beginTestMsg("basicRdHitTest",errs,flag);
  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,v);

  nop(1);

  if(verbose) $display("-I: setting initial configuration ");
  $readmemh("data/basicRdHit.d0.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicRdHit.d1.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicRdHit.d2.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicRdHit.d3.memh",top.dut0.dsram3.ram);

  load_initial_tags("data/basicRdHit.tags.memh",v);
  load_initial_bits("data/basicRdHit.bits.memb",v);

  nop(4);

  //FIXME: there is a problem somewhere, where the readmemh calls
  //above do not reset index 0 of the lru/dirty bits. I have not found
  //the problem yet.
  @(posedge clk);
  top.dut0.lrurf0.regs[0] = 3'b0;
  top.dut0.dirty0.regs[0] = 4'b0;
  @(posedge clk);


  //some arbitrary nops inserted just for simple state test

          //tag/way index   word                  --wpWpii
  rd_req({14'h003,13'h000,3'h3,2'h0},4'b1111,v);//00303000
  rd_req({14'h001,13'h001,3'h7,2'h0},4'b1111,v);//00107001
  nop(4);
  rd_req({14'h002,13'h002,3'h6,2'h0},4'b1111,v);//00206002
  nop(1);
  rd_req({14'h000,13'h003,3'h5,2'h0},4'b1111,v);//00005003
  rd_req({14'h000,13'h004,3'h2,2'h0},4'b1111,v);//00002004
  nop(4);
  rd_req({14'h001,13'h001,3'h1,2'h0},4'b1111,v);//00101001
  rd_req({14'h002,13'h005,3'h5,2'h0},4'b1111,v);//00205005
  rd_req({14'h003,13'h007,3'h3,2'h0},4'b1111,v);//00303007
  nop(2);
  rd_req({14'h000,13'h002,3'h2,2'h0},4'b1111,v);//00002002
  rd_req({14'h001,13'h003,3'h1,2'h0},4'b1111,v);//00101003
  rd_req({14'h000,13'h003,3'h6,2'h0},4'b1111,v);//00006003
  nop(4);
  rd_req({14'h001,13'h003,3'h7,2'h0},4'b1111,v);//00107003
  rd_req({14'h003,13'h006,3'h4,2'h0},4'b1111,v);//00304006
  rd_req({14'h000,13'h006,3'h3,2'h0},4'b1111,v);//00003006
  nop(1);
  rd_req({14'h000,13'h002,3'h0,2'h0},4'b1111,v);//00000002
  rd_req({14'h001,13'h000,3'h1,2'h0},4'b1111,v);//00101000

  nop(4); //let state propagate

  load_expect_capture_data("./golden/basicRdHit.addr.memh",
                           "./golden/basicRdHit.data.memh",v);
  load_expect_tags("./golden/basicRdHit.tags.memh",v);
  load_expect_bits("./golden/basicRdHit.bits.memb",v); //NOTE B file

  nop(4); //let state propagate

  check_tb_capture_info (errs,0,16,v); //EXP_DATA_ENTRIES);
  check_tb_tags_bits(errs,0,16,v); //EXP_DATA_ENTRIES);

  endTestMsg(testName,errs,flag);

  //$display("HERE basicRdHitTest");
  //show_line(0,0);
  nop(1);
end
endtask
// --------------------------------------------------------------------------
// Write hit test - uses same setup and pattern as read hit test
//
// directed walk of words within the indexes, way = tag due to initialzing
// data
//
// way/tag index   word
//  3        0       3
//  1        1       7
//  2        2       6
//  0        3       5
//  0        4       2
//  1        1       1
//  2        5       5
//  3        7       3
//  0        2       2
//  1        3       1
//  0        3       6
//  1        3       7
//  3        6       4
//  0        6       3
//  0        2       0
//  1        0       1
//
// --------------------------------------------------------------------------
task basicWrHitTest(inout int errs,inout flag,input int verbose);
integer i,j,mod;
reg [1:0]  _byte;
reg [2:0]  word;
reg [12:0] index;
reg [13:0] tag;

reg [31:0] addr;
reg [31:0] incr;
reg [2:0]  act_lru,exp_lru;
int check;
reg v;
begin
  beginTestMsg("basicWrHitTest",errs,flag);
  v = verbose;

  clear_tb_data(0,EXP_DATA_ENTRIES,v);

  if(verbose) $display("-I: setting initial configuration ");

  $readmemh("data/basicWrHit.dN.memh",top.dut0.dsram0.ram);
  $readmemh("data/basicWrHit.dN.memh",top.dut0.dsram1.ram);
  $readmemh("data/basicWrHit.dN.memh",top.dut0.dsram2.ram);
  $readmemh("data/basicWrHit.dN.memh",top.dut0.dsram3.ram);

  load_initial_tags("data/basicWrHit.tags.memh",v);
  load_initial_bits("data/basicWrHit.bits.memb",v);

  nop(1);

  //FIXME: I have not found why the readmemh's above are not working.
  //I see left over state from the previous read test. Uncomment to
  //see what I mean
  //  $display("HERE basicWrHit before explicit re-init 1");
  //show_line(0,0);
  //show_line(1,0);
  //show_line(2,0);
  //show_line(3,0);
  @(posedge clk);
  top.dut0.dsram0.ram[0] = 256'b0;
  top.dut0.dsram1.ram[0] = 256'b0;
  top.dut0.dsram2.ram[0] = 256'b0;
  top.dut0.dsram3.ram[0] = 256'b0;
  @(posedge clk);
  //  $display("HERE basicWrHit after explicit re-init 1");
  //show_line(0,0);
  //show_line(1,0);
  //show_line(2,0);
  //show_line(3,0);

//  way  index word be      value
// --------------------------------
//  0,   0,    0,   1111,   11111111 done
//  0,   0,    1,   0001,   00000022 done
//  0,   0,    2,   0010,   00003300 done
//  0,   0,    3,   0100,   00440000 done
//  0,   0,    4,   1000,   55000000 done
//
//  3,   0,    3,   0011,   22221111 done
//  1,   1,    7,   1100,   3333xxxx done
//  2,   2,    6,   0001,   xxxxxx44 done
//  0,   3,    5,   0100,   xx55xxxx done
//  0,   4,    2,   0010,   xxxx66xx done
//  1,   1,    1,   1000,   77xxxxxx done
//  2,   5,    5,   1111,   88888888 done
//  3,   7,    3,   1111,   99999999 done
//  0,   2,    2,   1111,   aaaaaaaa done
//  1,   3,    1,   1111,   bbbbbbbb done
//  0,   3,    6,   1111,   cccccccc done
//  1,   3,    7,   1111,   dddddddd done
//  3,   6,    4,   1111,   eeeeeeee done
//  0,   6,    3,   1111,   ffffffff done
//  0,   2,    0,   1111,   01020304 done
//  1,   0,    1,   1111,   50607080 done
//
//00000000_00000000_00000000_00000000_00000000_00000000_00000000_01111111
//00000000_00000000_00000000_00000000_00000000_00000000_00000022_01111111
//00000000_00000000_00000000_00000000_00000000_00003300_00000022_01111111
//00000000_00000000_00000000_00000000_00440000_00003300_00000022_01111111
//00000000_00000000_00000000_05000000_00440000_00003300_00000022_01111111
//00000000_00000000_0CACACAC_05000000_00440000_00003300_00000022_01111111
//00000000_0A4367BC_0CACACAC_05000000_00440000_00003300_00000022_01111111
//00E1D2C3_0A4367BC_0CACACAC_05000000_00440000_00003300_00000022_01111111

  //some random nops added for some variety in sequences

  //        //tag/way index   word                                dirty
  wr_req({14'h000,13'h000,3'h0,2'h0},4'b1111,32'h01111111,v);//i0 0000 -> 0001
  wr_req({14'h000,13'h000,3'h1,2'h0},4'b0001,32'h00000022,v);//i0 0001
  wr_req({14'h000,13'h000,3'h2,2'h0},4'b0010,32'h00003300,v);//i0 0001
  wr_req({14'h000,13'h000,3'h3,2'h0},4'b0100,32'h00440000,v);//i0 0001
  wr_req({14'h000,13'h000,3'h4,2'h0},4'b1000,32'h05000000,v);//i0 0001
  wr_req({14'h000,13'h000,3'h5,2'h0},4'b1111,32'h0CACACAC,v);//i0 0001
  wr_req({14'h000,13'h000,3'h6,2'h0},4'b1111,32'h0A4367BC,v);//i0 0001
  wr_req({14'h000,13'h000,3'h7,2'h0},4'b1111,32'h00E1D2C3,v);//i0 0001

          //tag/way index   word                              
  wr_req({14'h000,13'h001,3'h0,2'h0},4'b1111,32'h11111111,v);//i1 0000 -> 0001
  wr_req({14'h000,13'h001,3'h1,2'h0},4'b1111,32'h10000022,v);//i1
  wr_req({14'h000,13'h001,3'h2,2'h0},4'b1111,32'h10003300,v);//i1
  nop(1);
  wr_req({14'h000,13'h001,3'h3,2'h0},4'b1111,32'h10440000,v);//i1
  wr_req({14'h000,13'h001,3'h4,2'h0},4'b1111,32'h15000000,v);//i1
  wr_req({14'h000,13'h001,3'h5,2'h0},4'b1111,32'h1CACACAC,v);//i1
  wr_req({14'h000,13'h001,3'h6,2'h0},4'b1111,32'h1A4367BC,v);//i1
  wr_req({14'h000,13'h001,3'h7,2'h0},4'b1111,32'h10E1D2C3,v);//i1

          //tag/way index   word                              
  wr_req({14'h000,13'h002,3'h0,2'h0},4'b1111,32'h21111111,v);//i2 0000 -> 0001
  wr_req({14'h000,13'h002,3'h1,2'h0},4'b1111,32'h20000022,v);//i2 
  wr_req({14'h000,13'h002,3'h2,2'h0},4'b1111,32'h20003300,v);//i2 
  wr_req({14'h000,13'h002,3'h3,2'h0},4'b1111,32'h20440000,v);//i2 
  wr_req({14'h000,13'h002,3'h4,2'h0},4'b1111,32'h25000000,v);//i2 
  wr_req({14'h000,13'h002,3'h5,2'h0},4'b1111,32'h2CACACAC,v);//i2 
  wr_req({14'h000,13'h002,3'h6,2'h0},4'b1111,32'h2A4367BC,v);//i2 
  wr_req({14'h000,13'h002,3'h7,2'h0},4'b1111,32'h20E1D2C3,v);//i2 

          //      //tag/way index   word                              
  wr_req({14'h003,13'h000,3'h3,2'h0},4'b0011,32'h22221111,v); //i0 0001 -> 1001
  wr_req({14'h001,13'h001,3'h7,2'h0},4'b1100,32'h3333xxxx,v); //i1 
  nop(1);
  wr_req({14'h002,13'h002,3'h6,2'h0},4'b0001,32'hxxxxxx44,v); //i2 
  nop(1);
  wr_req({14'h000,13'h003,3'h5,2'h0},4'b0100,32'hxx55xxxx,v); //i3 
  wr_req({14'h000,13'h004,3'h2,2'h0},4'b0010,32'hxxxx66xx,v); //i4 
  wr_req({14'h001,13'h001,3'h1,2'h0},4'b1000,32'h77xxxxxx,v); //i1 
  wr_req({14'h002,13'h005,3'h5,2'h0},4'b1111,32'h88888888,v); //i5 
  wr_req({14'h003,13'h007,3'h3,2'h0},4'b1111,32'h99999999,v); //i7 
  wr_req({14'h000,13'h002,3'h2,2'h0},4'b1111,32'haaaaaaaa,v); //i2 
  wr_req({14'h001,13'h003,3'h1,2'h0},4'b1111,32'hbbbbbbbb,v); //i3 
  wr_req({14'h000,13'h003,3'h6,2'h0},4'b1111,32'hcccccccc,v); //i3 
  wr_req({14'h001,13'h003,3'h7,2'h0},4'b1111,32'hdddddddd,v); //i3 
  wr_req({14'h003,13'h006,3'h4,2'h0},4'b1111,32'heeeeeeee,v); //i6 
  wr_req({14'h000,13'h006,3'h3,2'h0},4'b1111,32'hffffffff,v); //i6 
  wr_req({14'h000,13'h002,3'h0,2'h0},4'b1111,32'h01020304,v); //i2 
  wr_req({14'h001,13'h000,3'h1,2'h0},4'b1111,32'h50607080,v); //i0 1001 -> 1011
  nop(1);

  load_expect_dary_data("./golden/basicWrHit.d0.memh",
                        "./golden/basicWrHit.d1.memh",
                        "./golden/basicWrHit.d2.memh",
                        "./golden/basicWrHit.d3.memh",v);

  load_expect_tags("./golden/basicWrHit.tags.memh",v);
  load_expect_bits("./golden/basicWrHit.bits.memb",v); //NOTE B file

  nop(1);

  check_data_arrays (errs,0,16,v); 
  check_tb_tags_bits(errs,0,2,v);

  endTestMsg(testName,errs,flag);
  nop(1);
end
endtask

  // ======================================================================
  // READ EVICT VERSION
  // #N
  // Access is to xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx -> xxxxxxxx hex
  // Line addr is xxx xxxx xxxx xxxx xxxx xxxx xxxx       ->  xxxxxxx hex
  // Index addr   x xxxx xxxx xxxx                        ->     xxxx hex
  // Word  addr                xxx                        ->        x hex
  // Byte  enbs               xxxx                        ->        x hex
  // LRU bits are xxx -> lru way is x
  //
  // Tag at index x/way x is: 14'hxxxx (14'bxx xxxx xxxx xxxx)
  // The data at index x/way x  is:
  // xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx 
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // <--- tag ----> <-- index --> <-0->
  // xxxxxxxxxxxxxx xxxxxxxxxxxxx xxxxx
  // xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx -> xxxxxxxx byte address
  // xxx xxxx xxxx xxxx xxxx xxxx xxxx       ->  xxxxxxx line address
  //
  // At the end of the access:
  //
  // main memory at 26'hxxxxx should contain:
  // xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index x way x should contain:
  //   xx xxxx xxxx xxxx  -> xxxx (from upper 14b of the access that missed)
  //
  // data at index x way x should contain (the contents of mm @0xxxxx):
  //   xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx 
  //
  // control bits at index x should be 
  //   val = 1111 (no change) 
  //   mod = xxxx (only wx is modified)
  //   lru =  xxx (was xxx, after write allocate to wayx becomes [x x  x])
  //
  //a:xxxxxxxx #N
  // ======================================================================

  // ======================================================================
  // WRITE EVICT VERSION
  // #0
  // Access is to xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx  -> 0xXXXXXXXX hex
  // Line addr is       xxx xxxx xxxx xxxx xxxx xxxx xxxx  ->  0xXXXXXXX hex
  // Index addr                          x xxxx xxxx xxxx  ->     0xXXXX hex
  // Word  addr                                       011  ->        0xX hex
  // Byte  enbs                                      1010 ->        xxxx bin
  // Write data                                               0xxxxxxxxx hex
  // LRU bits are xxx -> lru way is x
  //
  // Tag at index x/way x is: 14'hxxxx (14'bxx xxxx xxxx xxxx)
  // The data at index 0/way 2  is:
  // xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_
  //
  // The main memory (line) address for the write back is {tag,index,5'b0}
  // tag 14b idx 13b lin  5b
  //
  // <----- tag ----->   <--- index  --->  <-0->
  // xx xxxx xxxx xxxx   x xxxx xxxx xxxx  xxxxx
  // xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
  // xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx -> xxxxxxxx byte address
  // xxx xxxx xxxx xxxx xxxx xxxx xxxx       ->  xxxxxxx line address
  //
  // At the end of the access:
  //
  // main memory at 26'hxxxxxxxx should contain:
  // xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx 
  //
  // ----------------------------------------------------------------------
  // After allocation
  // ----------------------------------------------------------------------
  // tag at index x way x should contain:
  //   xx xxxx xxxx xxxx  -> xxxx (from upper 14b of the access that missed)
  //
  // data at index 0 way 2 should contain (the contents of mm @0000000):
  //   xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx 
  //
  // control bits at index 0 should be 
  //   val = 1111 (no change) 
  //   mod = 1111 (all are modified)
  //   lru =  xxx (was xxx, after read allocate to wayX becomes [x x x])
  //a:0xxxxxxxxx #x
  // ======================================================================

