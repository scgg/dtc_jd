/*
 * =====================================================================================
 *
 *       Filename:  hitinfo.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/06/2014 17:22:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  linjinming, linjinming@jd.com
 *        Company:  JD, China
 *
 * =====================================================================================
 */

#ifndef __HITINFO_H__
#define __HITINFO_H__

class CHitInfo
{
public:
	CHitInfo();
	~CHitInfo();

	bool isGet();
	bool isHit();
	void setGet(bool get);
	void setHit(bool hit);
private:
	bool m_isGet;
	bool m_isHit;
};

#endif
