// vi:syntax=verilog
`ifndef FUNCTIONS
`define FUNCTIONS 1
// =================================================================
// FUNCTIONS
// =================================================================
function [255:0] formLine;
input integer ram;
input integer addr;
reg [7:0] prefix;
begin
  prefix = { ram[3:0],addr[3:0] };
    formLine ={ prefix, 24'h000000,
                prefix, 24'h111111,
                prefix, 24'h222222,
                prefix, 24'h333333,
                prefix, 24'h444444,
                prefix, 24'h555555,
                prefix, 24'h666666,
                prefix, 24'h777777 };
end
endfunction
// -----------------------------------------------------------------
// -----------------------------------------------------------------
function compare(input [31:0] lhs,rhs);
begin
  compare = lhs === rhs;
end
endfunction
// -----------------------------------------------------------------
//$isunknown does not exist in icarus verilog
// -----------------------------------------------------------------
function isunknown(input [3:0] in);
begin isunknown = (^in == 1'bX); end
endfunction
// -----------------------------------------------------------------
//$onehot does not exist in icarus verilog
// -----------------------------------------------------------------
function onehot(input [3:0] in);
integer i;
integer cnt;
begin 
  cnt = 0;
  for(i=0;i<4;i=i+1) begin
    cnt += in[i];
  end
  onehot = (cnt === 1);
end
endfunction
`endif
