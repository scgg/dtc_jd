include ../../Make.conf

VPATH = ./:../../common:../../stat
################compile###############
target = ttc_diff 

CFLAGS += -I./ -I../../api -I../../common -I../../cache/ -I../../stat -I../../watchdog -I../../helper -I../purge_tool
TTC_API_LIB = ../../api/libttc.a ../../common/libcommon.a -lz

ttc_diff_list:= ttc_diff
ttc_diff_objs:= $(patsubst %,%.o,$(ttc_diff_list))
ttc_diff_libs:= $(TTC_API_LIB) -lrt
ttc_diff: LDFLAGS += -pthread 

################install##############
target_install = ttc_diff_tool
install_dir = ../../../bin

include ../../Make.rules
