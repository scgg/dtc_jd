/*
 * =====================================================================================
 *
 *       Filename:  hbp_status.cc
 *
 *    Description:  dump hbp runing status in human-reading format.
 *
 *        Version:  1.0
 *        Created:  12/07/2008 10:10:48 AM
 *       Revision:  $Id: hbp_status.cc 3050 2009-09-03 03:04:56Z samuelliao $
 *       Compiler:  gcc
 *
 *         Author:  jackda (ada), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <ttcapi.h>
#include "async_file.h"

static int read_server_address(const char *path, char *master, char *slave)
{
	if(!path || access(path, R_OK))
		return -1;

	FILE *fp = fopen(path, "r");
	if(!fp) return -1;

	char *p;
	p = fgets(master, 255, fp);
	p = fgets(slave , 255, fp);

	fclose(fp);
	return 0;
}

static int query_binlog_status_from_master(const char *path) 
{
	uint32_t id,off;
	id = off= 0;

	char master_address[256] = {0};
	char slave_address[256]  = {0};

	if(read_server_address(path, master_address, slave_address))
		return -1;

	if(master_address[0] == 0 || slave_address[0] == 0)
		return -1;

	master_address[strlen(master_address)-1] = 0;
	slave_address[strlen(slave_address)-1] = 0;


	fprintf(stdout, "master %s\nslave %s\n", master_address, slave_address);


	TTC::Server master, slave;

	slave.StringKey();
	slave.SetTableName("*");
	slave.SetAddress(slave_address);
	slave.SetTimeout(3);

	int ret=0;
	if ((ret = slave.Ping()) != 0 && ret != -TTC::EC_TABLE_MISMATCH)
		return -1; 

	master.SetAddress(master_address);
	master.SetTimeout(3);
	master.CloneTabDef(slave);
	master.SetAutoUpdateTab(false);
	if ((ret = master.Ping()) != 0)
		return -1;

	TTC::SvrAdminRequest request(&master);
	request.SetAdminCode(TTC::QueryBinlogID);

	TTC::Result result; 
	request.Execute(result);

	if (result.ResultCode() != 0)
		return -1;

	id  = result.BinlogID();
	off = result.BinlogOffset();

	fprintf(stdout, "master binlog %09u, %09u\n", id, off);
	return 0;
}

int main()
{
	fprintf(stdout, "\n");

	if (access(ASYNC_FILE_CONTROLLER, F_OK | R_OK)) {
		fprintf(stdout,
			"hbp status: UNKNOWN\n\terror: controller file not exist, %m\n");
		return -1;
	}

	CAsyncFileController file_controller;

	if (file_controller.Init()) {
		fprintf(stdout,
			"hbp status: UNKNOWN\n\terror: controller file init failed, %s\n",
			file_controller.ErrorMessage());
		return -2;
	}

	if (file_controller.JournalID().Zero()) {
		fprintf(stdout, "hbp status: INIT\n");
	} else {
		if (file_controller.IsDirty()) {
			fprintf(stdout, "hbp status: FULL-SYNC\n");
		} else {
			fprintf(stdout, "hbp status: INC-SYNC\n");
		}
	}

	fprintf(stdout, "====================================\n");

	
	query_binlog_status_from_master("../hbp/server_address");

	fprintf(stdout, "slave binlog  %09u, %09u\n",
		file_controller.JournalID().serial,
		file_controller.JournalID().offset);
	fprintf(stdout, "writer pos    %09u, %09u\n",
		file_controller.WriterPos().serial,
		file_controller.WriterPos().offset);
	fprintf(stdout, "reader pos    %09u, %09u\n",
			file_controller.ReaderPos().serial,
			file_controller.ReaderPos().offset);

	fprintf(stdout, "====================================\n");

	return 0;
}
