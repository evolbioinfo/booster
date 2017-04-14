# Version of bstools
GIT_VERSION := $(shell git describe --abbrev=10 --dirty --always --tags)

# Compiler: gcc

ifeq ($(cross),win32)
        CC = i686-w64-mingw32-gcc
else
	ifeq ($(cross),win64)
		CC = x86_64-w64-mingw32-gcc
	else
		CC = gcc
	endif
endif

# Compiler flags: all warnings + debugger meta-data
# CFLAGS = -Wall -O3 -fopenmp
CFLAGS = -Wall -g -O3 -DVERSION=\"$(GIT_VERSION)\"
CFLAGS_OMP = -Wall -g -fopenmp

# External libraries to link to: only the mathlib for now
LIBS = -lm
OBJS = hashtables_bfields.o  tree.o stats.o prng.o hashmap.o version.o sort.o io.o tree_utils.o

# default target
ALL = booster

INSTALL_PATH=$$HOME/bin/

all : $(ALL)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

# ****
# the "booster" supports. Needs ref tree and bt trees.
# ****
booster: $(OBJS) booster.c
	$(CC) $(CFLAGS_OMP) -o $@ $^ $(LIBS)


# ****
# TESTS
# ****
tests: $(OBJS) test.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test : tests
	./tests

.PHONY: clean

clean:
	rm -f *~ *.o $(ALL) tests 
	rm -rf *.dSYM

install: all
	mkdir -p $(INSTALL_PATH)
	cp $(ALL) $(INSTALL_PATH)

uninstall:
	rm $(addprefix $(INSTALL_PATH),$(ALL))
