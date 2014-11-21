#!/bin/sh


MYSQLDIR=$1

detect(){
	if [ -e "$1" ]; then
		echo $@
		exit 0
	fi
}

detect $MYSQLDIR/lib/mysql/libmysqlclient.a -lz
detect $MYSQLDIR/lib/mysql/libmysqlclient_r.a -lz -lpthread
detect $MYSQLDIR/lib/libmysqlclient.a -lz
detect $MYSQLDIR/lib/libmysqlclient_r.a -lz -lpthread
detect $MYSQLDIR/lib/mysql/libmysqlclient.so -Wl,-rpath=$MYSQLDIR/lib/mysql
detect $MYSQLDIR/lib/mysql/libmysqlclient_r.so -Wl,-rpath=$MYSQLDIR/lib/mysql
detect $MYSQLDIR/lib/libmysqlclient.so -Wl,-rpath=$MYSQLDIR/lib
detect $MYSQLDIR/lib/libmysqlclient_r.so -Wl,-rpath=$MYSQLDIR/lib
