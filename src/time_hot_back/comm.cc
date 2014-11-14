#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <getopt.h>
#include "comm.h"
#include "ttcapi.h"
#include "config.h"
#include "dbconfig.h"
#include "log.h"
#include "global.h"
#include "async_file.h"

TTC::Server CComm::master;
TTC::Server CComm::slave;
CConfig CComm::config;
CRegistor CComm::registor;
char * CComm::tablename = NULL;

const char *CComm::version = "time_hot_back0.1";
int CComm::backend = 1;
int CComm::fixed = 0;
int CComm::purge = 0;
int CComm::normal = 1;

void CComm::show_usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [OPTION]...\n"
		"Sync TTC/bitmap master data to TTC/bitmap slave.\n\n"
		"\t -n, --normal   normal running mode.\n"
		"\t -f, --fixed    when something wrong, use this argument to fix error.\n"
		"\t -p, --purge    when switch slave, use this argument to purge dirty data.\n"
		"\t -b, --backend  runing in background.\n"
		"\t -v, --version  show version.\n"
        "\t -t, --table.conf    master ttc's table.conf.\n"
        "\t -h, --help     display this help and exit.\n\n", argv[0]);
	return;
}

void CComm::show_version(int argc, char **argv)
{
	fprintf(stderr, "%s(%s)\n", argv[0], version);
	return;
}

void CComm::parse_argv(int argc, char **argv)
{
	if (argc < 3) {
		show_usage(argc, argv);
		exit(-1);
	}

	int option_index = 0, c = 0;
	static struct option long_options[] = {
		{"normal", 0, 0, 'n'},
		{"fixed", 0, 0, 'f'},
		{"purge", 0, 0, 'p'},
		{"backend", 0, 0, 'b'},
		{"version", 0, 0, 'v'},
		{"help", 0, 0, 'h'},
		{"tablename", 1, 0, 't'},
		{0, 0, 0, 0},
	};

	while ((c =
		getopt_long(argc, argv, "nfpbvht:", long_options,
			    &option_index)) != -1) {
		switch (c) {
		case 'n':
			normal = 1;
			break;

		case 'f':
			fixed = 1;
			break;

		case 'p':
			purge = 1;
			break;

		case 'b':
			backend = 1;
			break;;

		case 'v':
			show_version(argc, argv);
			exit(0);
			break;

		case 'h':
			show_usage(argc, argv);
			exit(0);
			break;

		case 't':

            tablename = strdup(optarg);
			break;

		case '?':
		default:
			show_usage(argc, argv);
			exit(-1);
		}
	}

	if (optind < argc) {
		show_usage(argc, argv);
		exit(-1);
	}

	if (fixed && purge) {
		show_usage(argc, argv);
		exit(-1);
	}

	return;
}

int CComm::connect_ttc_server(int ping_master)
{
	log_notice("try to ping master/slave server");

	int ret = 0;
	const char *master_addr = config.GetStrVal("SYSTEM", "MasterAddr");
	const char *slave_addr = config.GetStrVal("SYSTEM", "SlaveAddr");

	slave.StringKey();
	slave.SetTableName("*");
	slave.SetAddress(slave_addr);
	slave.SetTimeout(30);

	if ((ret = slave.Ping()) != 0 && ret != -TTC::EC_TABLE_MISMATCH) {
		log_error("ping slave[%s] failed, err: %d", slave_addr, ret);
		return -1;
	}

	log_notice("ping slave[%s] success", slave_addr);

	if(ping_master) {
	    master.SetAddress(master_addr);
	    master.SetTimeout(30);
	    master.StringKey();
	    master.SetTableName("*");
	    //		master.CloneTabDef(slave);	/* 从备机复制表结构 */
	    master.SetAutoUpdateTab(false);
	    if ((ret = master.Ping()) != 0 && ret != -TTC::EC_TABLE_MISMATCH) {
		log_error("ping master[%s] failed, err:%d", master_addr, ret);
		return -1;
	    }

		log_notice("ping master[%s] success", master_addr);
	}

	return 0;
}

int CComm::load_config()
{
	const char *f = strdup(SYS_CONFIG_FILE);

	if (config.ParseConfig(f, "SYSTEM")) {
		fprintf(stderr, "parse config %s failed\n", f);
		return -1;
	}
    
    free((void *)f);

	return 0;
}

int CComm::check_table_define()
{
    CDbConfig *master_dbConfig;
    CTableDefinition *master_gTableDef;
    CDbConfig *slave_dbConfig;
    CTableDefinition *slave_gTableDef;
    char _errmsg[1024];
    memset(_errmsg,0x0,sizeof(_errmsg));

    master_dbConfig = CDbConfig::Load(CComm::tablename);
    if(master_dbConfig == NULL)
    {
        snprintf(_errmsg,sizeof(_errmsg),"load master table configuire file error.\n");
        log_error(_errmsg);
        fprintf(stderr,_errmsg);
        return -2;
    }

    master_gTableDef = master_dbConfig->BuildTableDefinition();
    if(master_gTableDef == NULL)
    {
        snprintf(_errmsg,sizeof(_errmsg),"build master table definition error.\n");
        log_error(_errmsg);
        fprintf(stderr,_errmsg);
        return -3;
    }
    slave_dbConfig = CDbConfig::Load(SLAVE_TABLE_CONF);
    if(slave_dbConfig == NULL)
    {
        snprintf(_errmsg,sizeof(_errmsg),"load slave table configuire file error.\n");
        log_error(_errmsg);
        fprintf(stderr,_errmsg);
        return -4;
    }

    slave_gTableDef = slave_dbConfig->BuildTableDefinition();
    if(slave_gTableDef == NULL)
    {
        snprintf(_errmsg,sizeof(_errmsg),"build slave table definition error.\n");
        log_error(_errmsg);
        fprintf(stderr,_errmsg);
        return -4;
    }

    //检查主备机tablename是否一样
    const char *master_tablename = master_gTableDef->TableName();
    const char *slave_tablename = slave_gTableDef->TableName();
    int master_tablename_len = strlen(master_tablename);
    int slave_tablename_len = strlen(slave_tablename);

    if((master_tablename_len != slave_tablename_len) || (strncmp(master_tablename,slave_tablename,master_tablename_len) != 0))
    {
        snprintf(_errmsg,sizeof(_errmsg),"master tablename[%s] is diff from slave tablename[%s].\n",
                master_tablename,slave_tablename);
        log_error(_errmsg);
        fprintf(stderr,_errmsg);
        return -5;
    }


    //如果主机和备机字段数一样，或者主机比备机少一个字段，且主机没有有lastcmod字段，备机有lastcmod，检查字段是否一样
    int master_field_num = master_gTableDef->NumFields();
    int slave_field_num = slave_gTableDef->NumFields();
    int master_lastcmod = master_gTableDef->LastcmodFieldId();
    int slave_lastcmod = slave_gTableDef->LastcmodFieldId();

    //检查备机是否有lastcmod字段
    if(slave_lastcmod <=0)
    {
        snprintf(_errmsg,sizeof(_errmsg),"slave ttc have no lastcmod field.\n");
        log_error(_errmsg);
        fprintf(stderr,_errmsg);
        return -5;
    }
   //主机没有lastcmod，那么备机的lastcmod必须在最后一个字段 
    if(master_lastcmod <= 0 && slave_lastcmod != slave_field_num)
    {
        snprintf(_errmsg,sizeof(_errmsg),"master ttc have no lastcmod field,"
                "slave ttc have lastcmod field but not the last field.\n");
        log_error(_errmsg);
        fprintf(stderr,_errmsg);
        return -5;
    }

    //如果主机有lastcmod字段，但ID与备机不一致
    if(master_lastcmod > 0 && master_field_num != slave_field_num)
    {
        snprintf(_errmsg,sizeof(_errmsg),"master ttc have lastcmod,but it's id is diff from slave ttc's id.\n");
        log_error(_errmsg);
        fprintf(stderr,_errmsg);
        return -5;
    }

    if((master_field_num == slave_field_num) || 
            ((master_field_num == (slave_field_num -1)) && master_lastcmod<=0 && slave_lastcmod == slave_field_num))
    {
        for(int i = 0; i< master_field_num; i++)
        {
            //字段名称
            const char *master_field_name = master_gTableDef->FieldName(i);
            const char *slave_field_name = slave_gTableDef->FieldName(i);
            if(strlen(master_field_name) != strlen(slave_field_name) || 
                    strncmp(master_field_name,slave_field_name,strlen(master_field_name)) != 0)
            {
                snprintf(_errmsg,sizeof(_errmsg),"the field id[%d] have different name in master[%s] and slave[%s].\n",
                        i,master_field_name,slave_field_name);
                log_error(_errmsg);
                fprintf(stderr,_errmsg);
                return -6;
            }
            //字段类型
            if(master_gTableDef->FieldType(i) != slave_gTableDef->FieldType(i))
            {
                snprintf(_errmsg,sizeof(_errmsg),"field[%s] type is diff: master[%d] \t salve[%d].\n",
                        master_field_name,master_gTableDef->FieldType(i),slave_gTableDef->FieldType(i));
                log_error(_errmsg);
                fprintf(stderr,_errmsg);
                return -7;
            }
            //字段大小
            if(master_gTableDef->FieldSize(i) != slave_gTableDef->FieldSize(i))
            {
                snprintf(_errmsg,sizeof(_errmsg),"field[%s] size is diff: master[%d] \t salve[%d].\n",
                        master_field_name,master_gTableDef->FieldSize(i),slave_gTableDef->FieldSize(i));
                log_error(_errmsg);
                fprintf(stderr,_errmsg);
                return -8;
            }
        }
    }
    else
    {
        snprintf(_errmsg,sizeof(_errmsg),"master ttc's tabledefine is diff from slave ttc's tabledefine.\n");
        log_error(_errmsg);
        fprintf(stderr,_errmsg);
        return -9;
    }
  
    return 0;
}

int CComm::check_hb_status()
{
	CAsyncFileChecker checker;

	if (checker.Check()) {
		log_error
		    ("check hb status, __NOT__ pass! errmsg: %s, try use --fixed parament to start",
		     checker.ErrorMessage());
		return -1;
	}

	log_notice("check hb status, passed");

	return 0;
}

int CComm::fixed_hb_env()
{
	/* FIXME: 简单删除，后续再考虑如何恢复 */
	if (system("cd ../bin/ && ./hb_fixed_env.sh hbp")) {
		log_error("invoke hb_fixed_env.sh hbp failed, %m");
		return -1;
	}

	log_notice("fixed hb env, passed");

	return 0;
}

int CComm::fixed_slave_env()
{
	if (system("cd ../bin/ && ./hb_fixed_env.sh slave")) {
		log_error("invoke hb_fixed_env.sh slave failed, %m");
		return -1;
	}

	log_notice("fixed slave env, passed");

	return 0;
}

/* 确保hbp唯一, 锁住hbp的控制文件目录 */
int CComm::uniq_lock(const char *p)
{
	if (access(p, F_OK | X_OK))
		mkdir(p, 0777);

	int fd = open(p, O_RDONLY);
	if (fd < 0)
		return -1;

	fcntl(fd, F_SETFD, FD_CLOEXEC);

	return flock(fd, LOCK_EX | LOCK_NB);
}

int CComm::log_server_info(const char *p)
{
	if(access(p, F_OK|X_OK))
		mkdir(p, 0777);

	char info_name[256];
	snprintf(info_name, sizeof(info_name), "%s/%s", p, "server_address");

	FILE *fp = fopen(info_name, "w");
	if(!fp) {
		log_notice("log server info failed, %m");
		return -1;
	}

	const char *master_addr = config.GetStrVal("SYSTEM", "MasterAddr");
	const char *slave_addr = config.GetStrVal("SYSTEM", "SlaveAddr");

	int n;
	n = fwrite(master_addr, strlen(master_addr), 1, fp);
	n = fwrite("\n", 1, 1, fp);

	n = fwrite(slave_addr, strlen(slave_addr), 1, fp);
	n = fwrite("\n", 1, 1, fp);

	fclose(fp);
	return 0;
}

