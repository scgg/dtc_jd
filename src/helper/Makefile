include ../Make.conf

### 模块间调用 ####
ifneq ($(LOGAPIDIR),)
LOGAPIINC = -I$(LOGAPIDIR)/include/api -DHAS_LOGAPI
LOGAPILIB = -L$(LOGAPIDIR)/lib -Wl,-rpath=$(LOGAPIDIR)/lib -lmsglog
endif

CFLAGS+=-I../common -I../watchdog

LIBPATH := -L../common -L../watchdog -L../3rdparty/attr_api
ifneq ($(findstring x86_64,$(PLATFORM)),)
BITS=64
else
BITS=32
endif

###############mysql-helper############
ifneq ($(MYSQLDIR),)
target += mysql-helper
mysql-helper_objs := DBConn.o DBMain.o DBProcess.o
mysql-helper_libs := -lcommon -lwatchdog -lattr_api_$(BITS) $(MYSQLLIB) $(LOGAPILIB) -ldl

DB%.o: CFLAGS += $(MYSQLINC)  $(LOGAPIINC)
mysql-helper: LDFLAGS += -pthread
endif

###############comm-helper############
ifneq ($(CUSTOMHLP),)
target += custom-helper
custom-helper_objs := CommMain.o CommProcess.o
custom-helper_libs := -lcommon -lwatchdog -lattr_api_$(BITS) $(LOGAPILIB) -ldl -rdynamic -lrt
Comm%.o: CFLAGS += -I. -fno-ident $(LOGAPIINC)
custom-helper: LDFLAGS += -pthread
endif

#############install#############

ifneq ($(MYSQLDIR),)
target_install += mysql-helper
endif

ifneq ($(CUSTOMHLP),)
target_install += custom-helper
endif

install_dir = ../../bin

include ../Make.rules
