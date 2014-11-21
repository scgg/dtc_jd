include ../Make.conf
################compile###############
SVN_REVISION = $(shell test -d .git && (git svn info | grep "Last Changed Rev" | cut -d " " -f 4))

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = $(shell test -d .svn && (svn info | grep "Last Changed Rev" | cut -d " " -f 4))
endif

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = "(unknown)"
endif
GIT_VERSION=$(shell git log . | head -n 4 | grep "commit" | cut -d " " -f 2 | cut -c 1-7)
$(shell sed -i '8s/"[^"]*"/"$(GIT_VERSION)"/' dtcCodeFactoryd_version.h)

VPATH := ../3rdparty/tinyxml ../3rdparty/http_service
CFLAGS+=-DSVN_REVISION=$(SVN_REVISION) $(PROTOBUFINC) -I../3rdparty/http_service -I../common -I../stat -I../3rdparty/net $(CURLINC) -I../3rdparty/tinyxml -I../3rdparty/jsoncpp/include/
CFLAGS+=$(MYSQLINC)
CXXFLAGS+=-Wno-sign-compare
#TTC_API_LIB = ../api/c_api/libdtc.so
CFLAGS+=-DSVN_REVISION=$(SVN_REVISION) $(PROTOBUFINC) -Wno-sign-compare -DDEBUG -g

target = dtcCodeFactoryd

dtcCodeFactoryd_list := stream_receiver sender http_client_response_decoder worker main fd_transfer session \
			http_agent_request_msg http_agent_response_msg http_agent_error_code \
		 	join mysql_manager fk_config dtcCodeFactoryd_alarm curl_http
dtcCodeFactoryd_objs := $(patsubst %,%.o,$(dtcCodeFactoryd_list))
dtcCodeFactoryd_libs := -L../common -lcommon $(JSON_LIB) $(MYSQLLIB) $(CURL_LIB)
dtcCodeFactoryd: LDFLAGS+=-pthread

################install##############
target_install = dtcCodeFactoryd

				 
install_dir = ../../codeFactoryd_bin
include ../Make.rules
