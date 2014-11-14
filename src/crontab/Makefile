include ../Make.conf
################compile###############
SVN_REVISION = $(shell test -d .git && (git svn info | grep "Last Changed Rev" | cut -d " " -f 4))

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = $(shell test -d .svn && (svn info | grep "Last Changed Rev" | cut -d " " -f 4))
endif

ifeq "$(SVN_REVISION)a" "a"
	SVN_REVISION = "(unknown)"
endif
ifneq ($(findstring x86_64,$(PLATFORM)),)
BITS=64
else
BITS=32
endif

VPATH := ../3rdparty/tinyxml ../3rdparty/http_service ../stat

CFLAGS+=-DSVN_REVISION=$(SVN_REVISION) -I../common -I../watchdog -I../3rdparty/tinyxml
CXXFLAGS+=-Wno-sign-compare

target = dtccrontab test

dtccrontab_list := crontab process_helper tinystr tinyxmlparser tinyxmlerror tinyxml curl_http StatClient
dtccrontab_objs := $(patsubst %,%.o,$(dtccrontab_list)) ../watchdog/libwatchdog.a ../stat/libstat.a
dtccrontab_libs := -L../common -L../3rdparty/attr_api -lcommon -ldl -lattr_api_$(BITS) $(JSON_LIB) $(CURL_LIB)
dtccrontab: LDFLAGS+=-pthread

test_list := test
test_objs := $(patsubst %,%.o,$(test_list))
test_libs := -L../common -L../3rdparty/attr_api -lcommon -ldl -lattr_api_$(BITS)
test: LDFLAGS+=-pthread

################install##############
target_install = dtccrontab
install_dir = ../../agent_bin
include ../Make.rules
