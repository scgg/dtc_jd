#!/bin/bash

#
#  hotbackup fix工具，用作
#  1. 停止slave
#  2. 删除slave过期数据
#  3. 删除hotbackup临时数据文件
#  4. 启动slave
#

config_dir="../conf/"
ttc_config_file=$config_dir/cache.conf
bitmap_config_file=$config_dir/bsd.conf


#
# 停止slave
#
function stop_slave()
{
    echo -n "stop slave ..."

    if ../admin/stop.sh >/dev/null 2>&1; then
        echo " ok"
    else
        echo " err"
    fi
}

#
# 删除ttc共享内存
#
function delete_slave_ttc_shm()
{
    echo -n "delete slave ttc share mem ..."

    ./shmtool delete ttc  >/dev/null 2>&1

    echo " ok"
}

#
# 删除bitmap的数据索引文件
#
function delete_slave_bitmap_maps()
{
    echo -n "delete slave bitmap index&data ..."

    if rm -f ../data/* >/dev/null 2>&1; then
        echo " ok"
    else
        echo " err"
    fi
}

#
# 启动slave ttc
#
function start_slave()
{
    echo -n "start slave ..."

    if ../admin/start.sh >/dev/null 2>&1; then
        echo " ok"
    else
        echo " err"
    fi
}

#
# NON OP
#
function delay_secs()
{
    sleep 2
}

#
# 删除hb 日志
#
function remove_hbp_logs()
{
    echo -n "remove hotbackup logs ..."

    if rm -f ../hbp/*  >/dev/null 2>&1; then
        echo " ok"
    else
        echo " err"
    fi
}

#
# 判断是ttc，还是bitmap，然后删除其过期数据 
#
function delete_slave_dirty_data()
{
    if [ -f $ttc_config_file ]; then
        delete_slave_ttc_shm
    else
        delete_slave_bitmap_maps
    fi
}

if [ "$1" = "hbp" ] ; then
	remove_hbp_logs
	delay_secs
elif [ "$1" = "slave" ]; then
	stop_slave
	delay_secs
	delete_slave_dirty_data
	start_slave
	# 多次delay，防止内存不足，导致启动过缓
	delay_secs
	delay_secs
	delay_secs
else
	echo "usage: hb_fixed_env.sh hbp|slave"
fi

