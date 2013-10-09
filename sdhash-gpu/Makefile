
SOURCES=matrixCompare.cu matrixCompare.cpp
INCLUDES=-I../sdbf -I../external/
LDFLAGS1=-L../ -L../external/stage/lib -L/usr/local/cuda/lib64
LDFLAGS2=-lsdbf -lboost_system -lboost_filesystem -lboost_program_options -lc -lm -lcrypto -lboost_thread -lpthread  -lcudart

program: all 

matrixCompare-c.o:
	$(NVCC) $(INCLUDES) -arch=sm_20 -c matrixCompare.cu -o matrixCompare-c.o
matrixCompare.o:
	$(CC) $(INCLUDES) -c matrixCompare.cpp

NVCC=nvcc
CC=g++

all: matrixCompare-c.o matrixCompare.o
	$(CC) -fopenmp $(INCLUDES) $(LDFLAGS1) -o sdhash-gpu matrixCompare-c.o matrixCompare.o $(LDFLAGS2)


#nvcc I/home/cequates/zaphod/trunk/sdbf -L/home/cequates/zaphod/trunk -L/home/cequates/zaphod/trunk/external/stage/lib -o matrixCompare matrixCompare.cu matrixCompare.cpp 
