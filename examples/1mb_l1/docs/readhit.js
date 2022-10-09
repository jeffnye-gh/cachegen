{ signal: [
  { name: "clk", wave: 'p.......' },
  { name: "cnt", wave: '========', data: [0,1,2,3,4,5,6,7,8] },
    [ "TOP",
      { name: "tb_cc_read",          wave: '010.1110' },
      { name: "tb_cc_address",       wave: 'x=xx===x', data: ["a1","a2","a3","a4"] },
      { name: "cc_tb_req_hit",       wave: '010.1110' },
      { name: "cc_tb_readdata",      wave: 'xx=xx===', data: ["rd1",  "rd2", "rd3","rd4"] },
      { name: "cc_tb_readdata_valid",wave: '0010.111', data: ["adt1",  "adt2", "adt3"] },
    ],
    [ "CC",
      { name: "tag_out",  wave: 'x=xx===x', data: ["rdt1",  "rdt2", "rdt3","rdt4"] },
      { name: "dary_out",  wave: 'xx=x', data: ["adt1",  "adt2", "adt3"] },
    ],
    [ "TB",
      { name: "capture_a", wave: '010.1110'   },
      { name: "capture_d", wave: '0.10.111'  }
    ]
  ],
     foot: { text: ["tspan", "READ HIT"], tock:-5 }
}

