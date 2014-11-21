#!/bin/sh
#create by foryzhou
#2010年7月28日 11:25:18

RED=\\e[1m\\e[31m
DARKRED=\\e[31m
GREEN=\\e[1m\\e[32m
DARKGREEN=\\e[32m
BLUE=\\e[1m\\e[34m
DARKBLUE=\\e[34m
YELLOW=\\e[1m\\e[33m
DARKYELLOW=\\e[33m
MAGENTA=\\e[1m\\e[35m
DARKMAGENTA=\\e[35m
CYAN=\\e[1m\\e[36m
DARKCYAN=\\e[36m
RESET=\\e[m


#build ttc
make

architecture=`gcc -dumpmachine | grep x86_64`
if [ "$architecture" = "" ]; then
	cp ../src/3rdparty/dtc_api/lib32/libdtc-gcc-*.so ../monitor_tool_bin/
else
	cp ../src/3rdparty/dtc_api/lib64/libdtc-gcc-*.so ../monitor_tool_bin/
fi

PLATFORM=`getconf LONG_BIT`
SYSTEM=`cat /etc/issue | head -n 1 | awk '{print $1}'`
dist_name=dtctool_1.0.0_${SYSTEM}${PLATFORM}
dir=../dist/$dist_name

#if dist exist delete it
if [ -d "$dir" ];then
echo -e "${MAGENTA}DELETE ${dir}${RESET}"
rm -rf ${dir}
fi

#create dist directory and copy bin include lib conf to it
echo -e "${MAGENTA}MKDIR ${dir}${RESET}"
mkdir -p ${dir}
echo -e "${MAGENTA}MOVE agent_bin bin include lib conf TO DIST${RESET}"
mv ../monitor_tool_bin ../conf ${dir}
mkdir ${dir}/stat
mkdir ${dir}/log

rm ${dir}/conf/cache.conf
rm ${dir}/conf/crontab.xml
rm ${dir}/conf/dtcalarm.conf
rm ${dir}/conf/dtchttpd_example.conf
rm ${dir}/conf/hbp.conf
rm ${dir}/conf/prober.xml
rm ${dir}/conf/table.conf
rm ${dir}/conf/plugin.conf
rm ${dir}/conf/contacts.xml
rm ${dir}/conf/agent.xml
rm ${dir}/conf/agent.conf

#if tgz is exsit delete it.delete directory first.
if [ -f "$dir.tgz" ];then
echo -e "${MAGENTA}DELETE ${dir}.tgz${RESET}"
rm -rf ${dir}.tgz
fi

#enter dist and tar
echo -e ${MAGENTA}ENTER DIST${RESET}
cd ../dist
echo -e "CREATE ${GREEN}${dist_name}.tgz${RESET} FROM  ${GREEN}${dist_name}${RESET}"
tar zcf ${dist_name}.tgz ${dist_name}
rm -rf ${dist_name}