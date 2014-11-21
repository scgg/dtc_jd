/*
 * =====================================================================================
 *
 *       Filename:  hitinfo.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/06/2014 12:54:13 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  linjinming, linjinming@jd.com
 *        Company:  JD, China
 *
 * =====================================================================================
 */

#include "hitinfo.h"


CHitInfo::CHitInfo()
{
	m_isGet = false;
	m_isHit = false;
}

CHitInfo::~CHitInfo()
{

}

bool CHitInfo::isGet()
{
	return m_isGet;
}

bool CHitInfo::isHit()
{
	return m_isHit;
}

void CHitInfo::setGet(bool get)
{
	m_isGet = get;
}

void CHitInfo::setHit(bool hit)
{
	m_isHit = hit;
}


