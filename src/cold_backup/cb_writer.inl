/*
 * =====================================================================================
 *
 *       Filename:  cb_writer.inl
 *
 *    Description:  cb writer
 *
 *        Version:  1.0
 *        Created:  05/14/2009 10:47:59 AM
 *       Revision:  $Id: cb_writer.inl 3911 2009-11-17 08:54:22Z samuelliao $
 *       Compiler:  gcc
 *
 *         Author:  jackda (), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#include <time.h>               
#include <stddef.h>             
#include <stdint.h>             
#include <string.h>             
#include <sys/uio.h>            
#include <sys/types.h>  
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>

using namespace CB;

template <class T>
cb_writer<T>::cb_writer()
{
	_cur_fd  = -1;
	_cur_pos = 0;

	bzero(_errmsg, sizeof(_errmsg));
	bzero(_filename, sizeof(_filename));
}

template <class T>
cb_writer<T>::~cb_writer()
{
	close_target_file();
}

template <class T>
cb_writer<T>& cb_writer<T>::operator<< (CBinary v)
{
	_encoder << v;
	return *this;
}

template <class T>
int cb_writer<T>::cancle()
{
	return _encoder.cancle();
}

template <class T>
int cb_writer<T>::close_target_file()
{
	if(_cur_fd > 0) {
		::close(_cur_fd);
		_cur_fd  = -1;
		_cur_pos = 0;
	}

	return 0;
}

template <class T>
int cb_writer<T>::commit()
{
	if(_encoder.encode()) {
		snprintf(_errmsg, sizeof(_errmsg),
				"encode buffer failed");
		return -1;
	}
	
	int w_size 	 = _encoder.size();
	CBinary	w_buffer = _encoder.get_encoded_buffer(); 

	struct iovec iov[2] = {
				{&w_size, sizeof(w_size)},
				{w_buffer.ptr, w_buffer.len}
			      };

	if(::writev(_cur_fd, iov, 2)  != (ssize_t)sizeof(w_size)+w_buffer.len)
	{
		redress_fd_pos();
		snprintf(_errmsg, sizeof(_errmsg), 
				"writev user data failed: errno=%d, errmsg=%m", errno);
		return -1;
	}

	shift_fd_pos((off_t)(sizeof(w_size)+ w_buffer.len));

	/* reset encoder */
	_encoder.cancle();

	return 0;
}

template <class T>
int cb_writer<T>::redress_fd_pos()
{
	return ::lseek(_cur_fd, _cur_pos, SEEK_SET);
}

template <class T>
void cb_writer<T>::shift_fd_pos(off_t s)
{
	_cur_pos += s;
}

template <class T>
int cb_writer<T>::open(const char *file)
{
	if(!(file && file[0])) {
		snprintf(_errmsg, sizeof(_errmsg), "log name is invalid"); 
		return -1;
	}
	strncpy(_filename, file, sizeof(_filename));


	if(_cur_fd > 0) {
		return 0;
	}

	if(access(_filename, F_OK)) {
		if(create_target_file())
			return -1;
		if(format_target_file())
			return -1;
	}else {
		if(open_target_file())
			return -1;
	}

	return 0;
}

#ifndef O_NOATIME
#define O_NOATIME 0
#endif
template <class T>
int cb_writer<T>::create_target_file()
{
	_cur_fd = ::open(_filename, O_CREAT|O_LARGEFILE|O_NOATIME|O_TRUNC|O_WRONLY, 0644);
	if(_cur_fd < 0) {
		snprintf(_errmsg, sizeof(_errmsg), 
				"create [%s] failed: errno=%d, errmsg=%m", _filename, errno);
		return -1;
	}

	_cur_pos = 0;
	return 0;
}

template <class T>
int cb_writer<T>::format_target_file()
{
	struct cb_log_header l_hdr;
	
	if(::write(_cur_fd, &l_hdr, sizeof(l_hdr)) != sizeof(l_hdr)) {
		snprintf(_errmsg, sizeof(_errmsg),
				"format [%s] failed: errno=%d, errmsg=%m", _filename, errno);
		redress_fd_pos();
		return -1;
	}

	shift_fd_pos((off_t)sizeof(l_hdr));
	return 0;
}

template <class T>
int cb_writer<T>::open_target_file()
{
	_cur_fd = :: open(_filename, O_APPEND|O_LARGEFILE|O_NOATIME|O_RDWR, 0644);
	if(_cur_fd < 0) {
		snprintf(_errmsg, sizeof(_errmsg),
				"open [%s] failed: errno=%d, errmsg=%m", _filename, errno);
		return -1;
	}

	/* valid log-file header*/
	struct cb_log_header l_hdr;
	bzero(&l_hdr, sizeof(l_hdr));

	if(::read(_cur_fd, &l_hdr, sizeof(l_hdr)) != sizeof(l_hdr)) {
		snprintf(_errmsg, sizeof(_errmsg),
				"read [%s] failed: errno=%d, errmsg=%m", _filename, errno);
		return -1;
	}
	if(l_hdr.magic != CB_LOG_FILE_MAGIC) {
		snprintf(_errmsg, sizeof(_errmsg), "[%s]'s maigc mismatch", _filename);
		return -1;
	}
	if(l_hdr.version != CB_LOG_FILE_VERSION) {
		snprintf(_errmsg, sizeof(_errmsg), "[%s]'s version mismatch", _filename);
		return -1;
	}

	shift_fd_pos((off_t)::lseek(_cur_fd, 0, SEEK_END));
	return 0;
}

//////////////////////////////////////////////////////////////////////

template <class T>
cb_advanced_writer<T>::cb_advanced_writer()
{
	_writer = 0;
	_slice	= 0;
	_serial = 0 ;
	bzero(_dir, sizeof(_dir));
	bzero(_errmsg, sizeof(_errmsg));
}

template <class T>
cb_advanced_writer<T>::~cb_advanced_writer()
{
	DELETE(_writer);
}

template <class T>
int cb_advanced_writer<T>::open(const char *dir, size_t slice)
{
	if(!(dir && dir[0])) {
		snprintf(_errmsg, sizeof(_errmsg), "log dir is invalid"); 
		return -1;
	}

	struct stat st;
	stat(dir, &st);
	if(!S_ISDIR(st.st_mode)){
		snprintf(_errmsg, sizeof(_errmsg), "log dir is invalid, %s",dir); 
		return -1;
	}

	_slice = slice <= 0 ? CB_LOG_FILE_SLICE : slice;
	strncpy(_dir, dir, sizeof(_dir));
	
	if(!_writer)
		_writer = create_cb_writer_instance();
	if(!_writer)
		return -1;

	return 0;
}

template <class T>
cb_advanced_writer<T>& cb_advanced_writer<T>::operator <<(CBinary r)
{
	_writer->operator<<(r);
	return *this;
}

template <class T>
int cb_advanced_writer<T>::commit()
{
	int ret = _writer->commit();
	if(ret) {
		snprintf(_errmsg, sizeof(_errmsg), 
				"commit to log failed: %s",
				_writer->error_message()
			);
		return ret;
	}

	/* switch next file */
	if(_writer->size() >= _slice) {
		if(safe_switch_next_file()) {
			return -1;
		}
	}

	return 0;
}

template <class T>
int cb_advanced_writer<T>::safe_switch_next_file()
{
	_serial += 1;

	cb_writer<T>* t_writer = create_cb_writer_instance();
	if(!t_writer) {
		return -1;
	}

	/* create switch file and write next serial number */
	char ctrl_f[256] = {0};
	snprintf(ctrl_f, sizeof(ctrl_f), "%s/%s.%d",
			_dir,
			CB_LOG_FILE_SWITCH_PREFIX,
			_serial-1
		);

	::creat(ctrl_f, 0644);
	if(::access(ctrl_f, F_OK)) {
		snprintf(_errmsg, sizeof(_errmsg),
				"can't create switch file, %s", ctrl_f);
		return -1;
	}

	int sw_fd = ::open(ctrl_f, O_WRONLY|O_NOATIME|O_TRUNC, 0644);
	int unused;
	unused = ::write(sw_fd, &_serial, sizeof(_serial));
	::close(sw_fd);

	/* now, everything is ready */
	DELETE(_writer);
	_writer = t_writer;

	return 0;
}

template <class T>
int cb_advanced_writer<T>::generate_file_name(char *ptr, unsigned size)
{
	if(_serial == 0) {
		scan_current_dir();
	}

	snprintf(ptr, size, "%s/%s.%d", _dir, CB_LOG_FILE_PREFIX, _serial);
	return 0;
}

template <class T>
int cb_advanced_writer<T>::scan_current_dir()
{
	DIR *dir = opendir(_dir);
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

	for(;drt;drt=readdir(dir))
	{
		int n = strncmp(drt->d_name, prefix, l);
		if(n == 0)
		{
			v = strtoul(drt->d_name+l, NULL, 10);
			_serial = _serial < v ? v : _serial;
		}
	}

	closedir(dir);
	return 0;
}

template <class T>
int cb_advanced_writer<T>::cancle()
{
	return _writer -> cancle();
}

template <class T>
cb_writer<T>* cb_advanced_writer<T>::create_cb_writer_instance()
{
	char log_f[256] = {0};
	generate_file_name(log_f, sizeof(log_f));

	cb_writer<T> * n_writer = 0;
	NEW(cb_writer<T>(), n_writer);
	if(!n_writer) {
		snprintf(_errmsg, sizeof(_errmsg), 
				"create cb_writer obj failed, %s",log_f);
		return NULL;
	}

	if(n_writer->open(log_f)) {
		snprintf(_errmsg, sizeof(_errmsg), "%s",
				n_writer->error_message());
		DELETE(n_writer);
		return NULL;
	}

	return n_writer;
}
