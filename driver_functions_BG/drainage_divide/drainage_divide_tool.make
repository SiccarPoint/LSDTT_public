# make with make -f basin_averager.make

CC=g++
CFLAGS=-c -Wall -O3 
OFLAGS = -Wall -O3 
LDFLAGS= -Wall
SOURCES=drainage_divide_tool.cpp \
    ../../LSDIndexRaster.cpp \
    ../../LSDRaster.cpp \
    ../../LSDRasterInfo.cpp \
    ../../LSDBasin.cpp \
    ../../LSDFlowInfo.cpp \
    ../../LSDStatsTools.cpp \
    ../../LSDJunctionNetwork.cpp \
    ../../LSDIndexChannel.cpp \
    ../../LSDChannel.cpp \
    ../../LSDMostLikelyPartitionsFinder.cpp \
    ../../LSDShapeTools.cpp \
    ../../LSDAnalysisDriver.cpp \
    ../../LSDSpatialCSVReader.cpp \
    ../../LSDCRNParameters.cpp \
    ../../LSDParticle.cpp \
    ../../LSDParameterParser.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=drainage_divide_tool.exe

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
