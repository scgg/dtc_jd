/*
 * =====================================================================================
 *
 *       Filename:  keydump.cc
 *
 *    Description:  dump all keys in ttc
 *
 *        Version:  1.0
 *        Created:  08/30/2009 08:41:30 AM
 *       Revision:  $Id$
 *       Compiler:  g++
 *
 *         Author:  foryzhou (), foryzhou@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "kd_syncer.h"

using namespace KD;
/* global var */
static char * g_ttc_address;
static char * g_keydump_file;
static int    g_dump_step;
static int    g_dump_timeout;

/* usage */
static void print_usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [OPTIONS] \n"
			"dump TTC keys to keydump file\n"
			"OPTIONS:\n"
			"\t-a, --address       ttc address\n"
			"\t-s, --step          dump step\n"
			"\t-t, --timeout       dump timeout(s)\n"
			"\t-f, --dumpfile       keydump file's name\n"
			"\t-h, --help          display this help info\n\n"
			"Example: %s -a 127.0.0.1:9898/tcp -s 100 -t 3600 -f ./keydump.log \n",
			argv[0], argv[0]
		);

	return;
}

/* parse user input arguments */
static int parse_argv(int argc, char **argv)
{
	if(argc < 5) {
		print_usage(argc, argv);
		return -1;
	}

	int option_index = 0, c = 0;
	static struct option long_options[] = {
		{"address", 1, 0, 'a'},
		{"step", 1, 0, 's'},
		{"timeout", 1, 0, 't'},
		{"dumpfile", 1, 0, 'f'},
		{"help", 0, 0, 'h'},
		{0, 0, 0, 0},
	};

#define V_OPT(argc, argv)	do {\
	if(!(optarg && optarg[0])) {\
		print_usage(argc, argv);\
		return -1;\
	}\
}while(0)

	while ((c = getopt_long(argc, argv, "a:s:t:f:h", long_options,
					&option_index)) != -1) {
		switch (c) {
			case -1:
			case 0:
				break;
			case 'a':
				V_OPT(argc, argv);
				g_ttc_address = optarg;
				break;
			case 's':
				V_OPT(argc, argv);
				g_dump_step = atoi(optarg);
				break;
			case 't':
				V_OPT(argc, argv);
				g_dump_timeout = atoi(optarg);
				break;
			case 'f':
				V_OPT(argc, argv);
				g_keydump_file = optarg;
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

	if(!(g_ttc_address && g_ttc_address[0]) || !(g_keydump_file && g_keydump_file[0])) {
		print_usage(argc, argv);
		return -1;
	}
		
	return 0;
}


int main(int argc, char **argv)
{
	if(parse_argv(argc, argv)) {
		return -1;
	}
	
	
	kd_syncer keydump_syncer;
	if (keydump_syncer.open(g_ttc_address,
				g_dump_step,
				g_dump_timeout,
				g_keydump_file
			       )) 
	{
		return -1;

	}

	if (keydump_syncer.run()) { 
		return -1;
	}

	return 0;
}
