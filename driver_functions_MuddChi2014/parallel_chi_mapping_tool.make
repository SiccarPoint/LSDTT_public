# make with make -f chi_mapping_tool.make

CC=g++
CFLAGS=-c -Wall -O3 -fopenmp
OFLAGS = -Wall -O3 -fopenmp
LDFLAGS= -Wall
SOURCES=parallel_chi_mapping_tool.cpp \
             ../LSDMostLikelyPartitionsFinder.cpp \
             ../LSDIndexRaster.cpp \
             ../LSDRaster.cpp \
             ../LSDRasterInfo.cpp \
             ../LSDFlowInfo.cpp \
             ../LSDJunctionNetwork.cpp \
             ../LSDIndexChannel.cpp \
             ../LSDChannel.cpp \
             ../LSDIndexChannelTree.cpp \
             ../LSDStatsTools.cpp \
             ../LSDShapeTools.cpp \
             ../LSDChiNetwork.cpp \
             ../LSDBasin.cpp \
             ../LSDParticle.cpp \
             ../LSDChiTools.cpp \
             ../LSDParameterParser.cpp \
             ../LSDSpatialCSVReader.cpp \
             ../LSDCRNParameters.cpp \
             ../LSDRasterMaker.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=parallel_chi_mapping_tool.exe

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
	
clean:
	rm -f ../*.o *.o *.out *.exe
