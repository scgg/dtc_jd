#!/bin/sh
#create by adamxu
#date:2014-08-06
#build dtcsdk.tgz

MAGENTA=\\e[1m\\e[35m
RESET=\\e[m
GREEN=\\e[1m\\e[32m

#make c_api
libdtc_dir=./lib
libjava_dir=./lib
libphp_dir=./lib

c_dir=./dtc_api_c++
java_dir=./dtc_api_java
php_dir=./dtc_api_php

cd ../src/api/c_api
make clean;make

#ln -s libdtc-gcc*.so libdtc.so
#mkdir c++ lib for libdtc.so
if [ -d "$libdtc_dir" ];then
echo -e "${MAGENTA}DELETE ${libdtc_dir}${RESET}"
rm -rf ${libdtc_dir}
fi
echo -e "${MAGENTA}MKDIR ${libdtc_dir}${RESET}"
mkdir -p ${libdtc_dir}

cp ./libdtc-gcc*.so ${libdtc_dir}

if [ -d "doc" ];then
echo -e "${MAGENTA}DELETE doc ${RESET}"
rm -rf doc
fi
echo -e "${MAGENTA}MKDIR doc ${RESET}"
mkdir -p doc

cp ../../../doc/dtc_api_c++*.doc ./doc

#mkdir c++ dir for dtc_api_c++ includs doc,example and dtc api
if [ -d "$c_dir" ];then
echo -e "${MAGENTA}DELETE ${c_dir}${RESET}"
rm -rf ${c_dir}
fi
echo -e "${MAGENTA}MKDIR ${c_dir}${RESET}"
mkdir -p ${c_dir}

cp -r ./example ./doc ${libdtc_dir} ${c_dir}
cp ../script/auto_dtcc++.sh ${c_dir}
cp ./readme.txt ${c_dir}
#tar dtc_api_c++.tgz
if [ -f "$c_dir.tgz" ];then
echo -e "${MAGENTA}DELETE ${c_dir}.tgz${RESET}"
rm -rf ${c_dir}.tgz
fi
echo -e "CREATE ${GREEN}${c_dir}.tgz${RESET} FROM  ${GREEN}${c_dir}${RESET}"
tar zcf ${c_dir}.tgz ${c_dir}

cp ${c_dir}.tgz ../../../dist

#make and tar java_api
cd ../java_api/java_api_jni
make clean;make

#build java jar package
cd ../api_java
./compile.sh

cd ..
#mkdir java lib for lib_dtc_java_api.so
if [ -d "$libjava_dir" ];then
echo -e "${MAGENTA}DELETE ${libjava_dir}${RESET}"
rm -rf ${libjava_dir}
fi
echo -e "${MAGENTA}MKDIR ${libjava_dir}${RESET}"
mkdir -p ${libjava_dir}

cp ./java_api_jni/lib_dtc_java*.so ./api_java/bin/dtc_api*.jar ../c_api/libdtc-gcc*.so  ${libjava_dir}
#mkdir java dir for dtc_api_java includs doc,example and dtc java_api
if [ -d "$java_dir" ];then
echo -e "${MAGENTA}DELETE ${java_dir}${RESET}"
rm -rf ${java_dir}
fi
echo -e "${MAGENTA}MKDIR ${java_dir}${RESET}"
mkdir -p ${java_dir}

if [ -d "doc" ];then
echo -e "${MAGENTA}DELETE doc ${RESET}"
rm -rf doc
fi
echo -e "${MAGENTA}MKDIR doc ${RESET}"
mkdir -p doc

cp ../../../doc/dtc_api_java*.doc ./doc

cp -r ./doc ${libjava_dir} ./api_java/src/com/jd/dtc/example ${java_dir}
cp ../script/auto_dtcjava.sh ${java_dir}
cp ./readme.txt ${java_dir}
#tar dtc_api_java.tgz
if [ -f "$java_dir.tgz" ];then
echo -e "${MAGENTA}DELETE ${java_dir}.tgz${RESET}"
rm -rf ${java_dir}.tgz
fi
echo -e "CREATE ${GREEN}${java_dir}.tgz${RESET} FROM  ${GREEN}${java_dir}${RESET}"
tar zcf ${java_dir}.tgz ${java_dir}

cp ${java_dir}.tgz ../../../dist

#make and tar php_api
cd ../php_api_wrap/php_qqvip
make clean;make

cd ..
#mkdir php lib for tphp_dtc.so
if [ -d "$libphp_dir" ];then
echo -e "${MAGENTA}DELETE ${libphp_dir}${RESET}"
rm -rf ${libphp_dir}
fi
echo -e "${MAGENTA}MKDIR ${libphp_dir}${RESET}"
mkdir -p ${libphp_dir}

cp ./php_qqvip/tphp_dtc.so* ../c_api/libdtc-gcc*.so  ${libphp_dir}
#mkdir php dir for dtc_api_php includs doc,example and dtc php_api
if [ -d "$php_dir" ];then
echo -e "${MAGENTA}DELETE ${php_dir}${RESET}"
rm -rf ${php_dir}
fi
echo -e "${MAGENTA}MKDIR ${php_dir}${RESET}"
mkdir -p ${php_dir}

if [ -d "doc" ];then
echo -e "${MAGENTA}DELETE doc ${RESET}"
rm -rf doc
fi
echo -e "${MAGENTA}MKDIR doc ${RESET}"
mkdir -p doc

cp ../../../doc/dtc_api_php*.doc ./doc

cp -r ./example ./doc ${libphp_dir} ${php_dir}
cp ../script/auto_dtcphp.sh ${php_dir}
cp ./readme.txt ${php_dir}
#tar dtc_api_php.tgz
if [ -f "$php_dir.tgz" ];then
echo -e "${MAGENTA}DELETE ${php_dir}.tgz${RESET}"
rm -rf ${php_dir}.tgz
fi
echo -e "CREATE ${GREEN}${php_dir}.tgz${RESET} FROM  ${GREEN}${php_dir}${RESET}"
tar zcf ${php_dir}.tgz ${php_dir}

cp ${php_dir}.tgz ../../../dist
