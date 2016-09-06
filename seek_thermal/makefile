.PHONY: all clean help startTest startCalib startRecord dir

SOURCEDIR = .
BUILDDIR = build
#SOURCES = $(wildcard $(SOURCEDIR)/*.cpp)
LIB_SOURCES = seek.cpp
TEST_SOURCES = seek-test.cpp
CALIB_SOURCES = seek-calib.cpp
RECORD_SOURCES = seek-record.cpp

LIB_OBJECTS = $(patsubst %.cpp,build/%.o,$(LIB_SOURCES))
TEST_OBJECTS= $(patsubst %.cpp,build/%.o,$(TEST_SOURCES))
CALIB_OBJECTS = $(patsubst %.cpp,build/%.o,$(CALIB_SOURCES))
RECORD_OBJECTS = $(patsubst %.cpp,build/%.o,$(RECORD_SOURCES))
OBJECTS = $(patsubst %.cpp,build/%.o,$(wildcard $(SOURCEDIR)/*.cpp))

LIB = $(BUILDDIR)/libseek.a

EXECUTABLE_TEST = $(BUILDDIR)/seek-test
EXECUTABLE_CALIB = $(BUILDDIR)/seek-calib
EXECUTABLE_RECORD = $(BUILDDIR)/seek-record

LIBS_DIRS = -L$(BUILDDIR)
LIBS = -lusb-1.0 -lseek $(shell pkg-config --libs opencv)
INCLUDE_DIRS = -I/usr/include/libusb-1.0 -I/usr/include/opencv

CFLAGS = -Wall
LDFLAGS =
ARFLAGS = ru
AR = ar
CC = g++
RM = rm

#-----------------------------------------

all: dir $(EXECUTABLE_TEST) $(EXECUTABLE_CALIB) $(EXECUTABLE_RECORD)


dir:
	mkdir -p $(BUILDDIR)


$(EXECUTABLE_TEST): $(OBJECTS) $(LIB)
	@echo linken
	$(CC) $(LDFLAGS) $(TEST_OBJECTS) -o $(EXECUTABLE_TEST) $(INCLUDE_DIRS) $(LIBS_DIRS) $(LIBS)
	
	
$(EXECUTABLE_CALIB): $(OBJECTS) $(LIB)
	@echo linken
	$(CC) $(LDFLAGS) $(CALIB_OBJECTS) -o $(EXECUTABLE_CALIB) $(INCLUDE_DIRS) $(LIBS_DIRS) $(LIBS)


$(EXECUTABLE_RECORD): $(OBJECTS) $(LIB)
	@echo linken
	$(CC) $(LDFLAGS) $(RECORD_OBJECTS) -o $(EXECUTABLE_RECORD) $(INCLUDE_DIRS) $(LIBS_DIRS) $(LIBS)
	

$(OBJECTS): $(BUILDDIR)/%.o : $(SOURCEDIR)/%.cpp
	@echo building $@
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_DIRS) $(LIBS)


$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(LIB_OBJECTS)


startTest: $(EXECUTABLE_TEST)
	@echo starting $(EXECUTABLE_TEST)
	@echo ------------------------------------
	-@sudo ./$(EXECUTABLE_TEST)
	@echo ------------------------------------
	@echo Stopping program


startCalib: $(EXECUTABLE_CALIB)
	@echo starting $(EXECUTABLE_CALIB)
	@echo ------------------------------------
	-@sudo ./$(EXECUTABLE_CALIB)
	@echo ------------------------------------
	@echo Stopping program
	
	
startRecord: $(EXECUTABLE_RECORD)	
	startCalib: $(EXECUTABLE_RECORD)
	@echo starting $(EXECUTABLE_RECORD)
	@echo ------------------------------------
	-@sudo ./$(EXECUTABLE_RECORD)
	@echo ------------------------------------
	@echo Stopping program
	

clean:
	@echo cleaning
	-@$(RM) $(EXECUTABLE) $(OBJECTS) 2>/dev/null
	-@$(RM) $(EXECUTABLE_TEST) 2>/dev/null
	-@$(RM) $(EXECUTABLE_CALIB) 2>/dev/null
	-@$(RM) $(EXECUTABLE_RECORD) 2>/dev/null


help:
	@echo Targets are: all $(EXECUTABLE_TEST) $(EXECUTABLE_CALIB) $(EXECUTABLE_RECORD) $(OBJECTS) startCalib startTest startRecord clean help
	
	
