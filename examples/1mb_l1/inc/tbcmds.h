`ifndef TB_CMDS
`define TB_CMDS 1

localparam [3:0] TB_CMD_NOP       = 4'h0;
localparam [3:0] TB_CMD_NORMAL    = 4'h1;
localparam [3:0] TB_CMD_BYPASS    = 4'h2;
localparam [3:0] TB_CMD_INVAL     = 4'h3;
localparam [3:0] TB_CMD_INVAL_ALL = 4'h4;
localparam [3:0] TB_CMD_DIRTY     = 4'h5;
localparam [3:0] TB_CMD_CLEAN     = 4'h6;
localparam [3:0] TB_CMD_FLUSH     = 4'h7;
localparam [3:0] TB_CMD_FLUSH_ALL = 4'h8;
localparam [3:0] TB_CMD_WBACK     = 4'h9;
localparam [3:0] TB_CMD_TERM      = 4'ha; //end sim

`endif
