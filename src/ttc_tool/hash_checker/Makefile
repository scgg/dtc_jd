include ../../Make.conf

################compile###############
target = hashchecker

CFLAGS += -I./ -I../../plugin/key_hash/ -std=c++0x
VPATH := ../../plugin/key_hash/
#CFLAGS += -I./

hashchecker_list:= hashchecker KeyHash
#hashchecker_list:= hashchecker
#hashchecker_list:= hashchecker
hashchecker_objs:= $(patsubst %,%.o,$(hashchecker_list))
hashchecker: LDFLAGS += -pthread 

################install##############
target_install = hashchecker_tool
install_dir = ../../../bin

include ../../Make.rules
