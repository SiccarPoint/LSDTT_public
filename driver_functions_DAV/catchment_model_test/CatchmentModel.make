CC = g++
CFLAGS= -c -Wreturn-type -Og -pg -ggdb -std=gnu++11 -fopenmp
OFLAGS = -Wreturn-type -Og -pg -ggdb -std=gnu++11 -fopenmp
LDFLAGS= -Wreturn-type
SOURCES = ../catchmentmodel_driver.cpp \
			../../LSDCatchmentModel.cpp \
			../../LSDRaster.cpp \
			../../LSDIndexRaster.cpp \
			../../LSDStatsTools.cpp \
			../../LSDShapeTools.cpp \
			../../LSDGrainMatrix.cpp
SCRIPTS = 
OBJ = $(SOURCES:.cpp=.o)
#LIBS = -lfftw3 -lpython2.7 -g -O0 -D_GLIBCXX_DEBUG
#LIBS = -lfftw3 -lpython2.7 -Wwrite-strings
LIBS = 
EXEC = CatchmentModelOpenMP.out

all: $(SOURCES) $(SCRIPTS) $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OFLAGS) $(OBJ) $(LIBS) -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@


clean:
	rm -f ../../$(OBJ) ../catchmentmodel_driver.o CatchmentModelOpenMP.out
