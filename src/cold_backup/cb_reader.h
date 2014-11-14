/*
 * =====================================================================================
 *
 *       Filename:  cb_reader.h
 *
 *    Description:  cold backup log reader 
 *
 *        Version:  1.0
 *        Created:  05/14/2009 05:29:41 PM
 *       Revision:  $Id: cb_reader.h 3911 2009-11-17 08:54:22Z samuelliao $
 *       Compiler:  gcc
 *
 *         Author:  jackda (), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#ifndef __TTC_CB_READER_H
#define __TTC_CB_READER_H

#include <time.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include "buffer.h"
#include "cb_value.h"
#include "cb_writer.h"

namespace CB
{
	template <class T>
	class cb_reader 
	{
		public:
			cb_reader() {
				_cur_fd		  = -1;
				_cur_pos	  = 0;
				_cur_serial	  = 0;
				_is_directory	  = 0;
				_is_regular_file  = 0;
				_need_auto_switch = 0;
				bzero(_dir_or_file, sizeof(_dir_or_file));
				bzero(_errmsg, sizeof(_errmsg));
			}

			virtual ~cb_reader() {
				close_target_file();
			}

			const char *error_message() {
				return _errmsg;
			}

			int open(const char *file){
				if(!(file && file[0])) {
					snprintf(_errmsg, sizeof(_errmsg), 
							"log dir/name is invalid");
					return -1;
				}

				strncpy(_dir_or_file, file, sizeof(_dir_or_file));


				struct stat st;
				stat(file, &st);
				if(S_ISDIR(st.st_mode)){
					scan_current_dir();
					_is_directory=1;
					_need_auto_switch=1;
				}else if(S_ISREG(st.st_mode)){
					_is_regular_file = 1;
				}else {
					snprintf(_errmsg, sizeof(_errmsg),
							"unknow dir/file: %s", file);
					return -1;
				}


				if(open_target_file()) {
					return -1;
				}
				if(valid_target_file()) {
					return -1;
				}

				return 0;
			}

			/* TODO */
			int endof(){
				return 0;
			}

			int read(){

				int need_read_size = read_record_length();

				if(need_read_size == -ENOSPC) {
					/* EOF */
					return 0;
				} else if (need_read_size == -EAGAIN) {
					/* restart read() */
					return read();
				} else if(need_read_size == -EIO) {
					/* error */
					return -1;
				}

				if(_reader_buffer.resize(need_read_size)) {
					snprintf(_errmsg, sizeof(_errmsg), 
							"can't alloc reader buffer, need_size=%d", need_read_size);
					goto ERROR_PROCESS_AND_RETURN;
				}

				if(::read(_cur_fd, _reader_buffer.c_str(), need_read_size) 
						!= need_read_size) {
					snprintf(_errmsg, sizeof(_errmsg), "log content maybe corrupted: %m");
					goto ERROR_PROCESS_AND_RETURN;
				}
				
				_decoder <<  _reader_buffer;

				shift_file_pos(sizeof(need_read_size) + need_read_size);
				/* return 1 = OK */
				return 1;
ERROR_PROCESS_AND_RETURN:
				redress_file_pos();
				return -1;
			}

			int size() {
				return _decoder.size();
			}

			int decode() {
				return _decoder.decode();
			}

			CBinary operator[](int i) {
				return _decoder[i];
			}

		private:
			int read_record_length()
			{
				int need_read_size = 0, ret = 0;

				ret=::read(_cur_fd, &need_read_size, sizeof(need_read_size));

				/* normal */
				if(ret == sizeof(need_read_size)) {
					return need_read_size;  

				} else if( ret == 0) /* no more data */{
					/* auto switch next file */
					if(_need_auto_switch) {
						int next_serial = next_file_serial();
						if(next_serial < 0) {
							/* EOF */
							return -ENOSPC;
						}

						if(switch_next_file(next_serial))
							return -EIO;
						else
							/* try again*/
							return -EAGAIN;
					}
					/* EOF */
					return -ENOSPC;
				}

				/* ERROR */
				snprintf(_errmsg, sizeof(_errmsg), "log content maybe corrupted: %m");
				redress_file_pos();
				return -EIO;

			}
			int open_target_file() {
				if(_cur_fd > 0) {
					return 0;
				}
				
#ifndef O_NOATIME
#define O_NOATIME 0
#endif
				char log_f[256] = {0};
				generate_file_name(log_f, sizeof(log_f));
				_cur_fd = ::open(log_f, O_LARGEFILE|O_NOATIME|O_RDONLY, 0644);
				if(_cur_fd < 0){
					snprintf(_errmsg, sizeof(_errmsg),
							"open [%s] failed: errno=%d, errmsg=%m", log_f, errno);
					return -1;
				}

				_cur_pos = 0;
				return 0;
			}

			int close_target_file() {
				if(_cur_fd > 0) {
					::close(_cur_fd);
					_cur_fd = -1;
					_cur_pos = 0;
					_cur_serial = 0;
				}
				return 0;
			}

			int valid_target_file() {
				struct cb_log_header l_hdr;
				bzero(&l_hdr, sizeof(l_hdr));
				if(::read(_cur_fd, &l_hdr, sizeof(l_hdr)) != sizeof(l_hdr)) {
					snprintf(_errmsg, sizeof(_errmsg),
							"read log header failed: errno=%d, errmsg=%m", errno);
					return -1;
				}
				if(l_hdr.magic != CB_LOG_FILE_MAGIC) {
					snprintf(_errmsg, sizeof(_errmsg), "log maigc mismatch");
					return -1;
				}
				if(l_hdr.version != CB_LOG_FILE_VERSION) {
					snprintf(_errmsg, sizeof(_errmsg), "log version mismatch");
					return -1;
				}

				shift_file_pos(::lseek(_cur_fd, sizeof(l_hdr), SEEK_SET));
				return 0;
			}

			int shift_file_pos(size_t size) {
				_cur_pos += size;
				return 0;
			}
			int redress_file_pos() {
				return ::lseek(_cur_fd, _cur_pos, SEEK_SET);
			}
			int generate_file_name(char *ptr, unsigned size) {
				if(_is_regular_file) {
					strncpy(ptr, _dir_or_file, size);
					return 0;
				}else if(_is_directory) {
					snprintf(ptr, size, "%s/%s.%d", _dir_or_file, 
							CB_LOG_FILE_PREFIX, _cur_serial);
					return 0;
				}

				return -1;
			}
			int next_file_serial(){
				char ctrl_f[256] = {0};
				snprintf(ctrl_f, sizeof(ctrl_f), "%s/%s.%d",
						_dir_or_file,
						CB_LOG_FILE_SWITCH_PREFIX,
						_cur_serial);
				if(::access(ctrl_f, F_OK)) {
					return -1;
				}

				int next_serial = 0;	
				int sw_fd = ::open(ctrl_f, O_RDONLY|O_NOATIME, 0644);
				if(::read(sw_fd, &next_serial, sizeof(next_serial)) != sizeof(next_serial)){
					snprintf(_errmsg, sizeof(_errmsg),
							"read switch-file failed: errno=%d, errmsg=%m", errno);
					::close(sw_fd);
					return -1;
				}

				::close(sw_fd);
				return next_serial;
			}
			int scan_current_dir() {
				DIR *dir = opendir(_dir_or_file);
				if(!dir)
					return -1;

				struct dirent *drt = readdir(dir);
				if(!drt){
					closedir(dir);
					return -1;
				}

				char prefix[256] = {0};
				snprintf(prefix, sizeof(prefix), "%s.", CB_LOG_FILE_PREFIX);

				int l = strlen(prefix);
				uint32_t v =0;
				uint32_t min = (1ULL << 30);

				for(;drt;drt=readdir(dir))
				{
					int n = strncmp(drt->d_name, prefix, l);
					if(n == 0)
					{
						v = strtoul(drt->d_name+l, NULL, 10);
						min = min > v ? v : min;
					}
				}

				if(min < (1UL << 30))
					_cur_serial = min;

				closedir(dir);
				return 0;
			}
			int switch_next_file(int serial) {
				close_target_file();
				_cur_serial = serial;
				return open_target_file() || valid_target_file();
			}
		private:
			buffer	_reader_buffer;
			T	_decoder;
			int 	_cur_fd;
			size_t	_cur_pos;
			int 	_cur_serial;
			int 	_is_directory;
			int 	_is_regular_file;
			int 	_need_auto_switch;
			char	_dir_or_file[256];
			char 	_errmsg[256];
	};
};


#endif
