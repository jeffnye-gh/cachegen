// vi:syntax=verilog
`ifndef FUNCTIONS
`define FUNCTIONS 1
// =================================================================
// FUNCTIONS
// =================================================================
function int check_word;
input [31:0] a;
input [31:0] exp;
reg [13:0] tagway;
reg [12:0] index;
reg [ 2:0] offset;
reg [31:0] act;
begin
  tagway = a[31:18];
  index  = a[17:5];
  offset = a[4:2];
  if(tagway == 14'h000) begin
    act = top.dut0.dsram0.ram[index] >> offset*32;
  end else if(tagway == 14'h001) begin
    act = top.dut0.dsram1.ram[index] >> offset*32;
  end else if(tagway == 14'h002) begin
    act = top.dut0.dsram2.ram[index] >> offset*32;
  end else if(tagway == 14'h003) begin
    act = top.dut0.dsram3.ram[index] >> offset*32;
  end

  if(act !== exp) begin
    $display("-E: way:%0d idx:%04x wrd:%03x exp:%08x act:%08x m:0",
             tagway,index,offset,exp,act);
    check_word = 1;
  end else begin
    $display("-I: way:%0d idx:%04x wrd:%03x exp:%08x act:%08x m:1",
             tagway,index,offset,exp,act);
    check_word = 0;
  end
end
endfunction
// -----------------------------------------------------------------
// -----------------------------------------------------------------
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
function compare32(input [31:0] lhs,rhs);
begin
  compare32 = lhs === rhs;
end
endfunction
// -----------------------------------------------------------------
// -----------------------------------------------------------------
function compare256(input [255:0] lhs,rhs);
begin
  compare256 = lhs === rhs;
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
