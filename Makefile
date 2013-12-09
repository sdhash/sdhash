# Makefile for sdhash.  Change options at top if necessary

DESTDIR=
PREFIX=$(DESTDIR)/usr/local
INSTDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man/man1


SDBF_SRC = sdbf/sdbf_class.cc sdbf/sdbf_core.cc sdbf/map_file.cc sdbf/entr64.cc sdbf/base64.cc sdbf/bf_utils.cc sdbf/error.cc sdbf/sdbf_conf.cc sdbf/sdbf_set.cc base64/modp_b64.cc sdbf/bloom_filter.cc lz4/lz4.cc sdbf/bloom_vector.cc sdbf/blooms.pb.cc

SDHASH_SRC = sdhash-src/sdhash.cc sdhash-src/sdhash_threads.cc 

BLOOM_TEST = test/bloom-test.cc test/modp_b16.cc 


CC = g++
LD = $(CC)

# BAD, BAD OPTIMIZATION! -fstrict-aliasing 
ifneq ($(MAKECMDGOALS),debug)
CFLAGS = -fPIC -fopenmp -msse4.2 -O3 -fno-strict-aliasing -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE_API -D_BSD_SOURCE -I./external 
else
CFLAGS = -fPIC -fopenmp -msse4.2 -O0 -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE_API -D_BSD_SOURCE -I./external -Wall
endif

LDFLAGS = -fopenmp -L . -L./external/stage/lib -lboost_system -lboost_filesystem -lboost_program_options -lc -lm -lcrypto -lboost_thread -lpthread -lprotobuf

SDHASH_OBJ = $(SDHASH_SRC:.cc=.o)
SDBF_OBJ = $(SDBF_SRC:.cc=.o)
BLOOM_TEST_OBJ = $(BLOOM_TEST:.cc=.o)


LIBSDBF=libsdbf.a

all: boost stream 

debug: boost stream 

cygwin: boost stream

install: boost man 
	mkdir -p $(INSTDIR)
	mkdir -p $(MANDIR)
#cp sdhash sdhash-srv sdhash-cli $(INSTDIR)
	cp sdhash $(INSTDIR)
	cp man/sdhash.1 $(MANDIR)
	cp man/sdhash-cli.1 $(MANDIR)
	cp man/sdhash-srv.1 $(MANDIR)

install-server: install server
	cp sdhash-server/sdhash-srv $(INSTDIR)
	cp sdhash-server/sdhash-mgr $(INSTDIR)
	cp sdhash-server/sdhash-cli $(INSTDIR)

man: man/sdhash.1 man/sdhash-cli.1 man/sdhash-srv.1

docs: 
	doxygen 

man/sdhash.1:
	pod2man -c "" -r "" man/sdhash.pod > man/sdhash.1

man/sdhash-srv.1:
	pod2man -c "" -r "" man/sdhash-srv.pod > man/sdhash-srv.1

man/sdhash-cli.1:
	pod2man -c "" -r "" man/sdhash-cli.pod > man/sdhash-cli.1

swig-py: boost swig/python/sdbf_wrap.o swig/python/_sdbf_class.so 
swig-win-py: boost swig/python/sdbf_wrap.o swig/python/_sdbf_class.dll 

swig/python/sdbf_wrap.o: sdbf.i $(LIBSDBF)
	swig -c++ -python swig/python/sdbf.i
	g++ -std=c++0x -fPIC -c swig/python/sdbf_wrap.cxx -o swig/python/sdbf_wrap.o -I/usr/include/python2.7

swig/python/_sdbf_class.so: swig/python/sdbf_wrap.o $(LIBSDBF)
	g++ -shared swig/python/sdbf_wrap.o -fopenmp -L./external/stage/lib -Wl,--whole-archive -lboost_system -lboost_filesystem -lboost_thread -Wl,--no-whole-archive -lpython2.7 libsdbf.a -o swig/python/_sdbf_class.so -lcrypto -lpthread 

sdbf.i:

$(LIBSDBF): $(SDBF_OBJ) 
	ar r $(LIBSDBF) $(SDBF_OBJ)

stream: $(SDHASH_OBJ) $(LIBSDBF)
	$(LD) $(SDHASH_OBJ) $(SDHASH_CLIENT_OBJ) $(LIBSDBF) -o sdhash $(LDFLAGS) 

bloom-test: $(BLOOM_TEST_OBJ) $(LIBSDBF)
	$(LD) $(BLOOM_TEST_OBJ) $(LIBSDBF) -o bloom-test $(LDFLAGS) 

boost: 
	cd external ; ./bootstrap.sh --with-python=python2 ; ./b2 link=static cxxflags='-fPIC -std=c++0x' ; cd -

server:
	make -C ./sdhash-server -f Makefile

gpu:
	make -C ./sdhash-gpu -f Makefile

# Longest common substring
#lcs: lcs.c map_file.c error.c
#	gcc -std=c99 -O3 -o lcs lcs.c map_file.c error.c

#pcap: sdhash-pcap.c
#	gcc -I/usr/include/pcap -o sdhash-pcap sdhash-pcap.c -lpcap
	
clean:
	-@rm *.o sdhash sdhash-cli sdhash-srv 2> /dev/null || true
	-@rm sdhash-src/*.o sdbf/*.o 2> /dev/null || true
	-@rm base64/*.o 2> /dev/null || true
	-@rm lz4/*.o 2> /dev/null || true
	-@rm libsdbf.a 2> /dev/null || true

veryclean: clean
	cd external; ./b2 --clean ; cd -

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.cc -o $*.o
