#!/bin/bash

#
# dtc 热备切换脚本
#
# 1. 停止hotbackup 	 (./hotbackup.sh stop)
# 2. purge 未处理完的日志(./hotbackup.sh purge)
# 3. 重启dtc
#

dtcd_dir="./"
cur_dir="./"
hbp_bin=$dtcd_dir/hotbackup.sh
dtc_bin=$dtcd_dir/dtcd.sh

#check binary executable
function check_exec()
{
	if [ ! -x $hbp_bin ]; then
		exit 100
	fi

	if [ ! -x $dtc_bin ]; then
		exit 100
	fi
}

#delay secs, non op
function delay_secs()
{
	sleep 10
}

function stop_hbp()
{
	cd $dtcd_dir
	$hbp_bin stop
	res=$?
	cd $cur_dir
	
	if [ $res -ne 0 ];then
		exit 1
	fi
}

function purge_hbp()
{
	cd $dtcd_dir
	$hbp_bin purge
	res=$?
	cd $cur_dir
	
	if [ $res -ne 0 ];then
		exit 2
	fi
}

function restart_dtcd()
{
	cd $dtcd_dir
	$dtc_bin restart
	res=$?
	cd $cur_dir
	
	if [ $res -ne 0 ];then
		exit 3
	fi
}

check_exec
stop_hbp
delay_secs
purge_hbp
restart_dtcd
sleep 5