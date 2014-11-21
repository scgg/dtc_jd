/*
 * =====================================================================================
 *
 *       Filename:  ttc_dump.cc
 *
 *    Description:  ttc cold backup tools
 *
 *        Version:  1.0
 *        Created:  05/15/2009 04:04:36 PM
 *       Revision:  $Id: ttcdump.cc 1842 2009-05-26 08:38:10Z jackda $
 *       Compiler:  gcc
 *
 *         Author:  jackda (), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "cb_syncer.h"
#include "cb_syncer.inl"
#include "cb_writer.h"
#include "cb_writer.inl"


/* global var */
static char * master_address;
static char * binlog_backup_dir;
static int    sync_step;
static int    sync_timeout;
static size_t binlog_slice_size;

/* usage */
static void print_usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [OPTIONS] \n"
			"dump TTC/bitmap data to binlog file\n"
			"OPTIONS:\n"
			"\t-m, --master        master address\n"
			"\t-s, --step          sync step\n"
			"\t-t, --timeout       sync timeout(s)\n"
			"\t-d, --backdir       binlog backup dir\n"
			"\t-l, --slice         each binlog file slice size\n"
			"\t-h, --help          display this help info\n\n"
			"Example: %s -m 127.0.0.1:9898/tcp -s 100 -t 3600 -d /data/backup/ttc/ -l2000000000\n",
			argv[0], argv[0]
		);

	return;
}

/* parse user input arguments */
static int parse_argv(int argc, char **argv)
{
	if(argc < 6) {
		print_usage(argc, argv);
		return -1;
	}

	int option_index = 0, c = 0;
	static struct option long_options[] = {
		{"master", 1, 0, 'm'},
		{"step", 1, 0, 's'},
		{"timeout", 1, 0, 't'},
		{"backdir", 1, 0, 'd'},
		{"slice", 1, 0, 'l'},
		{"help", 0, 0, 'h'},
		{0, 0, 0, 0},
	};

#define V_OPT(argc, argv)	do {\
	if(!(optarg && optarg[0])) {\
		print_usage(argc, argv);\
		return -1;\
	}\
}while(0)

	while ((c = getopt_long(argc, argv, "m:s:t:d:l:h", long_options,
					&option_index)) != -1) {
		switch (c) {
			case -1:
			case 0:
				break;
			case 'm':
				V_OPT(argc, argv);
				master_address = optarg;
				break;
			case 's':
				V_OPT(argc, argv);
				sync_step = atoi(optarg);
				break;
			case 't':
				V_OPT(argc, argv);
				sync_timeout = atoi(optarg);
				break;
			case 'd':
				V_OPT(argc, argv);
				binlog_backup_dir = optarg;
				break;
			case 'l':
				V_OPT(argc, argv);
				binlog_slice_size = (size_t)strtoll(optarg, 0, 10);
				break;
			case 'h':
				print_usage(argc, argv);
				exit(0);
		}
	}

	if(optind < argc) {
		print_usage(argc, argv);
		return -1;
	}

	if(!(master_address && master_address[0]) || !(binlog_backup_dir && binlog_backup_dir[0])) {
		print_usage(argc, argv);
		return -1;
	}
		
	return 0;
}


using namespace CB;

int main(int argc, char **argv)
{
	if(parse_argv(argc, argv)) {
		return -1;
	}
	
	cb_syncer <cb_advanced_writer <cb_encoder> > cold_backup_syncer;
	if(cold_backup_syncer.open(master_address,
				sync_step,
				sync_timeout,
				binlog_backup_dir,
				binlog_slice_size)) {
		return -1;
	}

	if(cold_backup_syncer.run()) { 
		return -1;
	}
	
	return 0;
}
