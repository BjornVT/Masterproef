.PHONY: all clean help start 

SOURCEDIR = .
BUILDDIR = build
SOURCES = $(wildcard $(SOURCEDIR)/*.cpp)
OBJECTS = $(patsubst %.cpp,build/%.o,$(SOURCES))
EXECUTABLE = $(BUILDDIR)/seek

LIBS = -lusb-1.0 $(shell pkg-config --libs opencv)
INCLUDE_DIRS = -I/usr/include/libusb-1.0 -I/usr/include/opencv

CFLAGS = -Wall 
LDFLAGS =
CC = g++
RM = rm

#-----------------------------------------

all: dir $(EXECUTABLE) $(SOURCES)

dir:
	mkdir -p $(BUILDDIR)

$(EXECUTABLE): $(OBJECTS)
	@echo linken
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(EXECUTABLE) $(INCLUDE_DIRS) $(LIBS)

$(OBJECTS): $(BUILDDIR)/%.o : $(SOURCEDIR)/%.cpp
	@echo building $@
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_DIRS) $(LIBS)

start: $(EXECUTABLE)
	@echo starting $(EXECUTABLE)
	@echo ------------------------------------
	-@sudo ./$(EXECUTABLE)
	@echo ------------------------------------
	@echo Stopping program

clean:
	@echo cleaning
	-@$(RM) $(EXECUTABLE) $(OBJECTS) 2>/dev/null

help:
	@echo Targets are: all $(EXECUTABLE) $(OBJECTS) start clean help
	
	
