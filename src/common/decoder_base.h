#ifndef __H_TTC_DECODER_UNIT_H__
#define __H_TTC_DECODER_UNIT_H__

class CPollThread;
class CTimerList;

class CDecoderUnit
{
	public:
		CDecoderUnit(CPollThread *, int idletimeout);
		virtual ~CDecoderUnit();

		CTimerList *IdleList(void) const { return idleList; }
		int IdleTime(void) const { return idleTime; }
		CPollThread *OwnerThread(void) const { return owner; }

		virtual int ProcessStream(int fd, int req, void *, int) = 0;
		virtual int ProcessDgram(int fd) = 0;

	protected:
		CPollThread *owner;
		int idleTime;
		CTimerList *idleList;
};

#endif
