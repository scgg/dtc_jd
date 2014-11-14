/*                               -*- Mode: C -*- 
 * Filename: 
 * 
 * Keywords:  NodeID��CNode���໥ת��
 * 
 *       This is unpublished proprietary
 *       source code of Tencent Ltd.
 *       The copyright notice above does not
 *       evidence any actual or intended
 *       publication of such source code.
 *
 *       NOTICE: UNAUTHORIZED DISTRIBUTION,ADAPTATION OR 
 *       USE MAY BE SUBJECT TO CIVIL AND CRIMINAL PENALTIES.
 *
 *  $Id: node_index.h 7428 2010-10-29 09:26:45Z foryzhou $
 */

/*
 *    NodeID     -----   CNode
 *  	  8bits 1-index
 *  	 16bits 2-index
 *   	 8bits NodeGroup�ڲ�����
 *
 */

/* Change log:
 * 
 * 
 * 
 */
/* Code: */
#ifndef __TTC_NODE_INDEX_H
#define __TTC_NODE_INDEX_H

#include "namespace.h"
#include "global.h"

TTC_BEGIN_NAMESPACE

#define INDEX_1_SIZE (((1UL << 8)*sizeof(MEM_HANDLE_T))+sizeof(FIRST_INDEX_T)) 	// first-index size
#define INDEX_2_SIZE (((1UL << 16)*sizeof(MEM_HANDLE_T))+sizeof(SECOND_INDEX_T))// second-index size

#define OFFSET1(id) ((id) >> 24)				//��8λ��һ��index
#define OFFSET2(id) (((id) & 0xFFFF00) >> 8)			//�м�16λ������index
#define OFFSET3(id) ((id) & 0xFF)				//��8λ

struct first_index
{
    uint32_t fi_used;		//һ��indexʹ�ø���
    MEM_HANDLE_T fi_h[0]; 	//��Ŷ���index��handle
};
typedef struct first_index FIRST_INDEX_T;

struct second_index
{
    uint32_t si_used;
    MEM_HANDLE_T si_h[0];
};
typedef struct second_index SECOND_INDEX_T;

class CNode;
class CNodeIndex
{
    public:
	CNodeIndex();
	~CNodeIndex();

	static CNodeIndex* Instance();
	static void Destroy();

	int Insert(CNode);
	CNode Search(NODE_ID_T id);

	int PreAllocateIndex2(size_t size);

	const MEM_HANDLE_T Handle() const {return M_HANDLE(_firstIndex);}
	const char *Error() const {return _errmsg;}
	///* �ڴ������������ */
	int Init(size_t mem_size);
	int Attach(MEM_HANDLE_T handle);
	int Detach(void);

    private:
	FIRST_INDEX_T *_firstIndex;
	char 	       _errmsg[256];
};

TTC_END_NAMESPACE

#endif
/* ends here */
