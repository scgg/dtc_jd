/*
 * =====================================================================================
 *
 *       Filename:  ttcrecovery.cc
 *
 *    Description:  ttc cold backup recovery
 *
 *        Version:  1.0
 *        Created:  05/25/2009 05:31:37 PM
 *       Revision:  $Id: ttcrecovery.cc 1842 2009-05-26 08:38:10Z jackda $
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

#include "cb_reader.h"
#include "cb_decoder.h"
#include "cb_recovery.h"
#include "cb_recovery.inl"

/* global var */
static char * slave_address;
static char * binlog_backup_dir_or_file;


/* usage */
static void print_usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [OPTIONS] \n"
			"recovery TTC/bitmap binlog to memory\n"
			"OPTIONS:\n"
			"\t-s, --slave         slave address\n"
			"\t-d, --backdir       binlog backup dir\n"
			"\t-h, --help          display this help info\n\n"
			"Example: %s -s 127.0.0.1:9898/tcp -d /data/backup/ttc/\n",
			argv[0], argv[0]
		);

	return;
}

/* parse user input arguments */
static int parse_argv(int argc, char **argv)
{
	if(argc < 3) {
		print_usage(argc, argv);
		return -1;
	}

	int option_index = 0, c = 0;
	static struct option long_options[] = {
		{"slave", 1, 0, 's'},
		{"backdir", 1, 0, 'd'},
		{"help", 0, 0, 'h'},
		{0, 0, 0, 0},
	};

#define V_OPT(argc, argv)	do {\
	if(!(optarg && optarg[0])) {\
		print_usage(argc, argv);\
		return -1;\
	}\
}while(0)

	while ((c = getopt_long(argc, argv, "s:d:h", long_options,
					&option_index)) != -1) {
		switch (c) {
			case -1:
			case 0:
				break;
			case 's':
				V_OPT(argc, argv);
				slave_address = optarg;
				break;
			case 'd':
				V_OPT(argc, argv);
				binlog_backup_dir_or_file = optarg;
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

	if(!(slave_address && slave_address[0]) || 
			!(binlog_backup_dir_or_file && binlog_backup_dir_or_file[0])) {
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

	cb_recovery< cb_reader<cb_decoder> > cold_backup_recovery;
	if(cold_backup_recovery.open(slave_address, binlog_backup_dir_or_file)) {
		return -1;
	}


	if(cold_backup_recovery.run()) {
		return -1;
	}

	return 0;
}
