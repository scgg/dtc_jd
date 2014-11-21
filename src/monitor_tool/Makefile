include ../Make.conf

VPATH := ../3rdparty/tinyxml ../3rdparty/http_service
CFLAGS+=-DSVN_REVISION=$(SVN_REVISION) $(PROTOBUFINC) -I../3rdparty/http_service -I../common -I../stat -I../3rdparty/net $(CURLINC) -I../3rdparty/tinyxml -I../api/c_api -I../3rdparty/jsoncpp/include/
CFLAGS+=$(MYSQLINC)
CXXFLAGS+=-Wno-sign-compare
target = dtcmonitor agentmonitor

dtcmonitor_list := dtcinstancemonitor curl_http unix_lock
dtcmonitor_objs := $(patsubst %,%.o,$(dtcmonitor_list))
dtcmonitor_libs :=-L../common -lcommon -lpthread $(TTC_API_LIB) $(JSON_LIB) $(CURL_LIB) $(MYSQLLIB)

agentmonitor_list := agentmonitor curl_http unix_lock sock_util admin_protocol.pb
agentmonitor_objs := $(patsubst %,%.o,$(agentmonitor_list)) $(PROTOBUFLIB) 
agentmonitor_libs :=-L../common -lcommon -lpthread $(TTC_API_LIB) $(JSON_LIB) $(CURL_LIB) $(MYSQLLIB)

################install##############
target_install = dtcmonitor agentmonitor_start.sh dtcmonitor_start.sh agentmonitor install.sh
install_dir = ../../monitor_tool_bin/
include ../Make.rules