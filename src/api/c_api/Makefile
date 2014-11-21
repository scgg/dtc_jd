LIB_PATH = ../..

include ../../Make.conf

GIT_VERSION=$(shell git log . | head -n 4 | grep "commit" | cut -d " " -f 2 | cut -c 1-7)

VPATH = ../../common ../../3rdparty/tinyxml ../../3rdparty/http_service
################compile#############
target = libdtc.a container_api.pic.o version.pic.o somain.pic.o libdtc.so libdtc.pic.a testapi

# CLIENTAPI macro use for scope test only
CFLAGS += -DCLIENTAPI
CFLAGS += -pthread -I../../common -I../../stat  -I../../3rdparty/http_service $(ZINC) $(JSONCPPINC) $(CURLINC) 
LIBS = $(Z_LIB) -ldl -lpthread $(JSON_LIB) $(CURL_LIB)


filelist := ttcreq ttcsrv ttcpool ttcwrap ttcwrapp \
	    poller timerlist keylist tabledef \
	    log_client memcheck md5 value \
	    section decode encode field_api \
	    packet_base packet_client \
	    task_base task_const sockaddr udppool compress buffer curl_http thread


#lidttc.so: container_api.pic.o version.pic.o

# for auto ln -sf
libdtc_objs := $(patsubst %,%.o,$(filelist))
libdtc.so: LDFLAGS += -Wl,--version-script,ttcapi.lst -e _so_start container_api.pic.o version.pic.o somain.pic.o
libdtc_soname := libdtc-gcc-$(GCCVER)-r$(GIT_VERSION).so

testapi_list := testapi
testapi_objs := $(patsubst %,%.o,$(testapi_list))
testapi_libs := -L../../common -lcommon ./libdtc.so -lpthread -Wl,-rpath,\$$ORIGIN/../lib/ -Wl,-rpath,\$$ORIGIN  -Wl,-rpath,\$$ORIGIN/../api/  -Wl,-rpath,\$$ORIGIN/../ -z origin
testapi: LDFLAGS+=-pthread

###############install##############
target_install = libdtc.a libdtc.pic.a libdtc.so ttcapi.h testapi
install_dir = ../../../bin
%.a: install_dir = ../../../lib
%.h: install_dir = ../../../include

include ../../Make.rules

