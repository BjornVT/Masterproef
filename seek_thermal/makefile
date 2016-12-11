.PHONY: all clean help startTest startCalib startRecord dir

SOURCEDIR = .
BUILDDIR = build
#SOURCES = $(wildcard $(SOURCEDIR)/*.cpp)
LIB_SOURCES = seek.cpp
TEST_SOURCES = seek-test.cpp camSettings.cpp
CALIB_SOURCES = seek-calib.cpp readSettings.cpp camSettings.cpp
RECORD_SOURCES = seek-record.cpp camSettings.cpp
CALIBTEST_SOURCES = testCalib.cpp camSettings.cpp
RESCALE_SOURCES = rescale.cpp camSettings.cpp

LIB_OBJECTS = $(patsubst %.cpp,build/%.o,$(LIB_SOURCES))
TEST_OBJECTS= $(patsubst %.cpp,build/%.o,$(TEST_SOURCES))
CALIB_OBJECTS = $(patsubst %.cpp,build/%.o,$(CALIB_SOURCES))
RECORD_OBJECTS = $(patsubst %.cpp,build/%.o,$(RECORD_SOURCES))
CALIBTEST_OBJECTS = $(patsubst %.cpp,build/%.o,$(CALIBTEST_SOURCES))
RESCALE_OBJECTS = $(patsubst %.cpp,build/%.o,$(RESCALE_SOURCES))
OBJECTS = $(patsubst %.cpp,build/%.o,$(wildcard $(SOURCEDIR)/*.cpp))

LIB = $(BUILDDIR)/libseek.a

EXECUTABLE_TEST = $(BUILDDIR)/seek-test
EXECUTABLE_CALIB = $(BUILDDIR)/seek-calib
EXECUTABLE_RECORD = $(BUILDDIR)/seek-record
EXECUTABLE_CALIBTEST = $(BUILDDIR)/calibTest
EXECUTABLE_RESCALE = $(BUILDDIR)/rescale

LIBS_DIRS = -L$(BUILDDIR)
LIBS = -lusb-1.0 -pthread -lseek $(shell pkg-config --libs opencv)
INCLUDE_DIRS = -I/usr/include/libusb-1.0 -I/usr/include/opencv

CFLAGS = -Wall -g
LDFLAGS = -g
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

$(EXECUTABLE_CALIBTEST): $(OBJECTS) $(LIB)
	@echo linken
	$(CC) $(LDFLAGS) $(CALIBTEST_OBJECTS) -o $(EXECUTABLE_CALIBTEST) $(INCLUDE_DIRS) $(LIBS_DIRS) $(LIBS)

$(EXECUTABLE_RECORD): $(OBJECTS) $(LIB)
	@echo linken
	$(CC) $(LDFLAGS) $(RECORD_OBJECTS) -o $(EXECUTABLE_RECORD) $(INCLUDE_DIRS) $(LIBS_DIRS) $(LIBS)	
	
$(EXECUTABLE_RESCALE): $(OBJECTS) $(LIB)
	@echo linken
	$(CC) $(LDFLAGS) $(RESCALE_OBJECTS) -o $(EXECUTABLE_RESCALE) $(INCLUDE_DIRS) $(LIBS_DIRS) $(LIBS)	

$(OBJECTS): $(BUILDDIR)/%.o : $(SOURCEDIR)/%.cpp
	@echo building $@
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_DIRS) $(LIBS)


$(LIB): $(LIB_OBJECTS)
	@echo making lib
	$(AR) $(ARFLAGS) $@ $(LIB_OBJECTS)

$(LIB_OBJECTS): $(LIB_SOURCES)
	@echo building $@
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_DIRS) $(LIBS)
	

startTest: $(EXECUTABLE_TEST)
	@echo starting $(EXECUTABLE_TEST)
	@echo ------------------------------------
	-@./$(EXECUTABLE_TEST)
	@echo ------------------------------------
	@echo Stopping program


startCalib: $(EXECUTABLE_CALIB)
	@echo starting $(EXECUTABLE_CALIB)
	@echo ------------------------------------
	-@./$(EXECUTABLE_CALIB)
	@echo ------------------------------------
	@echo Stopping program
	
	
startRecord: $(EXECUTABLE_RECORD)	
	@echo starting $(EXECUTABLE_RECORD)
	@echo ------------------------------------
	-@./$(EXECUTABLE_RECORD)
	@echo ------------------------------------
	@echo Stopping program
	
startCalibTest: $(EXECUTABLE_CALIBTEST)	
	@echo starting $(EXECUTABLE_CALIBTEST)
	@echo ------------------------------------
	-@./$(EXECUTABLE_CALIBTEST) 
	@echo ------------------------------------
	@echo Stopping program	
	
startScale: $(EXECUTABLE_RESCALE)	
	@echo starting $(EXECUTABLE_RESCALE)
	@echo ------------------------------------
	-@./$(EXECUTABLE_RESCALE) out/rgb/rgb.mp4 out/lwir/lwir.mp4 calib.yml.bkp
	@echo ------------------------------------
	@echo Stopping program	
	

clean:
	@echo cleaning
	-@$(RM) $(EXECUTABLE) $(OBJECTS) 2>/dev/null
	-@$(RM) $(EXECUTABLE_TEST) 2>/dev/null
	-@$(RM) $(EXECUTABLE_CALIB) 2>/dev/null
	-@$(RM) $(EXECUTABLE_RECORD) 2>/dev/null
	-@$(RM) $(EXECUTABLE_CALIBTEST) 2>/dev/null
	-@$(RM) $(EXECUTABLE_RESCALE) 2>/dev/null


help:
	@echo Targets are: all $(EXECUTABLE_TEST) $(EXECUTABLE_CALIB) $(EXECUTABLE_RECORD) $(OBJECTS) startCalib startTest startRecord clean help
	@echo To be able to open the usb device, you need to add a udev file. See readme
	
