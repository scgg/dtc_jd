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

version=`grep "#define TTC_VERSION" ../src/common/version.h|grep -v "TTC_VERSION_DETAIL"|awk '{print $3}'|awk -F\" {'print $2'}`
sub_version=`grep "#define TTC_GIT_VERSION" ../src/common/version.h | awk '{print $3}' | awk -F\" '{print $2}'`
api_version=`grep "ttc_api_ver" ../src/api/c_api/ttcsrv.cc|awk -F\" '{print $2}'|awk -F: '{print $2}'`
code_version="dtc_${version}_release_${sub_version}"
PLATFORM=`getconf LONG_BIT`
path=`pwd|awk -F/ {'print $(NF-1)'}`
dist_name=dtc_${version}_r${sub_version}_suse$PLATFORM
dir=../dist/$dist_name
echo -e "TTC_VERSION(this directory) :\t${GREEN}${path}${RESET}"
echo -e "\tTTC_VERSION(in code):\t${GREEN}${code_version}${RESET}"
echo -e "\tTTC_API_VER(in code):\t${GREEN}$api_version${RESET}"
echo -e "\t\t   paltform:\t${GREEN}$PLATFORM${RESET} bit"

#add by adamxu
#after make for java_api and php_api make and install
cd ../src/api/java_api/api_java 
./compile.sh
cd ./bin 
cp dtc_api.jar ../../../../../bin/

#for php
cd ../../../php_api_wrap/php_qqvip/ 
make clean;make
cp tphp_dtc.so* ../../../../bin

#go back to the build dir
cd ../../../../build/
#if dist exist delete it
if [ -d "$dir" ];then
echo -e "${MAGENTA}DELETE ${dir}${RESET}"
rm -rf ${dir}
fi

#create dist directory and copy bin include lib conf to it
echo -e "${MAGENTA}MKDIR ${dir}${RESET}"
mkdir -p ${dir}
echo -e "${MAGENTA}MOVE bin include lib conf TO DIST${RESET}"
mv ../bin ../include ../lib ../conf ${dir}

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
if [ -f "dtc_${version}_r${sub_version}_suse32.tgz" ] && [ -f "dtc_${version}_r${sub_version}_suse64.tgz" ];
then
	if [ -f "dtc_${version}_r${sub_version}.md5" ];then
		echo -e ${YELLOW}OLD MD5${RESET}
		cat dtc_${version}_r${sub_version}.md5
	else
		echo -e ${YELLOW}OLD MD5 NOT EXIST${RESET}
	fi
	echo -e ${YELLOW}NEW MD5${RESET}
	md5sum *.tgz > dtc_${version}_r${sub_version}.md5
	cat dtc_${version}_r${sub_version}.md5
fi
#echo version info again!!!! please check you have change the version info
if [ "${path}" != "${code_version}" ];then
	echo -e "${RED}PLEASE CHECK YOU HAVE RIGHT VIRSION INFO${RESET}"
	echo -e "DIRCTORY IS :${RED}${path}${RESET} BUT VERSION IN CODE IS :${RED}${code_version}${RESET}"
	if [ "${version}" != "${api_version}" ];then
		echo -e "API_VERSION IS :${RED}${api_version}${RESET} BUT TTC_VERSION IS :${RED}${version}${RESET}"
	fi
elif [ "${version}" != "${api_version}" ];then
	echo -e "${RED}PLEASE CHECK YOU HAVE RIGHT VIRSION INFO${RESET}"
	echo -e "API_VERSION IS :${RED}${api_version}${RESET} BUT TTC_VERSION IS :${RED}${version}${RESET}"
else
	echo -e "${GREEN}VERSION INFO IS OK${RESET}"
	echo -e "TTC_VERSION(this directory) :\t${GREEN}${path}${RESET}"
	echo -e "\tTTC_VERSION(in code):\t${GREEN}${code_version}${RESET}"
	echo -e "\tTTC_API_VER(in code):\t${GREEN}$api_version${RESET}"
	echo -e "\t\t   paltform:\t${GREEN}$PLATFORM${RESET} bit"
fi
