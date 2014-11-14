#!/bin/sh

#get t_monitor_check datetime
get_db_time()
{
	db_host=172.22.224.130
	db_user=jd_ump
	db_pass=ump@789
	db_name=dtc_monitor
	sql="select datetime from t_monitor_check where type=1000"
	/usr/bin/mysql --connect-timeout 2 -s -h$db_host -u$db_user -p$db_pass $db_name -e "$sql"
}

#start dtchttpd
start_dtchttpd()
{
	cd /export/servers/dtchttpd/
	./dtchttpd -p 9090 -c dtchttpd.conf -d 0
	cd -
}

##########MAIN START##########

#alarm url
alarm_url=http://monitor.m.jd.com/tools/alarm/api

#check dtchttpd is alive or not
killall -0 dtchttpd
if [[ $? == 1 ]]; then
	msg_alive="dtchttpd is die"
	start_dtchttpd
	sleep 1
	killall -0 dtchttpd
	if [[ $? == 1 ]]; then
		sms_msg="dtchttpd is die and start fail"
		exit -1
	else
		sms_msg="dtchttpd is die and start success"
	fi
	post_data='req={"alarm_type":"sms","app_name":"dtchttpd_alarm","data":{"alarm_content":"'${sms_msg}'","alarm_list":"13817392165;18621589812;18516280383;","alarm_title":"dtchttpd restarted"}}'
	#echo $post_data
	/usr/bin/curl -m 2 -d "${post_data}" $alarm_url
fi

#get unix time
now_time=`date "+%s"`

#send check msg
dtchttpd_req='{"curve":1000,"datetime":'$now_time'}'
dtchttpd_url=http://172.22.224.34:9090/
/usr/bin/curl -m 2 -d "${dtchttpd_req}" ${dtchttpd_url}

#check now and db time is same or not
#try 3 times sleep 1,2,3 seconds
for i in 1 2 3 4;
do
	if [[ $i == 4 ]]; then
		break;
	fi
	sleep $i
	echo "`date "+%s"`"
	db_time=$(get_db_time)
	echo "`date "+%s"`"
	if [[ $now_time == $db_time ]]; then
		break;
	fi
done

#check now and db time not same
#dtchttpd is not healthy
if [[ $i == 4 ]]; then
	sms_msg="dtchttpd is not healthy check time is "$now_time" db time is "$db_time
	post_data='req={"alarm_type":"sms","app_name":"dtchttpd_alarm","data":{"alarm_content":"'${sms_msg}'","alarm_list":"13817392165;18621589812;18516280383;","alarm_title":"dtchttpd is not healthy"}}'
	#echo $post_data
	/usr/bin/curl -m 2 -d "${post_data}" $alarm_url
fi

##########MAIN END##########
