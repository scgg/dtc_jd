#!/bin/bash
echo "preun script start!"
dtcd=`ls |grep dtcd_*`
killall $dtcd
shmkey=`cat ../conf/cache.conf |grep CacheShmKey | awk -F '=' '{print $2}'`
ipcrm -M $shmkey
echo "preun script done!"
