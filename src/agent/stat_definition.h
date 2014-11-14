#ifndef AGENT_STAT_DEF_H__
#define AGENT_STAT_DEF_H__

enum 
{
    FRONT_ACCEPT_COUNT = 5,
    FRONT_CONN_COUNT,
    GET_COUNT = 10,
    INSERT_COUNT,
    UPDATE_COUNT,
    DELETE_COUNT,
    PURGE_COUNT,
    SVR_ADMIN_COUNT,
    NOP_COUNT,
    BATCH_GET_COUNT,
    REQUEST_ELAPES,//��Ӧ��ʱ
    REQUESTS,//������
    RESPONSES,//��Ӧ��
    REQPKGSIZE,//������ܴ�С
    RSPPKGSIZE,//��Ӧ���ܴ�С
    GET_HIT,//GET_COUNT��������,�������Ӧ��ͳ���е�ƫ��
    PKG_ENTER_COUNT,//�����ܰ���
    PKG_LEAVE_COUNT,//��Ӧ�ܰ���
    OTHERS_COUNT,
};

#define AGENT_STAT_DEF \
	{ FRONT_ACCEPT_COUNT,	"network - accept reqs",SA_COUNT, SU_INT	}, \
	{ FRONT_CONN_COUNT,	"network - cur connect",SA_VALUE, SU_INT	}, \
	{ GET_COUNT,	"select reqs",	SA_COUNT, SU_INT	}, \
	{ INSERT_COUNT,	"insert reqs",	SA_COUNT, SU_INT	}, \
	{ UPDATE_COUNT,	"update reqs",	SA_COUNT, SU_INT	}, \
	{ DELETE_COUNT,	"delete reqs",	SA_COUNT, SU_INT	}, \
	{ PURGE_COUNT,	"purge reqs",	SA_COUNT, SU_INT	}, \
    { SVR_ADMIN_COUNT, "server admin reqs", SA_COUNT, SU_INT }, \
    { NOP_COUNT, "nop reqs", SA_COUNT, SU_INT }, \
    { BATCH_GET_COUNT, "batch get reqs", SA_COUNT, SU_INT }, \
    { REQUEST_ELAPES, "req elapses", SA_COUNT, SU_INT }, \
    { REQUESTS, "total reqs", SA_COUNT, SU_INT }, \
    { RESPONSES, "total resp", SA_COUNT, SU_INT }, \
    { REQPKGSIZE, "Request  Packet Size(KB)", SA_COUNT, SU_INT }, \
    { RSPPKGSIZE, "Response Packet Size(KB)", SA_COUNT, SU_INT }, \
    { GET_HIT, "get hit", SA_COUNT, SU_INT }, \
	{ PKG_ENTER_COUNT, "enter pkg count", SA_COUNT, SU_INT }, \
	{ PKG_LEAVE_COUNT, "leave pkg count", SA_COUNT, SU_INT }, \
    { OTHERS_COUNT, "other reqs", SA_COUNT, SU_INT }

#define AGENT_STAT_REPORT_DEF \
	{ GET_COUNT,	"select reqs",	SA_COUNT, SU_INT	}, \
	{ REQUEST_ELAPES, "req elapses", SA_COUNT, SU_INT }, \
    { REQUESTS, "total reqs", SA_COUNT, SU_INT }, \
    { RESPONSES, "total resp", SA_COUNT, SU_INT }, \
    { REQPKGSIZE, "Request  Packet Size xxx KB", SA_COUNT, SU_INT }, \
    { RSPPKGSIZE, "Response Packet Size xxx KB", SA_COUNT, SU_INT }, \
    { GET_HIT, "get hit", SA_COUNT, SU_INT }, \
	{ PKG_ENTER_COUNT, "enter pkg count", SA_COUNT, SU_INT }, \
	{ PKG_LEAVE_COUNT, "leave pkg count", SA_COUNT, SU_INT }, \
	{ FRONT_CONN_COUNT, "network - cur connect", SA_VALUE, SU_INT}
	
#endif
