#!/bin/sh

moudle_dir=/usr/local/dtc/

cd $moudle_dir

dtcd_count=0
dtcd_alive=0
start_succ=0
start_fail=0
succ_list=""
fail_list=""

# check dtcd is alive or not
# arg1 disable_data_source
# arg2 dtcd num
# arg3 db num
# arg4 watch dog pid
dtcd_alive()
{
	if [[ $1 == 0 ]]; then
		if [[ $2 == 2 ]]; then
			for((i=0;i<$3;i++))
			do
				helper_num=`ps ajx | egrep "helper$i[ms]" | awk '$1=="'$4'" {print $0}' | wc -l`
				if [[ $helper_num < 1 ]]; then
					break
				fi
			done
			if [[ $i == $3 ]]; then
				return 0 
			else
				return 1
			fi
		else
			return 1
		fi
	else
		if [[ $2 == 2 ]]; then
			return 0
		else
			return 1
		fi
	fi
}


for file in `ls`
do
	if [ -d $file ]; then
		let dtcd_count++
	else
		continue
	fi
	
	DisableDataSource=`cat $moudle_dir/$file/conf/cache.conf | grep DisableDataSource | awk -F"=" '{print $2}'`
	db_machine_num=`cat $moudle_dir/$file/conf/table.conf | grep MachineNum | awk -F"=" '{print $2}'`
	
	ttcd_num=`ps -f -C dtcd_$file | fgrep -w dtcd_$file | wc -l`
	watchdog_pid=`ps axj | grep dtcd_$file | awk '$1==1 {print $2}'`
	
	if dtcd_alive $DisableDataSource $ttcd_num $db_machine_num $watchdog_pid ; then
		let dtcd_alive++
		continue
	fi
	
	echo "start dtcd"	

	if [ -f $file/bin/dtcd_$file ]; then
		echo $file
		cd $moudle_dir/$file/bin/
		killall dtcd_$file
		./dtcd_$file
		cd -
	fi

	sleep 1
	
	ttcd_num=`ps -f -C dtcd_$file | fgrep -w dtcd_$file | wc -l`
	watchdog_pid=`ps axj | grep dtcd_$file | awk '$1==1 {print $2}'`

	if dtcd_alive $DisableDataSource $ttcd_num $db_machine_num $watchdog_pid ; then
		let start_succ++
		succ_list+=${file:0:8}" "
	else
		let start_fail++
		fail_list+=${file:0:8}" "
	fi
done

msg=$moudle_dir" has "$dtcd_count" dtcd alive "$dtcd_alive" success start "$start_succ" fail start "$start_fail
if [[ $start_succ > 0 ]]; then
	msg+="succ list is "$succ_list
fi
if [[ $start_fail > 0 ]]; then
	msg+="fail list is "$fail_list
fi
#echo ${msg}

post_data='req={"alarm_type":"sms","app_name":"start_dtc_sh_alarm","data":{"alarm_content":"'${msg}'","alarm_list":"13817392165;15000688034;13816232661;18621589812;","alarm_title":"retart dtc sh"}}'
#echo ${post_data}

alarm_url=http://monitor.m.jd.com/tools/alarm/api
#alarm_url=http://192.168.215.54:8081/tools/alarm/api

if [[ $start_succ > 0 || $start_fail > 0 ]]; then
	/usr/bin/curl -d "${post_data}" $alarm_url
fi
