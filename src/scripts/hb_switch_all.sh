#!/bin/bash

#
# dtc 热备整机切换脚本
#

dtc_dir="/usr/local/dtc"
cur_dir=`pwd`

declare -a fail_list
declare -a success_list

index_fail=0
#check binary executable
function check_exec()
{
	if [ ! -x $1 ]; then
		return 100
	else
		return 0
	fi
}

cd $dtc_dir

for file in `ls`
do
	if [ ! -d "$file" ];then
		continue
	else
		script_bin=./$file/bin
		check_exec $script_bin/hb_switch.sh
		res=$?

		if [ $res -ne 0 ];then
			fail_list=$fail_list,$file
			continue
		fi
	fi
	
	cd $script_bin
	./hb_switch.sh
	res=$?
	cd $dtc_dir
	
	if [ $res -ne 0 ];then
		fail_list=$fail_list,$file
	else
		success_list=$success_list,$file
	fi	
	
done

echo "$success_list;$fail_list"
exit 0
