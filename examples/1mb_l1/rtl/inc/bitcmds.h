//`ifndef BITCMDS
//`define BITCMDS 1

localparam [3:0] B_CMD_NOP       = 4'h0;
localparam [3:0] B_CMD_VAL       = 4'h1;
localparam [3:0] B_CMD_INVAL     = 4'h2;
localparam [3:0] B_CMD_MOD       = 4'h3;
localparam [3:0] B_CMD_CLEAN     = 4'h4;
localparam [3:0] B_CMD_INVAL_ALL = 4'h5;
localparam [3:0] B_CMD_ALLOC     = 4'h6;
localparam [3:0] B_CMD_VAL_MOD   = 4'h7;
localparam [3:0] B_CMD_READ      = 4'h8;
localparam [3:0] B_CMD_LRU_UP    = 4'h9;
localparam [3:0] B_CMD_LRU_MOD_UP= 4'ha;

//`endif
