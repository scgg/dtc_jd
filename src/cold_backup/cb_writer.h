/*
 * =====================================================================================
 *
 *       Filename:  cb_writer.h
 *
 *    Description:  cold backup log-writer
 *
 *        Version:  1.0
 *        Created:  05/14/2009 10:06:53 AM
 *       Revision:  $Id: cb_writer.h 1842 2009-05-26 08:38:10Z jackda $
 *       Compiler:  gcc
 *
 *         Author:  jackda (), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#ifndef __TTC_CB_WRITER_H
#define __TTC_CB_WRITER_H

#include "cb_encoder.h"

#define CB_LOG_FILE_BEGIN_SERIAL	1
#define CB_LOG_FILE_SLICE		1024*1024*1024*2UL
#define CB_LOG_FILE_PREFIX		"cb_log"
#define CB_LOG_FILE_SWITCH_PREFIX	"cb_switch"
#define CB_LOG_FILE_MAGIC		0x6164610a
#define CB_LOG_FILE_VERSION		0x01


namespace CB
{

	struct cb_log_header
	{
		uint32_t magic;
		uint32_t version;
		uint32_t reserve[16];
		uint8_t  endof[0];

		cb_log_header() {
			memset(this, 0x00, sizeof(cb_log_header));
			magic   = CB_LOG_FILE_MAGIC;
			version = CB_LOG_FILE_VERSION;
		}
	}__attribute__((__aligned__(1)));

	template <class T>
	class cb_writer 
	{
		public: 
			cb_writer();
			virtual ~cb_writer();

			cb_writer & operator <<(CBinary v);

			int open(const char *file);
			int commit();
			int cancle();

			const char *error_message() const{
				return _errmsg;
			}
			
			size_t size() {
				return _cur_pos;
			}

			const char *filename() {
				return _filename;
			}

		private:
			int create_target_file();
			int format_target_file();
			int open_target_file();
			int close_target_file();
			int redress_fd_pos();
			void shift_fd_pos(off_t);

		private:
			T 	_encoder;
			int 	_cur_fd;
			size_t  _cur_pos;
			char	_filename[256];
			char	_errmsg[256];
	};

	template <class T>
	class cb_advanced_writer
	{
		public:
			cb_advanced_writer();
			virtual ~cb_advanced_writer();

			int open(const char *dir, size_t slice);
			int commit();
			int cancle();

			cb_advanced_writer & operator<<(CBinary);

			const char *error_message() const {
				return _errmsg;
			}

		private:
			cb_writer<T>* create_cb_writer_instance();
			int scan_current_dir();
			int safe_switch_next_file();
			int generate_file_name(char *ptr, unsigned size);

		private:
			cb_writer <T> 	* _writer;
			size_t 		  _slice;
			unsigned 	  _serial;
			char 		  _dir[256];
			char		  _errmsg[256];
	};
};

#endif
