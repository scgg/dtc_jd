include Make.conf

SUBDIRS := $(shell test -d ../conf || echo conf) \
	common api stat watchdog cache helper scripts \
	plugin hot_backup cold_backup time_hot_back agent ttc_tool agentm \
	crontab prober

make_all_api:: make_all_common

make_all_stat:: make_all_common

make_all_helper:: make_all_common

make_all_cache:: make_all_common make_all_api make_all_stat make_all_watchdog

make_all_hot_backup:: make_all_common make_all_api

make_all_time_hot_back:: make_all_common make_all_api

make_all_cold_backup:: make_all_common make_all_api

make_all_agent:: make_all_common make_all_stat

make_all_agentm:: make_all_common

make_all_ttc_tool:: make_all_common make_all_api

make_all_crontab:: make_all_common make_all_watchdog

all::
	@-mkdir -p ../data ../hbp ../stat ../log

distclean:: clean
	@-rm -rf ../data ../hbp ../stat ../log ../bin ../lib ../include ../conf

include Make.rules

