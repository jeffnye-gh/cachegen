.PHONY: clean default gen sim lib mdl

CPP = g++

GEN_TARGET = ./bin/cgen
LIB_TARGET = ./lib/libcache.a
MDL_TARGET = ./bin/cmdl

GEN_OPTS = \
       --generate --run \
       --tc_prefix="tc_1" \
       --data_dir="data" \
       --json_file="data"/"tc_1.json" \
       \
       --l1_capacity=1048576 \
       --l1_line_size=32 \
       --l1_associativity=4 \
       --l1_read_miss_policy="allocate" \
       --l1_write_miss_policy="allocate" \
       --l1_write_hit_policy="no-write-thru" \
       --l1_replacement_policy="plru" \
       --l1_victim_buffer_size=0 \
       --l1_store_buffer_size=0 \
       --l1_tag_type="physical" \
       --l1_critical_word_first \
       \
       --mm_address_bits=32 \
       --mm_capacity=4294967296 \
       --mm_fetch_size=32

MDL_OPTS = \
       --load_json \
       --interactive  \
       --json_file="data"/"tc_1.json" \
       --cmd_file="data/tc_1.cmd"

GEN_SRC = $(wildcard src/gen*.cpp)
GEN_OBJ = $(subst src/,obj/,$(GEN_SRC:.cpp=.o))
GEN_INC = $(wildcard inc/gen*.h)

# src/options.cpp
LIB_SRC = src/ram.cpp src/utils.cpp src/options.cpp src/jsoncpp.cpp

LIB_OBJ = $(subst src/,obj/,$(LIB_SRC:.cpp=.o))
LIB_INC = inc/ram.h inc/utils.h inc/options.h inc/msg.h

MDL_SRC = $(wildcard src/mdl*.cpp)
MDL_OBJ = $(subst src/,obj/,$(MDL_SRC:.cpp=.o))
MDL_INC = $(wildcard inc/mdl*.h)

ALL_SRC = $(GEN_SRC) $(LIB_SRC) $(MDL_SRC)
ALL_OBJ = $(subst src/,obj/,$(ALL_SRC:.cpp=.o))
ALL_DEP = $(ALL_OBJ:.o=.d)

BOOST = /usr/local/boost_1_72_0
BOOST_INC = -I$(BOOST)/include
#BOOST_LIB = -L$(BOOST)/lib -lprogram_options
BOOST_LIB = -L$(BOOST)/lib /usr/local/boost/lib/libboost_program_options.a

DEF  = -DVERSION=\"0.0.1\"
DEP  = -MP -MD
INC  = -I./inc $(BOOST_INC)
LIBS = $(LIB_TARGET) $(BOOST_LIB)

OPT  = -g -O0
STD  = -std=c++17
WARN = -Wall

CPPFLAGS = $(DEF) $(DEP) $(INC) $(OPT) $(STD) $(WARN)
LDFLAGS  = 
ARFLAGS  = rvs

default: $(LIB_TARGET) $(GEN_TARGET) $(MDL_TARGET) sim

obj/%.o: src/%.cpp
	-mkdir -p obj
	$(CPP) -c $(CPPFLAGS) $< -o $@

lib: $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OBJ)
	-mkdir -p lib
	ar $(ARFLAGS) $(LIB_TARGET) $(LIB_OBJ)

$(GEN_TARGET): $(LIB_TARGET) $(GEN_OBJ)
	-mkdir -p bin
	$(CPP) $(LDFLAGS) $^ -o $@ $(LIBS)

$(MDL_TARGET): $(LIB_TARGET) $(MDL_OBJ)
	-mkdir -p bin
	$(CPP) $(LDFLAGS) $^ -o $@ $(LIBS)

gen: $(GEN_TARGET)
	$(GEN_TARGET) $(GEN_OPTS)

mdl: sim

sim: $(LIB_TARGET) $(MDL_TARGET)
	$(MDL_TARGET) $(MDL_OPTS)

-include $(ALL_DEP)

clean:
	-rm -f obj/* bin/* lib/*
