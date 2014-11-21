include ../Make.conf
################compile###############
target = ttcd_test 

CFLAGS += -I../api/c_api -I ../cache -I ../common -I ../stat/ $(CURLINC)


ttcd_test_list:= ttcd_test 
ttcd_test_objs:= $(patsubst %,%.o,$(ttcd_test_list))
ttcd_test_libs:= ../common/libcommon.a $(TTC_API_LIB) $(Z_LIB) -lpthread -lrt $(CURL_LIB)
ttcd_test: LDFLAGS += -pthread 

################install##############
target_install = ttcd_test 
install_dir = ../../bin


include ../Make.rules
