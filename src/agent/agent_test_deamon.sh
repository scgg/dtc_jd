#!/bin/sh

#Filename:    agent_test_deamon.sh
#Description: run in background operate agent randomly
#Author:      linjinming(shrewdlin)
#Contact:     prudence@163.com
#Version:     1.0
#Date:        2014-06-11 21:55
#Company:     JD China

help()
{
	cat << HELP
	查询Agent模拟调用脚本
	使用方法：
		[-h] Agent IP地址
		[-p] Agent 端口 
		[-a] Agent 访问码

HELP
	exit 0
}


num=$#

while [ -n "$1" ]; do
case "$1" in
	-H) help;shift 1;;
	-h) ip=$2;shift 2;;
	-p) port=$2;shift 2;;
	-a) accesstoken=$2;shift 2;;
	-*) echo "No such option:$1. -H for help";exit 1;;
	*) break;;
esac
done

if [ $num != 6 ] ; then 
	help;
fi 

function random()
{
    min=$1;
    max=$2-$1;
    num=$(date +%s);
    ((retnum=num%max+min));
    echo $retnum
}

rnd=$(random 2 10000);

((selector=$rnd%6));

if [ $selector -eq 0 ] ;then 
    ../bin/ttcd_test -t ../conf/table.conf -k $rnd -o get -i $ip -p $port -a $accesstoken& 
elif [ $selector -eq 1 ] ;then 
    ../bin/ttcd_test -t ../conf/table.conf -k $rnd -o insert -i $ip -p $port -a $accesstoken&
elif  [ $selector -eq 2 ] ;then 
    ../bin/ttcd_test -t ../conf/table.conf -k $rnd -o replace -i $ip -p $port -a $accesstoken&
elif  [ $selector -eq 3 ] ;then 
    ../bin/ttcd_test -t ../conf/table.conf -k $rnd -o update -i $ip -p $port -a $accesstoken&
elif  [ $selector -eq 4 ] ;then 
    ../bin/ttcd_test -t ../conf/table.conf -k $rnd -o delete -i $ip -p $port -a $accesstoken&
elif  [ $selector -eq 5 ] ;then 
    ../bin/ttcd_test -t ../conf/table.conf -k $rnd -o purge -i $ip -p $port -a $accesstoken&
fi
