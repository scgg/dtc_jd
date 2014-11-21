include ../Make.conf

#################compile ############
target = libstat.a stattool

CFLAGS+=-I../common -I../3rdparty/attr_api -I../3rdparty/http_service -I../3rdparty/jsoncpp/include/ $(CURLINC)
VPATH := ../3rdparty/http_service
ifneq ($(findstring x86_64,$(PLATFORM)),)
BITS=64
else
BITS=32
endif

filelist := StatMgr StatTTC StatThread StatTTCDef StatClient StatAttn formater StatManagerContainerThread DtcStatAlarmReporter curl_http  StatTool
libstat_objs := $(filelist:%=%.o)
libstat_objs: CFLAGS += -pthread


stattool_src := StatTool formater StatMgr StatAttn StatClient StatTTCDef DtcStatAlarmReporter curl_http
stattool_objs := $(stattool_src:%=%.o) ../3rdparty/attr_api/libattr_api_$(BITS).a
stattool_libs := -L../common -lcommon $(JSON_LIB) $(CURL_LIB) -ldl

##################install #############
target_install = stattool
install_dir = ../../bin

include ../Make.rules

