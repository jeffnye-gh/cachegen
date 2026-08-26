# ----------------------------------------------------------------------
# FILE:    Makefile
# SOURCE:  CLI-001
# STATUS:  WORKING
# UPDATED: 2026-08-25
# CONTACT: Jeff Nye
#
# Builds cgen and the gtest suite. Make only, never CMake, see D-1.
# C++20, see D-2.
#
# Dependencies, all discovered in this tree, none installed:
#   nlohmann/json               inc/nlohmann/json.hpp, single header
#   pboettch json-schema-validator
#                               ../tools/json-schema-validator/src,
#                               the six sources are compiled in
#   Boost.ProgramOptions        system, /usr/include/boost, linked
#                               with -lboost_program_options
#   gtest                       ../tools/googletest, gtest-all.cc and
#                               gtest_main are compiled in
#   jnutils msg                 ../tools/jnutils/msg/inc, header only
# ----------------------------------------------------------------------
.PHONY: default all only test run clean help

CPP = g++
# --------------------------------------------------------------
TARGET = bin/cgen
TESTER = bin/cgen_tests
# --------------------------------------------------------------
REPO    = ..
MSG_INC = $(REPO)/tools/jnutils/msg/inc
JSV_DIR = $(REPO)/tools/json-schema-validator/src
GT_DIR  = $(REPO)/tools/googletest/googletest
GM_DIR  = $(REPO)/tools/googletest/googlemock
SCHEMAS = $(REPO)/planning/schema
PACINO  = $(REPO)/testcases/pacino/pacino_system.json
# --------------------------------------------------------------
CLI_SRC = $(wildcard src/*.cpp)
CLI_OBJ = $(patsubst src/%.cpp,obj/%.o,$(CLI_SRC))
LIB_OBJ = $(filter-out obj/main.o,$(CLI_OBJ))

JSV_BASE = json-validator json-uri json-patch \
           json-schema-draft7.json string-format-check \
           smtp-address-validator
JSV_OBJ  = $(patsubst %,obj/jsv_%.o,$(JSV_BASE))

TB_SRC = $(wildcard tb/*.cpp)
TB_OBJ = $(patsubst tb/%.cpp,obj/tb_%.o,$(TB_SRC))

GT_OBJ = obj/gtest-all.o
ALL_DEP = $(wildcard obj/*.d)
# --------------------------------------------------------------
DEP  = -MMD -MP
DEF  = -DVERSION='"0.1.0"'
INC  = -I./inc -isystem $(MSG_INC) -isystem $(JSV_DIR)
OPT  = -g -O1
STD  = -std=c++20
WARN = -Wall -Wextra
LIBS = -lboost_program_options -lpthread

CPPFLAGS = $(OPT) $(DEP) $(DEF) $(INC) $(WARN) $(STD)
GT_INC   = -isystem $(GT_DIR)/include -isystem $(GT_DIR)
LDFLAGS  =
# --------------------------------------------------------------
default: all
all: $(TARGET) $(TESTER)
only: $(TARGET)
# --------------------------------------------------------------
obj/%.o: src/%.cpp
	@mkdir -p obj
	$(CPP) -c $(CPPFLAGS) $< -o $@

obj/jsv_%.o: $(JSV_DIR)/%.cpp
	@mkdir -p obj
	$(CPP) -c $(CPPFLAGS) $< -o $@

obj/tb_%.o: tb/%.cpp
	@mkdir -p obj
	$(CPP) -c $(CPPFLAGS) $(GT_INC) $< -o $@

obj/gtest-all.o: $(GT_DIR)/src/gtest-all.cc
	@mkdir -p obj
	$(CPP) -c $(OPT) $(STD) $(GT_INC) $< -o $@

$(TARGET): $(CLI_OBJ) $(JSV_OBJ)
	@mkdir -p bin
	$(CPP) $(LDFLAGS) $^ -o $@ $(LIBS)

$(TESTER): $(LIB_OBJ) $(JSV_OBJ) $(TB_OBJ) $(GT_OBJ)
	@mkdir -p bin
	$(CPP) $(LDFLAGS) $^ -o $@ $(LIBS)
# --------------------------------------------------------------
# check the pacino configuration end to end
run: $(TARGET)
	@echo " ---------------------------------------------"
	-@CGEN_SCHEMA_DIR=$(SCHEMAS) $(TARGET) --cmd=check --config=$(PACINO)
	@echo " ---------------------------------------------"

test: $(TESTER)
	@CGEN_SCHEMA_DIR=$(SCHEMAS) $(TESTER)
# --------------------------------------------------------------
# Helpful to display the value of a variable, e.g. make help-INC
help-%:
	@echo $* = $($*)

-include $(ALL_DEP)

clean:
	-rm -rf obj bin
