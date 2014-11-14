#!/bin/sh

ulimit -c unlimited

ip=`head -1 ../../ip`
param1=$2
param2=$3

success_report_url=`echo ${param1/\%ip/$ip}`
fail_report_url=`echo ${param2/\%ip/$ip}`
session_token=`pwd | awk -F'/' '{print $5}'`
DTC_BIN="dtcd_$session_token"

rm -f "$DTC_BIN"
ln -s dtcd "$DTC_BIN" 

DisableDataSource=`cat ../conf/cache.conf | grep DisableDataSource | awk -F"=" '{print $2}'`
db_machine_num=`cat ../conf/table.conf | grep MachineNum | awk -F"=" '{print $2}'`

#上报admin平台
report()
{
	ttcd_num=`ps -f -C $DTC_BIN | fgrep -w $DTC_BIN | wc -l`
	watchdog_pid=`ps axj | grep $DTC_BIN | awk '$1==1 {print $2}'`
	
	#echo "DisableDataSource=$DisableDataSource"
	#echo "ttcd_num=$ttcd_num"
	#echo "watchdog_pid=$watchdog_pid"
	#echo "helper_num=$helper_num"
	
	#有源模式
	if [[ $DisableDataSource == 0 ]] ; then
		if [[ $ttcd_num == 2 ]] ; then
			for((i=0;i<$db_machine_num;i++))
			do
				helper_num=`ps ajx | egrep "helper$i[ms]" | awk '$1=="'$watchdog_pid'" {print $0}' | wc -l`
				if [[ $helper_num > 1 ]] ; then
					#echo "success_report_url=$success_report_url"
					#curl $success_report_url
					echo ""
				else
					#echo "fail_report_url=$fail_report_url"
					curl $fail_report_url
					exit 1;
				fi
			done
			
			#echo "success_report_url=$success_report_url"
			curl $success_report_url
		else
			#echo "fail_report_url=$fail_report_url"
			curl $fail_report_url
		fi
	#无源模式
	elif  [[ $DisableDataSource == 1 ]] ; then
		./hotbackup.sh start > /dev/null
		if [[ $ttcd_num == 2 ]] ; then
			#echo "success_report_url=$success_report_url"
			curl $success_report_url
		else
			#echo "fail_report_url=$fail_report_url"
			curl $fail_report_url
		fi
	echo "exit"
	fi
}

if [ "$1" = "stop" ] ; then
    killall $DTC_BIN
elif [ "$1" = "restart" ]; then
    killall $DTC_BIN
	sleep 3
    ./$DTC_BIN
elif [ "$1" = "start" ]; then
    ./$DTC_BIN
	sleep 1
	report
else
    echo "usage: $0 stop | start \"success_report_url\" \"fail_report_url\" | restart \"success_report_url\" \"fail_report_url\""
fi
