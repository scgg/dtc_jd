include ../Make.conf
################compile###############
SVN_REVISION = $(shell test -d .git && (git svn info | grep "Last Changed Rev" | cut -d " " -f 4))

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = $(shell test -d .svn && (svn info | grep "Last Changed Rev" | cut -d " " -f 4))
endif

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = "(unknown)"
endif

VPATH := ../3rdparty/net

CFLAGS+=-DSVN_REVISION=$(SVN_REVISION) $(PROTOBUFINC) -I../3rdparty/net -I../common
CFLAGS+=$(MYSQLINC)
CXXFLAGS+=-Wno-non-virtual-dtor -Wno-sign-compare

#### add by mikejiang ####
#CFLAGS+=-DDEBUG -g

target = agentm turn_push_on
agentm_list := epoll_reactor listener protocol_handler sock_util status_updater \
	admin_protocol.pb main master_worker ../common/log
agentm_objs := $(patsubst %,%.o,$(agentm_list)) $(PROTOBUFLIB)
agentm_libs := $(MYSQLLIB)

turn_push_on_list := sock_util admin_protocol.pb turn_push_on ../common/sockaddr
turn_push_on_objs := $(patsubst %,%.o,$(turn_push_on_list)) $(PROTOBUFLIB)
turn_push_on_libs := $(MYSQLLIB)

target_install = turn_push_on agentm agentm_insert_sample.sql agentm_database.sql
install_dir = ../../agent_bin
include ../Make.rules
