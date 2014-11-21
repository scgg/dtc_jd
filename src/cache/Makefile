include ../Make.conf
VPATH :=../helper ../stat ../3rdparty/http_service

####################compile#################
CFLAGS += -I./ -I../common -I../api/c_api -I../stat -I../watchdog -I../helper -I../cold_backup -I../3rdparty/http_service
LIBPATH := -L. -L../common -L../watchdog -L../stat -L../3rdparty/attr_api -L../api
DB%.o tmp.DB%.o:CFLAGS += $(MYSQLINC)
ifneq ($(findstring x86_64,$(PLATFORM)),)
BITS=64
else
BITS=32
endif

LIBTTCAPI := -L../api ../api/c_api/libdtc.so -lpthread -Wl,-rpath,\$$ORIGIN/../lib/ -Wl,-rpath,\$$ORIGIN  -Wl,-rpath,\$$ORIGIN/../api/  -Wl,-rpath,\$$ORIGIN/../ -z origin

target = libttcd.a dtcd shmtool mem_tool check_fix_tool mem_defragment
target_external = ../api/libdtc.a ../stat/libstat.a ../common/libcommon.a

$(filterout libttcd.a,$(target)): libttcd.a;

filelist := feature hash ng_info node_group node_index barrier_unit cache_bypass cache_pool bin_malloc sys_malloc raw_data raw_data_process raw_data_process2 cache_process cache_flush cache_unit empty_filter black_hole  logger task_pendlist lru_bit hb_log admin_process hb_feature container_ttcd
libttcd_objs:= $(sort $(filelist:%=%.o))

#shmtool
shmtool_objs:= shmtool.o
shmtool_libs:= -lcommon -lpthread

#ttcd
dtcd: CFLAGS += -export-dynamic
dtcd: LDFLAGS += -Wl,--version-script,ttcd.export.lst
dtcd_objs:= main.o svr_control.o trunner.o StatClient.o curl_http.o
dtcd_libs:= -lstat -lwatchdog  -lttcd -lcommon -lattr_api_$(BITS) -lpthread -ldl $(Z_LIB) -rdynamic $(JSON_LIB) $(CURL_LIB)

#mem_tool
mem_tool_list= mem_tool DBReader DBWriter TxtReader TxtWriter TxtEscape cache_reader cache_writer \
			   cache_pool raw_data raw_data_process  tmp.DBConn \
	       feature hash ng_info node_group node_index bin_malloc sys_malloc empty_filter

mem_tool_objs += $(mem_tool_list:%=%.o)
mem_tool_libs:= -lstat -lcommon $(MYSQLLIB) -lpthread -ldl $(Z_LIB)

#check_fix_tool
check_fix_tool_list =   check_fix_tool \
						check_hash_node_info \
						check_lru_node_info \
						check_result \
						check_output \
						check_logic \
						check_cache_holder \
						bin_malloc \
						feature \
						hash \
						node_index \
						ng_info \
						node_group \
						raw_data \
						../common/dbconfig
check_fix_tool_objs += $(check_fix_tool_list:%=%.o)
check_fix_tool_libs:= -lstat -lcommon -lpthread -ldl

#mem_defragment
mem_defragment_list =   defragment_main \
						defragment\
						bin_malloc\
						../common/dbconfig
mem_defragment_objs += $(mem_defragment_list:%=%.o)
mem_defragment_libs:= -lstat -lcommon -lpthread -ldl $(LIBTTCAPI)

#####################install############
target_install = shmtool dtcd mem_tool check_fix_tool mem_defragment
install_dir = ../../bin

include ../Make.rules
