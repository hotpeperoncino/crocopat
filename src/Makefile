###########################################################################
# Dirk Beyer
# 2005-07-31: Modified for MacOS by Derek Rayside (MIT).
#
# This makefile is for Unix, MacOS and Cygwin.  
# Type 'make' to build the program.
#
############################################################################
#
# Macro definitions
#
# SHELL=/bin/sh


DEB   = #-g    # Store debugging information.
PROF  = #-pg   # Store profiling information.
OPT   = -O3    # Optimized code generation.
MINGW = #-mno-cygwin # For compilation without 'cygwin1.dll'.

CFLAGS:= $(DEB) $(PROF) $(OPT) $(MINGW) -I. -ansi #-Wall
LFLAGS:= $(PROF) $(MINGW) -L. -lrelbdd

BUILDTIME:= $(shell date -u +%Y-%m-%dT%H:%M:%S)
REVISION:=  $(shell svnversion)

ifeq (darwin, $(findstring darwin,$(subst Darwin,darwin,$(shell uname))))
	# mac
	CXX:=/usr/bin/g++       # apple version of g++
	LEX:=/usr/bin/flex -+   # apple version of flex
	YACC:=/sw/bin/bison     # fink version of bison is newer 
else
	# other platforms
	CXX:=g++
	LEX:=flex -+   # For Linux, where flex++ is not available.
	YACC:=bison
	LFLAGS:= $(LFLAGS) -static
endif


##############################################

LIB_OBJECTS=\
    crocopat.o\
    relYacc.tab.o\
    relLex.yy.o\
    relNumExpr.o\
    relStrExpr.o\
    relReaderWriter.o\
    bddBdd.o\
    relObject.o

TARGETS=crocopat
TARGET_OBJECTS=main.o

ALL_OBJECTS=$(LIB_OBJECTS) $(TARGET_OBJECTS)

ALL_SOURCES= $(ALL_OBJECTS:.o=.cpp)

LIBRARIES=librelbdd.a


###########################################

all: $(TARGETS)

###########################################

# Reset suffix list from default to empty.  This
#   removes default rules which seem to be veeeery
#   time consuming on the Win95 file system.
.SUFFIXES:

# Lex:
# Option '-i' means case-insensitive.
%.yy.cpp: %.l 
	$(LEX) -o$*.yy.cpp  $<

%.yy.o:	%.yy.cpp
	$(CXX) $(CFLAGS) -c $*.yy.cpp

# Yacc:
%.tab.cpp %.tab.hpp: %.y
	$(YACC) --defines --verbose -o $*.tab.cpp  $<
# Rename bison's output file name if using an old bison version.
#	mv relYacc.tab.cpp.h relYacc.tab.hpp   

%.tab.o: %.tab.cpp
	$(CXX) $(CFLAGS) -c $*.tab.cpp

# C++:
$(TARGETS).o: $(TARGETS).cpp
	$(CXX) $(CFLAGS) -DREVISION=$(REVISION) -DBUILDTIME=$(BUILDTIME) -c $(TARGETS).cpp

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $*.cpp

# Libraries:
%.a: $(LIB_OBJECTS)
	ar -rs $*.a $(LIB_OBJECTS)

$(TARGETS): $(TARGET_OBJECTS) $(LIBRARIES)
	$(CXX) -o $(TARGETS) $(TARGET_OBJECTS) $(LFLAGS)
	@sh config.sh > config.txt   # Keep track of the compiler versions.

clean:
	rm -f $(TARGETS) $(TARGETS).exe $(LIBRARIES) *.o *.yy.cpp *.tab.* *.out *.output *~

include dependencies

dep: $(ALL_SOURCES)
	rm -f dependencies
	touch dependencies
	for i in $(ALL_SOURCES); do \
	  echo Processing  $$i; \
	  $(CXX) $(CFLAGS) -MM $$i >> dependencies; \
	done

gprof: ttt
	gprof -E internal_mcount $(CUR_TARGET) > gprof.output

gcov: ttt
	for i in *.cpp; do gcov -l -f -b $$i > $$i.summary.gcov ; done

##############################################
