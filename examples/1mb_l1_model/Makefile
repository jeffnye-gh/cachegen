.PHONY: clean default run

CPP = g++

TARGET = ./bin/cgen
OPTS = --dry_run --output_file_prefix="tc_" --output_dir="output" \
       --l1_capacity=1048576 --l1_line_size=32 --l1_associativity=4 \
       --l1_critical_word_first \
       --mm_address_bits=32 --mm_capacity=4294967296 \
       --mm_fetch_size=32

ALL_SRC = $(wildcard src/*.cpp)
ALL_INC = $(wildcard inc/*.h)
ALL_OBJ = $(subst src/,obj/,$(ALL_SRC:.cpp=.o))
ALL_DEP = $(ALL_OBJ:.o=.d)

BOOST = /usr/local/boost_1_72_0
BOOST_INC = -I$(BOOST)/include
#BOOST_LIB = -L$(BOOST)/lib -lprogram_options
BOOST_LIB = -L$(BOOST)/lib /usr/local/boost/lib/libboost_program_options.a

DEF  = -DVERSION=\"0.0.1\"
DEP  = -MP -MD
INC  = -I./inc $(BOOST_INC)
LIBS = $(BOOST_LIB) 
OPT  = -g -O0
STD  = -std=c++17
WARN = -Wall

CPPFLAGS = $(DEF) $(DEP) $(INC) $(OPT) $(STD) $(WARN)
LDFLAGS  = 

default: run

obj/%.o: src/%.cpp
	-mkdir -p obj
	$(CPP) -c $(CPPFLAGS) $< -o $@

$(TARGET): $(ALL_OBJ)
	-mkdir -p bin
	$(CPP) $(LDFLAGS) $^ -o $@ $(LIBS)

run: $(TARGET)
	$(TARGET) $(OPTS)

-include $(ALL_DEP)

clean:
	-rm -f obj/* $(TARGET)
