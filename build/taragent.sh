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

cp ../src/3rdparty/dtc_api/lib64/libdtc-gcc-*.so ../agent_bin/
#ln -s  ../agent_bin/libdtc-gcc-*.so  ../agent_bin/libdtc.so
version=`./../agent_bin/dtcagent -v | grep version | awk '{print $3}'`
PLATFORM=`getconf LONG_BIT`
SYSTEM=`cat /etc/issue | head -n 1 | awk '{print $1}'`
dist_name=agent_${version}_${SYSTEM}${PLATFORM}
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
mv ../agent_bin ../conf ../lib ${dir}
mkdir ${dir}/stat
mkdir ${dir}/log

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

#if 32/64bit tgz is exist. create a MD5SUM
if [ -f "again_${version}_${SYSTEM}32.tgz" ] && [ -f "again_${version}_${SYSTEM}64.tgz" ];
then
	if [ -f "agent_${version}.md5" ];then
		echo -e ${YELLOW}OLD MD5${RESET}
		cat agent_${version}.md5
	else
		echo -e ${YELLOW}OLD MD5 NOT EXIST${RESET}
	fi
	echo -e ${YELLOW}NEW MD5${RESET}
	md5sum *.tgz > agent_${version}.md5
	cat agent_${version}.md5
fi