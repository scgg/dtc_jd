TDB = ../tdb
CXX	= g++
LINK	= -L$(TDB) -ltdb_handle -lasn1c++ -ldl -rdynamic -lrt 
INC	= -I. -I$(TDB)
CXXFLAGS= -g -Wall -pipe -fno-ident -shared -D_GNU_SOURCE -D_REENTRNT -fPIC $(INC)

SO_OBJ = TDBConn.o TDBProcess.o
SO_LIB = tdb-helper-service.so
LINK_HEADER=CommProcess.h

all: $(SO_LIB)

$(SO_LIB): $(SO_OBJ)
	g++ -o $@ $^ $(CXXFLAGS) $(LINK)

%.o: %.cm
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf *.o $(SO_LIB)

install: all
	cp $(SO_LIB) ../../../bin/
