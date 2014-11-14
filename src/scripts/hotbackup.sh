#!/bin/sh

ulimit -c unlimited

MS=`cat /usr/local/dtc/ip | sed -n -e '2p'`
if [ "$MS" = "ms" ];then
	CONFIG_FILE="../conf/hbp_m.conf"
elif [ "$MS" = "s" ];then
	CONFIG_FILE="../conf/hbp_s.conf"
fi

HBP_ARGV="-n -b -c $CONFIG_FILE"
HBP_FIXED_ARGV="-f -b -c $CONFIG_FILE"
HBP_PURGE_ARGV="-p -c $CONFIG_FILE"

session_token=`pwd | awk -F'/' '{print $5}'`
HBP_BIN="hotbackup_$session_token"

if [ $# -eq 0 ];then
    ./$HBP_BIN $HBP_ARGV
    exit
fi

if [ "$1" = "stop" ] ; then
    killall -2 $HBP_BIN
elif [ "$1" = "restart" ]; then
    killall -2 $HBP_BIN
	rm -f "$HBP_BIN"
	ln -s hotbackup "$HBP_BIN" 
    ./$HBP_BIN $HBP_ARGV
elif [ "$1" = "start" ]; then
	rm -f "$HBP_BIN"
	ln -s hotbackup "$HBP_BIN" 
    ./$HBP_BIN $HBP_ARGV
	res=$?
	if [ $res -ne 0 ];then
		exit 1
	fi
elif [ "$1" = "fixed" ]; then
    ./$HBP_BIN $HBP_FIXED_ARGV
	res=$?
	if [ $res -ne 0 ];then
		exit 1
	fi
elif [ "$1" = "purge" ]; then
    ./$HBP_BIN $HBP_PURGE_ARGV
	res=$?
	if [ $res -ne 0 ];then
		exit 1
	fi
else
	echo "usage: hotbackup.sh start|stop|restart|fixed|purge"
fi

