include ../../Make.conf

JSON_LIB :=../../3rdparty/jsoncpp/lib64/libjsoncpp.a
ifeq ($(findstring x86_64, $(PLATFORM)),) 
	CURL_LIB := ../../3rdparty/curl/lib/libcurl32.a -lrt
else
   	CURL_LIB := ../../3rdparty/curl/lib/libcurl64.a -lrt 
endif
################compile###############
target = dtcbench 

CFLAGS += -I../../api/c_api -I ../../cache -I ../../common -I ../../stat/
TTC_API_LIB = ../../3rdparty/dtc_api/libdtc-gcc-4.4-rf588dad.so

dtcbench_list:= dtcbench dtctest threadwraper
dtcbench_objs:= $(patsubst %,%.o,$(dtcbench_list))
dtcbench_libs:=../../common/libcommon.a $(TTC_API_LIB) -lz -lpthread -lrt -L/usr/lib64 $(CURL_LIB)
dtcbench: LDFLAGS += -pthread 

################install##############
target_install = dtcbench_tool
install_dir = ../../../bin

include ../../Make.rules
