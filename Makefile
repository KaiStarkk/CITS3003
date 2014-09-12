CC := clang++
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