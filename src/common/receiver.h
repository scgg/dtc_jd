#ifndef __RECVEIVER_H__
#define __RECVEIVER_H__

#include <unistd.h>
#include "protocol.h"

class CSimpleReceiver {
	private:
		struct CPacketHeader _header;
		char *_packet;
		int _netfd;
		int _count;
		int _offset;
	public:
		CSimpleReceiver(int fd=-1) : _packet(NULL), _netfd(fd), _count(0), _offset(0) {};
		~CSimpleReceiver(void) {};

		void attach(int fd) { _netfd = fd; }
		void erase(void) { _packet = NULL; _count = 0; _offset = 0; }
		int remain(void) const { return _count - _offset; }
		int discard(void) {
			char buf[1024];
			int len;
			const int bsz = sizeof(buf);
			while( (len = remain()) > 0)
			{
				int rv = read(_netfd, buf, (len>bsz?bsz:len));
				if(rv <= 0) 
					return rv;
				_offset += rv;
			}
			return 1;
		}
		int fill(void) {
			int rv = read(_netfd, _packet + _offset, remain());
			if(rv > 0) 
				_offset += rv;
			return rv;
		}
		void init(void) { _packet = (char *)&_header; _count = sizeof(_header); _offset = 0; }
		void set(char *p, int sz) { _packet = p; _count = sz; _offset = 0; }
		char *c_str(void) { return _packet; }
		const char *c_str(void) const { return _packet; }
		CPacketHeader &header(void) { return _header; }
		const CPacketHeader &header(void) const { return _header; }
};


#endif


