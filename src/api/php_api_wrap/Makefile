include ../../Make.conf

SUBDIRS := php_qqvip

BIN = test speed

all: $(BIN)

test: test.o ttc_php_api.o ttc_php_api.h
	g++ -g -Wall -o test test.o ttc_php_api.o ../../bin/libdtc.so

speed: test_speed.o ttc_php_api.o ttc_php_api.h
	g++ -g -Wall -o speed test_speed.o ttc_php_api.o ../../bin/libdtc.so

%.o: %.cc
	g++ -g -Wall -DDEBUG_M -I../../include -c -o $@ $<

clean:
	rm -rf *.o $(BIN)
 
include ../../Make.rules
