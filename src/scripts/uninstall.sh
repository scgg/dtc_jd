#!/bin/bash
echo "preun script start!"
dtcd=`ls |grep dtcd_*`
hb=`ls |grep hotbackup_*`
killall $dtcd
killall $hb 2>>1 > /dev/null
shmkey=`cat ../conf/cache.conf |grep CacheShmKey | awk -F '=' '{print $2}'`
ipcrm -M $shmkey
echo "preun script done!"
