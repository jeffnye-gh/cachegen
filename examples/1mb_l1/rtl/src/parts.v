// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
module bnk_be(
  output wire [3:0] y,
  input  wire [3:0] be,
  input  wire       sel
);

assign y = { be[3] & sel, be[2] & sel, be[1] & sel, be[0] & sel };
endmodule
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
