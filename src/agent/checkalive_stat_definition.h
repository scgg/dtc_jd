/*
 * checkalive_stat_definition.h
 *
 *  Created on: 2014��8��27��
 *      Author: Jiansong
 */

#ifndef CHECKALIVE_STAT_DEFINITION_H_
#define CHECKALIVE_STAT_DEFINITION_H_

enum
{
    PING_TIMES_COUNT = 1, //ping �ܴ���ͳ��
    PING_FAIL_TIMES_COUNT, //PING ��ʧ�ܴ���ͳ��
    PING_FAIL_TIMES_COUNT_CON, //PING����ʧ�ܴ���
    PING_TIME_STAMP,           //PINGʱ�������
};

#define CHECKALIVE_STAT_DEF \
	{ PING_TIMES_COUNT,	"ping times count",SA_COUNT, SU_INT	}, \
	{ PING_FAIL_TIMES_COUNT,	"ping total fails",SA_COUNT, SU_INT	}, \
	{ PING_FAIL_TIMES_COUNT_CON,	"ping continue fails",	SA_COUNT, SU_INT	}, \
	{ PING_TIME_STAMP,	"ping time stamp",	SA_VALUE, SU_DATETIME	}

#endif /* CHECKALIVE_STAT_DEFINITION_H_ */
