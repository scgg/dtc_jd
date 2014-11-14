include ../Make.conf

################compile###############
target = ttcdump ttcrecovery

CFLAGS+=-I../api/c_api -I../common

ttcdump_objs := ttcdump.o
ttcdump_libs := ../common/libcommon.a -L../api/c_api -ldtc -lpthread -Wl,-rpath,\$$ORIGIN/../lib/ -Wl,-rpath,\$$ORIGIN  -Wl,-rpath,\$$ORIGIN/../api/  -Wl,-rpath,\$$ORIGIN/../ -z origin -rdynamic
ttcdump : LDFLAGS += -pthread


ttcrecovery_objs := ttcrecovery.o
ttcrecovery_libs := ../common/libcommon.a -L../api/c_api -ldtc -lpthread -Wl,-rpath,\$$ORIGIN/../lib/ -Wl,-rpath,\$$ORIGIN  -Wl,-rpath,\$$ORIGIN/../api/ -Wl,-rpath,\$$ORIGIN/../ -z origin -rdynamic
ttcrecovery : LDFLAGS += -pthread

################install##############
target_install = ttcdump ttcrecovery
install_dir = ../../bin


include ../Make.rules
