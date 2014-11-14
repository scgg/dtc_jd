include ../Make.conf

################compile###############
target = time_hotbackup
CFLAGS = -I. -I ../common -I ../api/c_api  -I ../cache -I ../stat

LIBTTCAPI := -L../api/c_api -ldtc -lpthread -Wl,-rpath,\$$ORIGIN/../lib/ -Wl,-rpath,\$$ORIGIN  -Wl,-rpath,\$$ORIGIN/../api/  -Wl,-rpath,\$$ORIGIN/../ -z origin
time_hotbackup_list :=async_file comm full_sync incsync_key incsync_value main sync_unit daemon registor
time_hotbackup_objs := $(patsubst %,%.o,$(time_hotbackup_list))
time_hotbackup_libs := -L../common -lcommon -ldl $(LIBTTCAPI) 
time_hotbackup: LDFLAGS += -pthread

################install##############
target_install = time_hotbackup 
install_dir = ../../bin


include ../Make.rules
