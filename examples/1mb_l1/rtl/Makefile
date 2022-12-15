.PHONY: run default clean wave waves data
ALL_SRC = $(wildcard src/*.v)
ALL_INC = $(wildcard inc/*.h)

VLIBS   = -lsrc/parts.v
VLG_SRC = $(filter-out src/parts.v,$(ALL_SRC))

IVL      = iverilog
IVL_DEF  = 
IVL_OPTS = -g2012 -s top -I./inc $(IVL_DEF) $(VLIBS)

WAVE = /usr/local/gtkwave/bin/gtkwave
WV_VCD_FILE  = -f csim.vcd
WV_SAVE_FILE = -a csim.gtkw
#WV_RC_FILE   = -r csim.rc

DROM = ../../../../exttools/wavedrom/wavedrom-editor
WAVE_OPTS = $(WV_VCD_FILE) $(WV_SAVE_FILE) $(WV_RC_FILE)
#WAVE_OPTS = $(WV_VCD_FILE) $(WV_SAVE_FILE) $(WV_RC_FILE)

TARGET = bin/cachesim

default: run

$(TARGET): $(ALL_SRC) $(ALL_INC)
	-mkdir -p bin
	$(IVL) $(IVL_OPTS)	$(VLG_SRC) -o $@

run: $(TARGET)
	$(TARGET)

data:
	./scripts/gentbdata.py

waves: wave
wave:
	$(WAVE) $(WAVE_OPTS)  &

drom:
	$(DROM)

#_IVL_OPTS = -g2012  -I./inc $(IVL_DEF) $(VLIBS)
#
#one:
#	$(IVL) $(_IVL_OPTS) src/fsm.v

clean:
	-rm -f $(TARGET)
	-rm -f $(WV_VCD_FILE)
