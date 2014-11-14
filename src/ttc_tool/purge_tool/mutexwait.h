/*
 * =====================================================================================
 *
 *       Filename:  mutexwait.h
 *
 *    Description:  limit the freq
 *
 *        Version:  1.0
 *        Created:  08/31/2009 08:11:14 AM
 *       Revision:  $Id$
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (), foryzhou@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#include <time.h>
#include <pthread.h>

class mutexwait {
    private:
	pthread_mutex_t mwait;
	struct timespec ts;
    public: 
	mutexwait(){
	    pthread_mutex_init(&mwait, NULL);
	    clock_gettime(CLOCK_REALTIME, &ts);
	}       
	~mutexwait(){pthread_mutex_destroy(&mwait);}
	void msstep(int msec) { 
	    ts.tv_nsec += 1000000*msec; // 1ms
	    while(ts.tv_nsec > 1000000000) // 1s
	    {       
		ts.tv_sec ++;
		ts.tv_nsec -= 1000000000;
	    }       
	    pthread_mutex_timedlock(&mwait, &ts);
	}       
};

