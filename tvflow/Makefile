ARPACKDIR = /home/schultz/work/test/arpack++
CC      = gcc
CXX	= g++
INCLUDE = -I $(ARPACKDIR)/include
CCFLAGS = -Wall -O2 -std=c99
CXXFLAGS= -Wall -Wno-write-strings -Wno-missing-field-initializers -O2
LIBS    = -lm -lteem -lbz2 -lpng -lz
TARGETS = tvflow

all:
	make clean
	make $(TARGETS)

.c:
	$(CC) $(INCLUDE) $(CCFLAGS) $@.c $(LIBS) -o $@

clean:
	rm -f $(TARGETS) *.o

cleanbkp:
	rm -f *.c~

tvflow: tvflow.cpp tv.o
	$(CXX) $(INCLUDE) $(CXXFLAGS) -o tvflow tvflow.cpp tv.o $(LIBS)

tv.o: tv.cpp
	$(CXX) $(INCLUDE) $(CXXFLAGS) -c -o tv.o tv.cpp
