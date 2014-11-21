include ../../Make.conf

JSON_LIB :=../../3rdparty/jsoncpp/lib64/libjsoncpp.a
ifeq ($(findstring x86_64, $(PLATFORM)),) 
	CURL_LIB := ../../3rdparty/curl/lib/libcurl32.a -lrt
else
   	CURL_LIB := ../../3rdparty/curl/lib/libcurl64.a -lrt 
endif
################compile###############
target = cplusTplPreTest 

CFLAGS += -I../../api/c_api -I ../../cache -I ../../common -I ../../stat/
TTC_API_LIB = ../../3rdparty/dtc_api/libdtc-gcc-4.4-r7351daa.so

cplusTplPreTest_list:= dtcbench dtc_t_dtc_example dtctest threadwraper 
cplusTplPreTest_objs:= $(patsubst %,%.o,$(cplusTplPreTest_list))
cplusTplPreTest_libs:=../../common/libcommon.a $(TTC_API_LIB) -lz -lpthread -lrt -L/usr/lib64 $(CURL_LIB)
cplusTplPreTest: LDFLAGS += -pthread 

################install##############
#target_install = dtcbench_tool
#install_dir = ../../../../bin

include ../../Make.rules
