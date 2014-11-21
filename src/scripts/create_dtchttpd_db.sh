#!/bin/sh

create_db_and_tables()
{
	db_user=root
	db_pass=root
	db_name=dtc_monitor_$1
	
	create_db_sql="CREATE DATABASE "$db_name";"
	echo $create_db_sql
	/usr/bin/mysql --connect-timeout 2 -s -u$db_user -p$db_pass -e "$create_db_sql"
	
	create_update_sql="CREATE TABLE t_monitor_instant (id bigint(20) NOT NULL AUTO_INCREMENT,bid int(11) NOT NULL,type int(11) NOT NULL,data1 bigint(20) NOT NULL DEFAULT '0',data2 bigint(20) NOT NULL DEFAULT '0',extra bigint(20) NOT NULL DEFAULT '0',datetime bigint(20) DEFAULT '0',PRIMARY KEY (id),UNIQUE KEY bid_type(bid,type)) ENGINE=InnoDB DEFAULT CHARSET=utf8;"
	/usr/bin/mysql --connect-timeout 2 -s -u$db_user -p$db_pass $db_name -e "$create_update_sql"
	if [[ $1 == 0 ]]; then
		create_alarm_sql="CREATE TABLE t_monitor_alarm (id bigint(20) NOT NULL AUTO_INCREMENT,bid int(11) NOT NULL,curve int(11) NOT NULL,data1 bigint(20) NOT NULL DEFAULT '0',data2 bigint(20) NOT NULL DEFAULT '0',extra bigint(20) NOT NULL DEFAULT '0',datetime int(11) NOT NULL DEFAULT '0',PRIMARY KEY (id),UNIQUE KEY datetime_bid_curve (datetime,bid,curve)) ENGINE=InnoDB DEFAULT CHARSET=utf8;"
		/usr/bin/mysql --connect-timeout 2 -s -u$db_user -p$db_pass $db_name -e "$create_alarm_sql"
		create_check_sql="CREATE TABLE t_monitor_check (id bigint(20) NOT NULL AUTO_INCREMENT,type int(11) NOT NULL,datetime bigint(20) DEFAULT '0',PRIMARY KEY (id),UNIQUE KEY index_type (type)) ENGINE=InnoDB DEFAULT CHARSET=utf8;"
		/usr/bin/mysql --connect-timeout 2 -s -u$db_user -p$db_pass $db_name -e "$create_check_sql"
	fi
	
	for((j=0;j<100;j++));
	do
		((table_index=$1+10*$j))
		create_insert_sql="CREATE TABLE t_monitor_"$table_index" (id bigint(20) NOT NULL AUTO_INCREMENT,bid int(11) NOT NULL,curve int(11) NOT NULL,etype int(11) NOT NULL,data1 bigint(20) NOT NULL DEFAULT '0',data2 bigint(20) NOT NULL DEFAULT '0',extra bigint(20) NOT NULL DEFAULT '0',datetime int(11) NOT NULL DEFAULT '0',PRIMARY KEY (id),UNIQUE KEY datetime_bid_curve (datetime,bid,curve)) ENGINE=InnoDB DEFAULT CHARSET=utf8;"
		/usr/bin/mysql --connect-timeout 2 -s -u$db_user -p$db_pass $db_name -e "$create_insert_sql"
	done
}

#######MAIN START#########

for((i=0;i<10;i++));
do
	create_db_and_tables $i
done


#######MAIN END###########
