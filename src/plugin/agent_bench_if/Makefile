include ../../Make.conf

################compile#############
target = sb_interface.so

CFLAGS += -DDEBUG

sb_interface_objs := log.o config.o util.o tcp.o
LDFLAGS += -Wl,-soname=sb_interface.so

SUBDIRS = http

###############install##############
target_install = sb_interface.so benchapi.h
install_dir = ../../../lib
%.h: install_dir = ../../../include

include ../../Make.rules
