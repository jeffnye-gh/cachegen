// ----------------------------------------------------------------------
// States - FIXME: reduce state width as needed once fsm works
// ----------------------------------------------------------------------
localparam [3:0] IDLE      = 4'h0;
localparam [3:0] WR_ALLOC  = 4'h1;
localparam [3:0] RD_ALLOC  = 4'h2;
localparam [3:0] FILL      = 4'h3;
localparam [3:0] WR_EVICT  = 4'h4;
localparam [3:0] RD_EVICT  = 4'h5;
localparam [3:0] READ      = 4'h6;
localparam [3:0] INVAL_ALL = 4'h7;
