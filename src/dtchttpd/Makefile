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
GIT_VERSION=$(shell git log | head -n 4 | grep "commit" | cut -d " " -f 2)
$(shell sed -i '10s/"[^"]*"/"$(GIT_VERSION)"/' dtchttpd_version.h)

VPATH := ../3rdparty/tinyxml ../3rdparty/http_service
CFLAGS+=-DSVN_REVISION=$(SVN_REVISION) $(PROTOBUFINC) -I../3rdparty/http_service -I../common -I../stat -I../3rdparty/net $(CURLINC) -I../3rdparty/tinyxml -I../api/c_api -I../3rdparty/jsoncpp/include/
CFLAGS+=$(MYSQLINC)
CXXFLAGS+=-Wno-sign-compare
TTC_API_LIB = ../3rdparty/dtc_api/libdtc-gcc-4.4-rf588dad.so
#TTC_API_LIB = ../api/c_api/libdtc.so
#CFLAGS+=-DSVN_REVISION=$(SVN_REVISION) $(PROTOBUFINC) -Wno-sign-compare -DDEBUG -g

target = dtchttpd

dtchttpd_list := stream_receiver sender http_client_response_decoder main fd_transfer session \
		 storage_manager config http_agent_request_msg http_agent_response_msg http_agent_error_code \
		 join msg_parser mysql_manager fk_config dtchttpd_alarm curl_http group_router \
		 helper_client helper_group 
dtchttpd_objs := $(patsubst %,%.o,$(dtchttpd_list))
dtchttpd_libs := -L../common -lcommon $(JSON_LIB) $(MYSQLLIB) $(CURL_LIB)
dtchttpd: LDFLAGS+=-pthread

################install##############
target_install = dtchttpd

				 
install_dir = ../../agent_bin
include ../Make.rules
