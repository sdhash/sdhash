BOOST_DIR = ../external
#BOOST_DIR = /usr/local/boost
THRIFT_DIR = /usr/local/include/thrift
GEN_DIR = ./gen-cpp
SDBF_DIR = ../sdbf
COMMON_DEF=-DHAVE_NETINET_IN_H -DHAVE_INTTYPES_H
CC=g++

LDFLAGS= -L/usr/local/lib -L../external/stage/lib -L..

INCLUDES=-I${THRIFT_DIR} -I${BOOST_DIR} -I${SDBF_DIR} -I${GEN_DIR} -fopenmp

SRC = gen-cpp/sdhashsrv.cpp gen-cpp/sdhashsrv_types.cpp

CLSRC = src/sdhash-cli.cc 

SDSRC = src/sdhash-srv-main.cc src/set_list.cc src/sdhash-srv-opts.cc 

default: server client manager

server: ${SDSRC}
	$(CC) -O0 -g -o sdhash-srv ${INCLUDES} ${COMMON_DEF} ${LDFLAGS}  ${SDSRC} ${SRC} -lthrift -lsdbf -lpthread -lboost_program_options -lboost_filesystem -lboost_system -lboost_thread -lcrypto

manager: src/sdhash-mgr.cc
	$(CC) -O2 -o sdhash-mgr ${INCLUDES} ${COMMON_DEF}  ${LDFLAGS} src/sdhash-mgr.cc ${SRC} -lthrift 

client: ${CLSRC}
	$(CC) -O2 -o sdhash-cli ${INCLUDES} ${COMMON_DEF}  ${LDFLAGS} ${CLSRC} ${SRC} -lthrift -lboost_filesystem -lboost_system -lboost_program_options

clean:
	$(RM) -r sdhash-srv sdhash-cli sdhash-mgr sdtest
