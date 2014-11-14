/*
 * =====================================================================================
 *
 *       Filename:  proccess_command.h
 *
 *    Description:  处理管理台发来的命令
 *
 *        Version:  1.0
 *        Created:  11/13/2010 05:11:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef _PROCCESS_COMMAND_H_
#define _PROCCESS_COMMAND_H_
#include <string>
#include "MarkupSTL.h"
#include "network.h"
#include "file.h"

typedef enum command
{
		UNKNOWN,
		GET_STATUS,
		MIGRATE,
		RETRY_MIGRATE,
		REDO_MIGRATE,
		MAX_COMMAND_ALLOWED
}CMD_TYPE;

class CProcessCommand
{
		public:
				CProcessCommand(int fd)
				{
						_fd = fd;
						_package_len = 0;
						_error_msg = "unknown error";
						_log_file = NULL;
				};
				~CProcessCommand()
				{
						if (_log_file) delete _log_file;
				};
				void process();
		private:
				bool recv_package(void);
				bool decode(void);
				bool process_command(void);
				bool process_migrate(void);
				bool process_redo_migrate(void);
				bool process_retry_migrate(void);
				bool process_get_status(void);
				CMD_TYPE parse_command(void);
				bool IsTaskExist(void);
				bool Lock(void);
		private:
				int _fd;
				int _package_len;
				char _buf[MAX_PACKAGE_SIZE];
				CMarkupSTL _xml;
				std::string _command;
				std::string _logfile_name;
				std::string _error_msg;
				CFile *_log_file;
};
#endif

