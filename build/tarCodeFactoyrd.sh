#!/bin/sh
#adamxu
#2014年10月22日

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
cd ../src
make clean;
cd common/
make
cd ../dtcCodeFactoryd/
make
make install

version=`./../../codeFactoryd_bin/dtcCodeFactoryd  -v -p -c -d  | grep version | awk '{print $3}'`
cd ../../build/
PLATFORM=`getconf LONG_BIT`
SYSTEM=`cat /etc/issue | head -n 1 | awk '{print $1}'`
dist_name=dtcCodeFactoryd_${version}_${SYSTEM}${PLATFORM}
dir=../dist/$dist_name

#if dist exist delete it
if [ -d "$dir" ];then
echo -e "${MAGENTA}DELETE ${dir}${RESET}"
rm -rf ${dir}
fi

#create dist directory and copy bin include lib conf to it
echo -e "${MAGENTA}MKDIR ${dir}${RESET}"
mkdir -p ${dir}
echo -e "${MAGENTA}MOVE codeFactoryd_bin and cp PhpCodeFactory TO DIST${RESET}"
cp -r ../codeFactoryd_bin ../src/conf  ${dir}
cp -r ../src/api/php_api_wrap/PhpCodeFactory/  ${dir}

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
if [ -f "dtcCodeFactoryd_${version}_${SYSTEM}32.tgz" ] && [ -f "dtcCodeFactoryd_${version}_${SYSTEM}64.tgz" ];
then
	if [ -f "dtcCodeFactoryd_${version}.md5" ];then
		echo -e ${YELLOW}OLD MD5${RESET}
		cat dtcCodeFactoryd_${version}.md5
	else
		echo -e ${YELLOW}OLD MD5 NOT EXIST${RESET}
	fi
	echo -e ${YELLOW}NEW MD5${RESET}
	md5sum *.tgz > dtcCodeFactoryd_${version}.md5
	cat dtcCodeFactoryd_${version}.md5
fi