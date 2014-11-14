include ../Make.conf

target = libwatchdog.a faultlogger

CFLAGS+=-I../common -I../stat
VPATH := ../stat
filelist := watchdog watchdog_daemon watchdog_fork watchdog_helper watchdog_listener watchdog_main watchdog_run watchdog_stattool watchdog_unit watchdog_logger fault gdb gdbsrv DtcStatAlarmReporter
libwatchdog_objs := $(filelist:%=%.o)

faultlogger: LDFLAGS += -shared -e _so_start

faultlogger_objs = fault.pic.o

fault.pic.o fault.o: CFLAGS += -O0

target_install = faultlogger
install_dir = ../../bin

include ../Make.rules

