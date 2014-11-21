include ../../Make.conf

VPATH = ./:../../common:../../stat
################compile###############
target = purge_tool 

CFLAGS += -I./ -I../../api/c_api
TTC_API_LIB = ../../api/c_api/libttc.so


purge_tool_list:= purge_tool 
purge_tool_objs:= $(patsubst %,tmp.%.o,$(purge_tool_list))
purge_tool_libs:= -lpthread -Wl,-rpath,\$$ORIGIN/../../lib/ -Wl,-rpath,\$$ORIGIN  -Wl,-rpath,\$$ORIGIN/../../api/c_api  -Wl,-rpath,\$$ORIGIN/../../ -z origin -rdynamic -lrt $(TTC_API_LIB)
purge_tool: LDFLAGS += -pthread 

################install##############
target_install = pre_read 
install_dir = ../../../bin


include ../../Make.rules
