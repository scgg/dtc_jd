include ../Make.conf

target = libcommon.a libcommon_nopic.a
CFLAGS+=-I../api/c_api -I. -I../stat -I../3rdparty/attr_api $(ZINC)
LIBS = $(Z_LIB)

GIT_VERSION=$(shell git log | head -n 4 | grep "commit" | cut -d " " -f 2 | cut -c 1-7)
$(shell sed -i '11s/"[^"]*"/"$(GIT_VERSION)"/' version.h)


filelist := md5 value decode encode field section tabledef \
	packet_base packet_server packet_result \
	task_const task_base task_pkey task_server task_copy task_request task_api \
	poller timerlist config version unix_lock log log_stat log_alert \
	shmem memcheck dbconfig dbconfig_tdef admin_tdef \
	daemon daemon_wait proctitle thread poll_thread poll_thread_group thread_cpu_stat \
	buffer localip \
	key_guard bitsop sockaddr sockbind blacklist blacklist_unit \
	multi_request task_multiplexer \
	helper_client helper_collect helper_group \
	listener listener_pool decoder_base \
	client_sync client_async client_unit client_dgram  \
	plugin_request plugin_mgr plugin_dgram plugin_sync \
	plugin_worker plugin_unit plugin_listener_pool \
	plugin_global plugin_decoder plugin_timer ttc_global\
	compress client_resource_pool \
	new_hash dynamic_helper_collection consistent_hash_selector chash\
	file_backed_key_set MarkupSTL parse_cluster_config MarkupSTL key_route \
	agent_client agent_client_unit agent_listener agent_listen_pool agent_process agent_receiver agent_sender \
	agent_multi_request plugin_agent_mgr plugin_agent_listener_pool

libcommon_objs := $(filelist:%=%_pic.o)
%_pic.o: CFLAGS  += -pthread -fPIC

libcommon_nopic_objs := $(filelist:%=%.o)
%.o: CFLAGS  += -pthread

include ../Make.rules

