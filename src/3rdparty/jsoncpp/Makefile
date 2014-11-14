include ../../Make.conf

target = libjsoncpp.a
LIBS = -lz

CFLAGS+=-I./include

VPATH := ./src
filelist := json_reader json_value json_writer

libjsoncpp_objs := $(filelist:%=%.o)
%.o: CFLAGS  += -pthread

include ../../Make.rules

