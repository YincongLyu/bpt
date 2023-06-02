# Redis Makefile
# Copyright (C) 2009 Salvatore Sanfilippo <antirez at gmail dot com>
# This file is released under the BSD license, see the COPYING file

OPTIMIZATION?=
CFLAGS?=-std=c++0x $(OPTIMIZATION) -Wall $(PROF)
CCLINK?=
DEBUG?=-g -ggdb
CCOPT= $(CFLAGS) $(ARCH) $(PROF)

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
BINCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CXX$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR);
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR);
endif

#TESTPRGNAME = bpt_unit_test

#OBJ = bpt.o util/cli.o
#PRGNAME = bpt_cli
OBJ = bpt.o test.o
PRGNAME = bpt_unit_test

#DUMP_OBJ = bpt.o util/dump_numbers.o
#DUMPPRGNAME = bpt_dump_numbers

#all: $(DUMPPRGNAME) $(PRGNAME)
all: ${PRGNAME}
run:
	./bpt_unit_test
# test:
# 	@-rm bpt_unit_test
# 	$(MAKE) TEST="-DUNIT_TEST" bpt_unit_test
# 	./bpt_unit_test

gprof:
	$(MAKE) PROF="-pg"

gcov:
	$(MAKE) PROF="-fprofile-arcs -ftest-coverage"

noopt:
	$(MAKE) OPTIMIZATION=""

clean:
	rm -rf $(PRGNAME) $(CHECKDUMPPRGNAME) $(CHECKAOFPRGNAME) *.o *.gcda *.gcno *.gcov
#	rm -rf $(PRGNAME) $(TESTPRGNAME) $(DUMPPRGNAME) $(CHECKDUMPPRGNAME) $(CHECKAOFPRGNAME) *.o *.gcda *.gcno *.gcov util/*.o

distclean: clean
	$(MAKE) clean

dep:
	$(CC) -MM *.cc

bpt_unit_test: $(OBJ)
	$(QUIET_LINK)$(CXX) -o $(PRGNAME) $(CCOPT) $(DEBUG) $(OBJ) $(CCLINK)
#bpt_cli: $(OBJ)
#	$(QUIET_LINK)$(CXX) -o $(PRGNAME) $(CCOPT) $(DEBUG) $(OBJ) $(CCLINK)

#bpt_unit_test:
#	$(QUIET_LINK)$(CXX) -o bpt_unit_test $(CCOPT) $(DEBUG) util/unit_test.cc bpt.cc $(TEST) $(CCLINK) 

#bpt_dump_numbers: $(DUMP_OBJ)
#	$(QUIET_LINK)$(CXX) -o $(DUMPPRGNAME) $(CCOPT) $(DEBUG) $(DUMP_OBJ) $(CCLINK)

%.o: %.cc
#目前没有test功能
#	$(QUIET_CC)$(CXX) -o $@ -c $(CFLAGS) $(TEST) $(DEBUG) $(COMPILE_TIME) $<
	$(QUIET_CC)$(CXX) -c $(CFLAGS) $(DEBUG) $(COMPILE_TIME) $<

# Deps (use make dep to generate this)
bpt.o: bpt.cc bpt.h predefined.h
test.o: test.cc bpt.h predefined.h
#bpt.o: bpt.cc bpt.h predefined.h
#cli.o: cli.cc bpt.h predefined.h
#dump_numbers.o: dump_numbers.cc bpt.h predefined.h
#unit_test.o: unit_test.cc bpt.h predefined.h
