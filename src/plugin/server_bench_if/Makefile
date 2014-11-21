include ../../Make.conf

################compile#############
target = sb_interface.so plugin_http.so

CFLAGS += -DDEBUG -I../../common -I../../3rdparty/jsoncpp/include -I../../api/c_api

sb_interface_objs := log.o config.o util.o tcp.o
sb_interface: LDFLAGS += -Wl,-soname=sb_interface.so

plugin_http_objs := plugin_http.o http_plugin_request_msg.o http_plugin_error_code.o http_plugin_response_msg.o plugin_operate.o
plugin_http_libs := -L../../common -lcommon ../$(JSON_LIB) ../../api/c_api/libdtc.so
plugin_http: LDFLAGS += -fPIC -shared -Wl,-soname=plugin_http.so

###############install##############
target_install = sb_interface.so plugin_http.so benchapi.h
install_dir = ../../../lib
%.h: install_dir = ../../../include

include ../../Make.rules
