include ../Make.conf

################compile###############
target = hotbackup hbp_status

CFLAGS+=-I../common -I../api/c_api

LIBTTCAPI := -L../api/c_api -ldtc -lpthread -Wl,-rpath,\$$ORIGIN/../lib/ -Wl,-rpath,\$$ORIGIN  -Wl,-rpath,\$$ORIGIN/../api/  -Wl,-rpath,\$$ORIGIN/../ -z origin
hotbackup_list:=  async_file comm full_sync incsync_key incsync_value main sync_unit daemon registor
hotbackup_objs := $(patsubst %,%.o,$(hotbackup_list))
hotbackup_libs := -L../common -lcommon $(LIBTTCAPI)
hotbackup: LDFLAGS += -pthread


hbp_status_list:=  async_file hbp_status
hbp_status_objs:= $(patsubst %,%.o,$(hbp_status_list))
hbp_status_libs := -L../common -lcommon $(LIBTTCAPI)
hbp_status: LDFLAGS += -pthread

################install##############
target_install = hotbackup hbp_status
install_dir = ../../bin


include ../Make.rules
