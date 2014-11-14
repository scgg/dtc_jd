TOPDIR = ../../..
include $(TOPDIR)/Make.conf

VPATH = $(TOPDIR)/api/c_api $(TOPDIR)/common $(TOPDIR)/3rdparty/tinyxml $(TOPDIR)/3rdparty/http_service
################compile#############
target = plugin_http.so

filelist := ttcreq ttcsrv ttcpool ttcwrap ttcwrapp \
            poller timerlist keylist tabledef \
            log_client memcheck md5 value \
            section decode encode field_api \
            packet_base packet_client \
            task_base task_const sockaddr udppool compress buffer curl_http thread


CFLAGS += -DDEBUG -I$(TOPDIR)/common -I$(TOPDIR)/3rdparty/jsoncpp/include \
	-I$(TOPDIR)/api/c_api -I../ \
	-I$(TOPDIR)/stat  -I$(TOPDIR)/3rdparty/http_service $(ZINC) $(JSONCPPINC) $(CURLINC)

plugin_http_objs := plugin_http.o plugin_http.o http_plugin_request_msg.o http_plugin_error_code.o http_plugin_response_msg.o httptool.o \
		$(patsubst %,%.o,$(filelist))

plugin_http_libs := -L$(TOPDIR)/common -lcommon ../../$(JSON_LIB) -ldl -lpthread ../../$(Z_LIB) ../../$(CURL_LIB)

LDFLAGS += -fPIC -shared -Wl,-soname=plugin_http.so

###############install##############
target_install = plugin_http.so
install_dir = ../../../../lib

include $(TOPDIR)/Make.rules
