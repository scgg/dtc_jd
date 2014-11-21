include ../Make.conf

################version###############
GIT_VERSION=$(shell git log | head -n 4 | grep "commit" | cut -d " " -f 2 | cut -c 1-7)
$(shell sed -i '6s/"[^"]*"/"$(GIT_VERSION)"/' version.h)

################compile###############
target = dtcproberd

CFLAGS += -I./  -I../3rdparty/tinyxml -I../3rdparty/jsoncpp/include -I../3rdparty/segvcatch -I../3rdparty/http_service -I../common -I../stat -I../cache -I../dtchttpd -fnon-call-exceptions
VPATH := ../3rdparty/tinyxml ../3rdparty/segvcatch ../3rdparty/http_service ../common ../dtchttpd ../cache
LIBPATH := -L../common -L../stat -L../cache -L../3rdparty/attr_api
ifneq ($(findstring x86_64,$(PLATFORM)),)
BITS=64
else
BITS=32
endif

dtcproberd_list:= dtcproberd segvcatch stream_receiver sender http_client_response_decoder http_agent_request_msg \
		  http_agent_response_msg http_agent_error_code map_reduce http_server \
		  tinystr tinyxmlparser tinyxmlerror tinyxml memory_usage_cmd history_node_size_distribute_cmd history_node_survival_time_distribute_cmd history_op_type_distribute_cmd \
		  current_node_size_distribute_cmd current_row_size_distribute_cmd current_get_count_distribute_cmd current_last_acc_distribute_cmd current_dirty_clean_percent_cmd \
		  current_dirty_distribute_cmd current_node_survival_time_distribute_cmd cache_memory_info prober_cmd
dtcproberd_objs:= $(patsubst %,%.o,$(dtcproberd_list))
dtcproberd_libs:= -lttcd -lstat -lcommon -ldl $(JSON_LIB) $(CURL_LIB) -lattr_api_$(BITS) $(Z_LIB)
dtcproberd: LDFLAGS += -pthread

################install##############
target_install = dtcproberd
install_dir = ../../bin

include ../Make.rules
