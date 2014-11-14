include ../Make.conf
################compile###############
SVN_REVISION = $(shell test -d .git && (git svn info | grep "Last Changed Rev" | cut -d " " -f 4))

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = $(shell test -d .svn && (svn info | grep "Last Changed Rev" | cut -d " " -f 4))
endif

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = "(unknown)"
endif
#增加版本及编译基本信息
GIT_VERSION=$(shell git log | head -n 4 | grep "commit" | cut -d " " -f 2 | cut -c 1-7)
$(shell sed -i '11s/"[^"]*"/"$(GIT_VERSION)"/' Version.h)

DATE=$(shell git show | head -n 4 | grep Date | cut -b 9-30)
$(shell sed -i '12s/"[^"]*"/"$(DATE)"/' Version.h)

AUTHOR=$(shell git show | head -n 4 | grep Author | cut -d " " -f 2)
$(shell sed -i '13s/"[^"]*"/"$(AUTHOR)"/' Version.h)

COMPILEDATE=$(shell date)
$(shell sed -i '14s/"[^"]*"/"$(COMPILEDATE)"/' Version.h)

VPATH := ../3rdparty/tinyxml ../3rdparty/http_service
CFLAGS+=-DSVN_REVISION=$(SVN_REVISION) $(PROTOBUFINC) -I../3rdparty/http_service -I../common -I../stat -I../3rdparty/net $(CURLINC) -I../3rdparty/tinyxml -I../api/c_api -I../3rdparty/jsoncpp/include/
CFLAGS+=$(MYSQLINC)
CXXFLAGS+=-Wno-sign-compare
#TTC_API_LIB = ../api/c_api/libdtc.so
#CFLAGS+=-DSVN_REVISION=$(SVN_REVISION) $(PROTOBUFINC) -Wno-sign-compare -DDEBUG -g

target = dtcagent agent_stat_tool agent_add_cache_server agent_remove_cache_server \
		 agent_change_cache_server_addr agent_add_module agent_remove_module \
		 agentm_verfy dtcmonitor agentmonitor agent_checkalive agent_dump_config

dtcagent_list := back_worker front_worker main module_acceptor receiver sender module cache_server \
	business_module admin_acceptor admin_worker stat_agent tinystr tinyxmlparser tinyxmlerror tinyxml config_file \
	admin_protocol.pb sock_util heartbeat hitinfo agent_alert curl_http CFdDispacher
dtcagent_objs := $(patsubst %,%.o,$(dtcagent_list)) ../stat/libstat.a $(PROTOBUFLIB)
dtcagent_libs := -L../common -lcommon_nopic -ldl -lrt $(JSON_LIB) $(CURL_LIB)
dtcagent: LDFLAGS+=-pthread

agent_stat_tool_list := agent_stat_tool tinystr tinyxmlparser tinyxmlerror tinyxml curl_http
agent_stat_tool_objs := $(patsubst %,%.o,$(agent_stat_tool_list)) ../stat/libstat.a $(PROTOBUFLIB) 
agent_stat_tool_libs := -L../common -lcommon $(JSON_LIB) $(CURL_LIB) 

agent_add_cache_server_list := agent_add_cache_server admin_protocol.pb
agent_add_cache_server_objs := $(patsubst %,%.o,$(agent_add_cache_server_list)) $(PROTOBUFLIB)
agent_add_cache_server_libs := -L../common -lcommon -lpthread

agent_remove_cache_server_list := agent_remove_cache_server admin_protocol.pb
agent_remove_cache_server_objs := $(patsubst %,%.o,$(agent_remove_cache_server_list)) $(PROTOBUFLIB)
agent_remove_cache_server_libs := -L../common -lcommon -lpthread

agent_change_cache_server_addr_list := agent_change_cache_server_addr admin_protocol.pb
agent_change_cache_server_addr_objs := $(patsubst %,%.o,$(agent_change_cache_server_addr_list)) $(PROTOBUFLIB)
agent_change_cache_server_addr_libs := -L../common -lcommon -lpthread

agent_add_module_list := agent_add_module admin_protocol.pb
agent_add_module_objs := $(patsubst %,%.o,$(agent_add_module_list)) $(PROTOBUFLIB)
agent_add_module_libs := -L../common -lcommon -lpthread

agent_remove_module_list := agent_remove_module admin_protocol.pb
agent_remove_module_objs := $(patsubst %,%.o,$(agent_remove_module_list)) $(PROTOBUFLIB)
agent_remove_module_libs := -L../common -lcommon -lpthread

agentm_verfy_list := agentm_verfy sock_util admin_protocol.pb tinystr tinyxmlparser tinyxmlerror tinyxml curl_http
agentm_verfy_objs := $(patsubst %,%.o,$(agentm_verfy_list)) $(PROTOBUFLIB) 
agentm_verfy_libs :=-L../common -lcommon -lpthread $(JSON_LIB) $(CURL_LIB) 

agent_checkalive_list := agent_checkalive stat_agent sock_util admin_protocol.pb tinystr tinyxmlparser tinyxmlerror tinyxml curl_http
agent_checkalive_objs := $(patsubst %,%.o,$(agent_checkalive_list)) ../stat/libstat.a $(PROTOBUFLIB) 
agent_checkalive_libs :=-L../common -lcommon -lpthread $(JSON_LIB) $(CURL_LIB) 

memstat_list := memstat tinystr tinyxmlparser tinyxmlerror tinyxml curl_http
memstat_objs := $(patsubst %,%.o,$(memstat_list)) $(PROTOBUFLIB)
memstat_libs := -L../common -lcommon $(TTC_API_LIB) $(Z_LIB) -lpthread -lrt $(JSON_LIB) $(CURL_LIB) 

dtcmonitor_list := dtcinstancemonitor curl_http unix_lock
dtcmonitor_objs := $(patsubst %,%.o,$(dtcmonitor_list))
dtcmonitor_libs :=-L../common -lcommon -lpthread $(TTC_API_LIB) $(JSON_LIB) $(CURL_LIB) $(MYSQLLIB)

agentmonitor_list := agentmonitor curl_http unix_lock sock_util admin_protocol.pb
agentmonitor_objs := $(patsubst %,%.o,$(agentmonitor_list)) $(PROTOBUFLIB) 
agentmonitor_libs :=-L../common -lcommon -lpthread $(TTC_API_LIB) $(JSON_LIB) $(CURL_LIB) $(MYSQLLIB)

agent_dump_config_list := agent_dump_config admin_protocol.pb
agent_dump_config_objs := $(patsubst %,%.o,$(agent_dump_config_list)) $(PROTOBUFLIB)
agent_dump_config_libs := -L../common -lcommon -lpthread

################install##############
target_install = dtcagent agent_stat_tool agent_add_cache_server agent_remove_cache_server \
				 agent_change_cache_server_addr agent_add_module agent_remove_module \
				 dtcmonitor agent_test_deamon.sh agentmonitor_start.sh dtcmonitor_start.sh \
				 agent_stat_report.sh agent.sh dtccrontab_start.sh agentm_verfy agent_checkalive agent_dump_config

				 
install_dir = ../../agent_bin
include ../Make.rules
