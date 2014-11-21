#!/bin/sh
#create by linjinming

#build ttc
make

PRG_VERSION=`./../agent_bin/dtccrontab -v | head -n 1 | awk '{print $3}'`
PLATFORM=`getconf LONG_BIT`
SYSTEM=`cat /etc/issue | head -n 1 | awk '{print $1}'`
dist_name=dtccrontab_${PRG_VERSION}_${SYSTEM}${PLATFORM}
dir=../dist/$dist_name

#if dist exist delete it
if [ -d "$dir" ];then
echo -e "DELETE ${dir}"
rm -rf ${dir}
fi

#create dist directory and copy bin conf to it
echo -e "MKDIR ${dir}"
mkdir -p ${dir}
mkdir -p ${dir}/conf
mkdir -p ${dir}/bin
echo -e "MOVE dtchttpd TO DIST"
cp ../agent_bin/dtccrontab ${dir}/bin
cp ../agent_bin/dtccrontab_start.sh ${dir}/bin
cp ../conf/crontab.xml ${dir}/conf

#if tgz is exsit delete it.delete directory first.
if [ -f "$dir.tgz" ];then
echo -e "DELETE ${dir}.tgz"
rm -rf ${dir}.tgz
fi

#enter dist and tar
echo -e "ENTER DIST"
cd ../dist
echo -e "CREATE ${dist_name}.tgz FROM ${dist_name}"
tar zcf ${dist_name}.tgz ${dist_name}
rm -rf ${dist_name}

#if tgz is exist. create a MD5SUM
if [ -f "$dir.tgz" ];
then
	if [ -f "dtccrontab_${version}.md5" ];then
		echo -e "OLD MD5"
		cat dtccrontab_${version}.md5
	else
		echo -e "OLD MD5 NOT EXIST"
	fi
	echo -e "NEW MD5"
	md5sum ${dir}.tgz > dtccrontab_${version}.md5
	cat dtccrontab_${version}.md5
fi
