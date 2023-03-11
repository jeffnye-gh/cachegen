Copyright (c) 2023 Jeff Nye

Ultimately this is a repo for my cache generation tool, cmd line and Qt5 based,
in progress but not in this repo yet.

I am releasing one example at the moment, a 1MB 4-way cache, w/ 32B lines, 
tree PLRU, physical addressing. There are two levels in the RTL and model,
the cache and a main memory block. There is nothing fancy in this example, 
no sub-sectors, victim or store buffers. Before adding these I want to 
verify capacity of the FPGA devices I have handy.

The RTL has basic directed tests for all the major operations. The RTL is
ready for randomized testing and physical implementation. I have some files
for Quartus targetting Cyclone 5. I will be adding Xlinix and the open
source tools as examples.

The RTL tests are directed and cover basic LD/ST hit, LD/ST allocation
on miss clean, miss dirty,and some LRU testing.

The functional model has similar tests for most of this except the write
miss allocation operation still needs work. The c++ is in two parts,
the functional model and the test case generator code.

RTL implementation detail in examples/1mb_l1/rtl/README.txt.

I spent some time getting a test bench mechanism that I like, that was
race-free and extensible from handwritten directed tests to generation and
playback of trace input files.

The RTL coding has some artifacts due to aspects of Icarus Verilog. I like
this tool but there are some things you need to account for when using 
iVerilog + GTKW. Not a big deal but there are notes when I made tradeoffs 
to accommodate the tools.

One thing to mention again is that physical design has not been accounted
for yet, some re-pipelining will occur when mapping from my implementation 
using a mental model of what can be done in one pipestage in 28nm and what can 
be done with reasonably priced FPGA boards. FYI.

I have written both the RTL and the model in a style that I think makes it
easier to comprehend while building efficient gates and fast models. I could 
be off base, I've been doing this for a while so I may have forgotten what 
is 'easy' to read and what is not. But in my real life I have also had to deal 
with RTL and C++ written in the most god awful way, so I made some effort.
YMMV.

- Jeff

Plugs for my public repos, so far:

- jeffnye-gh/cachegen 
- jeffnye-gh/jnutils 
- jeffnye-gh/testbench 
- jeffnye-gh/performance-modeling

