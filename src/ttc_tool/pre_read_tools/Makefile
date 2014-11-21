include ../../Make.conf

VPATH = ./:../../common:../../stat
################compile###############
target = pre_read keydump 

CFLAGS += -I./ -I../../api/c_api
TTC_API_LIB = ../../api/c_api/libttc.so

pre_read_list:= pre_read
pre_read_objs := $(patsubst %,tmp.%.o,$(pre_read_list))
pre_read_libs := -lpthread -Wl,-rpath,\$$ORIGIN/../../lib/ -Wl,-rpath,\$$ORIGIN  -Wl,-rpath,\$$ORIGIN/../../api/c_api  -Wl,-rpath,\$$ORIGIN/../../ -z origin -rdynamic -lrt $(TTC_API_LIB) -lz
pre_read: LDFLAGS += -pthread 

keydump_objs := kd_syncer.o keydump.o
keydump_libs := -lpthread  -Wl,-rpath,\$$ORIGIN/../../lib/ -Wl,-rpath,\$$ORIGIN  -Wl,-rpath,\$$ORIGIN/../../api/c_api  -Wl,-rpath,\$$ORIGIN/../../ -z origin -rdynamic $(TTC_API_LIB) -lz
keydump : LDFLAGS += -pthread



################install##############
target_install = pre_read keydump
install_dir = ../../../bin


include ../../Make.rules
