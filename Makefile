# #! /usr/bin/make

# SOURCES = $(wildcard *.cpp)   # $(wildcard *.c)
# HEADERS = $(wildcard *.h)
# TARGETS = $(basename $(SOURCES))

# INIT_SHADER = ../../Common/InitShader.o

# uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# # Linux (default)
# #LDLIBS = -lglut -lGL -lXmu -lX11  -lm
# LDDIRS = -L.
# LDLIBS = -l:libassimp.so.3 -lGLEW -lglut -lGL -lXmu -lX11  -lm -Wl,-rpath,. #-rpath,../../lib/linux

# CXXINCS = -I../../include -I../../assimp--3.0.1270/include
# CXXFLAGS = $(CXXOPTS) $(CXXDEFS) $(CXXINCS) -Wall -fpermissive -O3 -g
# LDFLAGS = $(LDOPTS) $(LDDIRS) $(LDLIBS)

# DIRT = $(wildcard *.o *.i *~ */*~ *.log)

# #-----------------------------------------------------------------------------

# .PHONY: Makefile gnatidread.h

# default all: $(TARGETS)

# $(TARGETS): $(INIT_SHADER)

# %: %.cpp gnatidread.h
# 	$(CXX) $(CXXFLAGS) $@.cpp $(INIT_SHADER) bitmap.c $(LDFLAGS) -o $@


# %: %.c 
# 	gcc $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# #-----------------------------------------------------------------------------

# %.i: %.cpp
# 	$(CXX) -E $(CXXFLAGS) $< | uniq > $@

# #-----------------------------------------------------------------------------

# clean:
# 	$(RM) $(DIRT)

# rmtargets:
# 	$(RM) $(TARGETS) $(INIT_SHADER)

# clobber: clean rmtargets

CC := g++
SRCDIR := src
BUILDDIR := build
TARGET := bin/scene-start

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CFLAGS := -g
LIB := -lassimp -lGLEW -lglut -lGL -lXmu -lX11 -lm -L lib
INC := -I include

$(TARGET): $(OBJECTS) $(BUILDDIR)/bitmap.o
	@echo "$(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo "$(CC) $(CCFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CCFLAGS) $(INC) -c -o $@ $<

$(BUILDDIR)/bitmap.o: $(SRCDIR)/bitmap.c
	@mkdir -p $(BUILDDIR)
	@echo "$(CC) $(CCFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CCFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "$(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

.PHONY: clean% 