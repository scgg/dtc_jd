#ifndef __CH_PACKET_H__
#define __CH_PACKET_H__



#include "protocol.h"
#include "section.h"
#include "sockaddr.h"


class NCRequest;
union CValue;
class CTask;
class CTaskRequest;
class CTableDefinition;

enum CSendResult {
	SendResultError,
	SendResultMoreData,
	SendResultDone
};

struct iovec
{
	void *iov_base; /* Pointer to data. */
	size_t iov_len; /* Length of data. */
};

/* just one buff malloced */
class CPacket {
private:
	iovec *v;
	int nv;
	int bytes;
	CBufferChain *buf;
	int sendedVecCount;

	CPacket(const CPacket &);
public:
	CPacket() : v(NULL), nv(0), bytes(0), buf(NULL), sendedVecCount(0) { };
	~CPacket() {
		/* free buffer chain buffer in several place, not freed all here */
		FREE_IF(buf);
	}
	int Send(int fd);
	inline void Clean()
	{
		v = NULL;
		nv = 0;
		bytes = 0;
		if(buf)
			buf->Clean();
	}
	int EncodeRequest(NCRequest &r, const CValue *k=NULL);
	static int EncodeHeader(CPacketHeader &header);
	char *AllocateSimple(int size);
	static int EncodeSimpleRequest(NCRequest &rq, const CValue *kptr, char * &ptr, int &len);

#if __BYTE_ORDER == __LITTLE_ENDIAN
	static int EncodeHeader(const CPacketHeader &header);
#endif



};

#endif
