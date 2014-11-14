include ../../../Make.conf

################compile#############
target = lib_dtc_java_api.so
# CLIENTAPI macro use for scope test only
CFLAGS += -DCLIENTAPI
CFLAGS += -I/usr/lib/jvm/default-java/include/linux \
		  -I/usr/lib/jvm/default-java/include \
		  -I./include \
		 
# for auto ln -sf
lib_dtc_java_api_objs := java_dtc_comm.o java_dtc_server.o java_dtc_request.o java_dtc_result.o
lib_dtc_java_api_libs := ../../c_api/libdtc.so
lib_dtc_java_api : LDFLAGS += -fPIC -shared -Wl,-soname=lib_dtc_java_api.so

target_install = lib_dtc_java_api.so
install_dir = ../../../../bin

include ../../../Make.rules
