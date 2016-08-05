.PHONY: all clean help start 

CC = g++
CFLAGS = -Wall 
LDFLAGS =
SOURCES = seek.cpp seek-test.cpp
OBJECTS = $(patsubst %.cpp,build/%.o,$(SOURCES))
#OBJECTS = $(SOURCES:.cpp=.o)
LIBS = -lusb-1.0 $(shell pkg-config --libs opencv)
INCLUDE_DIRS = -I/usr/include/libusb-1.0 -I/usr/include/opencv
EXECUTABLE = seek
BUILDDIR = build
RM = rm


#-----------------------------------------

all: dir $(BUILDDIR)/$(EXECUTABLE) $(SOURCES)

dir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(EXECUTABLE): $(OBJECTS)
	@echo linken
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(BUILDDIR)/$(EXECUTABLE) $(INCLUDE_DIRS) $(LIBS)

$(OBJECTS): $(BUILDDIR)/%.o : %.cpp
	@echo building $@
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_DIRS) $(LIBS)

start: $(BUILDDIR)/$(EXECUTABLE)
	@echo starting $(BUILDDIR)/$(EXECUTABLE)
	@echo ------------------------------------
	-@./$(BUILDDIR)/$(EXECUTABLE))
	@echo ------------------------------------
	@echo Stopping program

clean:
	@echo cleaning
	-@$(RM) $(BUILDDIR)/$(EXECUTABLE) $(BUILDDIR)/*.o 2>/dev/null

help:
	@echo Targets are: all $(OBJECTS) start clean help
	
	
