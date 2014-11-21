/************************************************************
  Description:    封装了T-tree的各种操作      
  Version:         TTC 3.0
  Function List:   
    1.  Attach()		attach一颗树
   2.  Insert()		插入一条记录
   3. Delete() 		删除一条记录
   4. Find()			查找一条记录
   5. TraverseForward()	从小到大遍历整棵树
   6. TraverseBackward()	从大到小遍历整棵树
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "t_tree.h"
#include "data_chunk.h"


#ifndef MODU_TEST
#include "tree_data.h"
#endif

int _CTtreeNode::Init()
{
	m_hLeft = INVALID_HANDLE;
	m_hRight = INVALID_HANDLE;
	m_chBalance = 0;
	m_ushNItems = 0;
	for(int i=0; i<PAGE_SIZE; i++)
		m_ahItems[i] = INVALID_HANDLE;
	return(0);
}

ALLOC_HANDLE_T _CTtreeNode::Alloc(CMallocator& stMalloc, ALLOC_HANDLE_T hRecord)
{
	ALLOC_HANDLE_T h;
	h = stMalloc.Malloc(sizeof(CTtreeNode));
	if(h == INVALID_HANDLE)
		return(INVALID_HANDLE);
	
	CTtreeNode* p = (CTtreeNode*)stMalloc.Handle2Ptr(h);	
	p->Init();
	p->m_ahItems[0] = hRecord;
	p->m_ushNItems = 1;
	
	return(h);
}

int _CTtreeNode::Insert(CMallocator& stMalloc, ALLOC_HANDLE_T& hNode, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T hRecord)
{
	CTtreeNode* pstNode;
	
	GET_OBJ(stMalloc, hNode, pstNode);
	uint16_t ushNodeCnt = pstNode->m_ushNItems;
	int iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, pstNode->m_ahItems[0]);
	
	if(iDiff == 0){
//		assert(0);
		return(-2);
	}
	
	if(iDiff <= 0){
		ALLOC_HANDLE_T hLeft = pstNode->m_hLeft;
		if((hLeft == INVALID_HANDLE || iDiff == 0) && pstNode->m_ushNItems < PAGE_SIZE){
			for(uint32_t i = ushNodeCnt; i > 0; i--)
				pstNode->m_ahItems[i] = pstNode->m_ahItems[i-1];
            pstNode->m_ahItems[0] = hRecord;
            pstNode->m_ushNItems++;
            return(0);
		}
		if(hLeft == INVALID_HANDLE){
			hLeft = Alloc(stMalloc, hRecord);
			if(hLeft == INVALID_HANDLE)
				return(-1);
			pstNode->m_hLeft = hLeft;
		}
		else{
			ALLOC_HANDLE_T hChild = hLeft;
			int iGrow = Insert(stMalloc, hChild, pchKey,  pCmpCookie, pfComp, hRecord);
			if(iGrow < 0)
				return iGrow;
			if(hChild != hLeft){
				hLeft = hChild;
				pstNode->m_hLeft = hChild;
			}
			if(iGrow == 0)
				return(0);
		}
		if(pstNode->m_chBalance > 0){
			pstNode->m_chBalance = 0;
			return(0);
		}
		else if(pstNode->m_chBalance == 0){
			pstNode->m_chBalance = -1;
			return(1);
		}
		else{
			CTtreeNode* pstLeft = (CTtreeNode*)stMalloc.Handle2Ptr(hLeft);
			if (pstLeft->m_chBalance < 0) { // single LL turn
                pstNode->m_hLeft = pstLeft->m_hRight;
                pstLeft->m_hRight = hNode;
                pstNode->m_chBalance = 0;
                pstLeft->m_chBalance = 0;
                hNode = hLeft;
            } else { // double LR turn
                ALLOC_HANDLE_T hRight = pstLeft->m_hRight;
                CTtreeNode* pstRight = (CTtreeNode*)stMalloc.Handle2Ptr(hRight);
                pstLeft->m_hRight = pstRight->m_hLeft;
                pstRight->m_hLeft = hLeft;
                pstNode->m_hLeft = pstRight->m_hRight;
                pstRight->m_hRight = hNode;
                pstNode->m_chBalance = (pstRight->m_chBalance < 0) ? 1 : 0;
                pstLeft->m_chBalance = (pstRight->m_chBalance > 0) ? -1 : 0;
                pstRight->m_chBalance = 0;
                hNode = hRight;
            }
            return(0);
		}
	}
	
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, pstNode->m_ahItems[ushNodeCnt-1]);
	if(iDiff == 0){
//		assert(0);
		return(-2);
	}
	if(iDiff >= 0){
		ALLOC_HANDLE_T hRight = pstNode->m_hRight;
		if((hRight == INVALID_HANDLE || iDiff == 0) && pstNode->m_ushNItems < PAGE_SIZE){
			pstNode->m_ahItems[ushNodeCnt] = hRecord;
            pstNode->m_ushNItems++;
            return(0);
		}
		if(hRight == INVALID_HANDLE){
			hRight = Alloc(stMalloc, hRecord);
			if(hRight == INVALID_HANDLE)
				return(-1);
			pstNode->m_hRight = hRight;
		}
		else{
			ALLOC_HANDLE_T hChild = hRight;
			int iGrow = Insert(stMalloc, hChild, pchKey,  pCmpCookie, pfComp, hRecord);
			if(iGrow < 0)
				return iGrow;
			if(hChild != hRight){
				hRight = hChild;
				pstNode->m_hRight = hChild;
			}
			if(iGrow == 0)
				return(0);
		}
		if(pstNode->m_chBalance < 0){
			pstNode->m_chBalance = 0;
			return(0);
		}
		else if(pstNode->m_chBalance == 0){
			pstNode->m_chBalance = 1;
			return(1);
		}
		else{
			CTtreeNode* pstRight = (CTtreeNode*)stMalloc.Handle2Ptr(hRight);
			if(pstRight->m_chBalance > 0){ // single RR turn
                pstNode->m_hRight = pstRight->m_hLeft;
                pstRight->m_hLeft = hNode;
                pstNode->m_chBalance = 0;
                pstRight->m_chBalance = 0;
                hNode = hRight;
            } else { // double RL turn
                ALLOC_HANDLE_T hLeft = pstRight->m_hLeft;
                CTtreeNode* pstLeft = (CTtreeNode*)stMalloc.Handle2Ptr(hLeft);
                pstRight->m_hLeft = pstLeft->m_hRight;
                pstLeft->m_hRight = hRight;
                pstNode->m_hRight = pstLeft->m_hLeft;
                pstLeft->m_hLeft = hNode;
                pstNode->m_chBalance = (pstLeft->m_chBalance > 0) ? -1 : 0;
                pstRight->m_chBalance = (pstLeft->m_chBalance < 0) ? 1 : 0;
                pstLeft->m_chBalance = 0;
                hNode = hLeft;
            }
            return(0);
		}
	}
	
	int iLeft = 1;
	int iRight = ushNodeCnt - 1;
	while(iLeft < iRight){
		int i = (iLeft + iRight) >> 1;
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, pstNode->m_ahItems[i]);
		if(iDiff == 0){
//			assert(0);
			return(-2);
		}
		if(iDiff > 0){
			iLeft = i + 1;
		}
		else{
			iRight = i;
			if(iDiff == 0)
				break;
		}
	}
	// Insert before item[r]
	if(pstNode->m_ushNItems < PAGE_SIZE){
		for (int i = ushNodeCnt; i > iRight; i--)
			pstNode->m_ahItems[i] = pstNode->m_ahItems[i-1];
        pstNode->m_ahItems[iRight] = hRecord;
         pstNode->m_ushNItems++;
        return(0);
	}
	else{
		CTtreeNode stBackup;
		memcpy(&stBackup, pstNode, sizeof(CTtreeNode));
        ALLOC_HANDLE_T hReInsert;
        if (pstNode->m_chBalance >= 0) { 
            hReInsert = pstNode->m_ahItems[0];
            for(int i = 1; i < iRight; i++)
				pstNode->m_ahItems[i-1] = pstNode->m_ahItems[i]; 
            pstNode->m_ahItems[iRight-1] = hRecord;
        } else { 
            hReInsert = pstNode->m_ahItems[ushNodeCnt-1];
            for (int i = ushNodeCnt-1; i > iRight; i--)
				pstNode->m_ahItems[i] = pstNode->m_ahItems[i-1]; 
            pstNode->m_ahItems[iRight] = hRecord;
        }
		const char* pchTmpKey = ((CDataChunk*)stMalloc.Handle2Ptr(hReInsert))->Key();
		int iRet = Insert(stMalloc, hNode, pchTmpKey,  pCmpCookie, pfComp, hReInsert);
		if(iRet < 0){
			memcpy(pstNode->m_ahItems, stBackup.m_ahItems, sizeof(pstNode->m_ahItems));
		}
		return(iRet);
	}
}

int _CTtreeNode::Delete(CMallocator& stMalloc, ALLOC_HANDLE_T& hNode, const char* pchKey, void* pCmpCookie, KeyComparator pfComp)
{
	CTtreeNode* pstNode;
	ALLOC_HANDLE_T hTmp;
	
	GET_OBJ(stMalloc, hNode, pstNode);
	uint16_t ushNodeCnt = pstNode->m_ushNItems;
	int iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, pstNode->m_ahItems[0]);
	
	if(iDiff < 0){
		ALLOC_HANDLE_T hLeft = pstNode->m_hLeft;
		if(hLeft != INVALID_HANDLE){
			ALLOC_HANDLE_T hChild = hLeft;
			int iRet = Delete(stMalloc, hChild, pchKey,  pCmpCookie, pfComp);
			if(iRet < -1)
				return(iRet);
			if(hChild != hLeft){
				pstNode->m_hLeft = hChild;
			}			
			if(iRet > 0){
				return BalanceLeftBranch(stMalloc, hNode);
			}
			else if(iRet == 0){
				return(0);
			}
		}
//		assert(iDiff == 0);
	}
	
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, pstNode->m_ahItems[ushNodeCnt-1]);
	if(iDiff <= 0){
		for(int i=0; i<ushNodeCnt; i++){
			if(pfComp(pchKey,  pCmpCookie, stMalloc, pstNode->m_ahItems[i]) == 0){
				if(ushNodeCnt == 1){
					if(pstNode->m_hRight == INVALID_HANDLE){
						hTmp = pstNode->m_hLeft;
						stMalloc.Free(hNode);
						hNode = hTmp;
						return(1);
					}
					else if(pstNode->m_hLeft == INVALID_HANDLE){
						hTmp = pstNode->m_hRight;
						stMalloc.Free(hNode);
						hNode = hTmp;
						return(1);
					}
				}
				ALLOC_HANDLE_T hLeft = pstNode->m_hLeft;
				ALLOC_HANDLE_T hRight = pstNode->m_hRight;
				if(ushNodeCnt <= MIN_ITEMS){
					if(hLeft != INVALID_HANDLE && pstNode->m_chBalance <= 0){
						CTtreeNode* pstLeft;
						GET_OBJ(stMalloc, hLeft, pstLeft);
						while(pstLeft->m_hRight != INVALID_HANDLE){
							GET_OBJ(stMalloc, pstLeft->m_hRight, pstLeft);
						}
						while (--i >= 0) { 
                            pstNode->m_ahItems[i+1] = pstNode->m_ahItems[i];
                        }
						pstNode->m_ahItems[0] = pstLeft->m_ahItems[pstLeft->m_ushNItems-1];
						const char* pchTmpKey = ((CDataChunk*)stMalloc.Handle2Ptr(pstNode->m_ahItems[0]))->Key();
						ALLOC_HANDLE_T hChild = hLeft;
						int iRet = Delete(stMalloc, hChild, pchTmpKey,  pCmpCookie, pfComp);
						if(iRet < -1){
							return(iRet);
						}
						if(hChild != hLeft){
							pstNode->m_hLeft = hChild;
						}
						if(iRet > 0){
							iRet = BalanceLeftBranch(stMalloc, hNode);
						}
						return(iRet);
					}
					else if(pstNode->m_hRight != INVALID_HANDLE){
						CTtreeNode* pstRight;
						GET_OBJ(stMalloc, hRight, pstRight);
						while(pstRight->m_hLeft != INVALID_HANDLE){
							GET_OBJ(stMalloc, pstRight->m_hLeft, pstRight);
						}
						while(++i < ushNodeCnt){
							 pstNode->m_ahItems[i-1] = pstNode->m_ahItems[i];
						}
						pstNode->m_ahItems[ushNodeCnt-1] = pstRight->m_ahItems[0];
						const char* pchTmpKey = ((CDataChunk*)stMalloc.Handle2Ptr(pstNode->m_ahItems[ushNodeCnt-1]))->Key();
						ALLOC_HANDLE_T hChild = hRight;
						int iRet = Delete(stMalloc, hChild, pchTmpKey,  pCmpCookie, pfComp);
						if(iRet < -1){
							return(iRet);
						}
						if(hChild != hRight){
							pstNode->m_hRight = hChild;
						}
						if(iRet > 0){
							iRet = BalanceRightBranch(stMalloc, hNode);
						}
						return(iRet);
					}
				}
				
				while (++i < ushNodeCnt) { 
                    pstNode->m_ahItems[i-1] = pstNode->m_ahItems[i];
                }
                pstNode->m_ushNItems--;
				
                return(0);
			}
		}
	}
	
	ALLOC_HANDLE_T hRight = pstNode->m_hRight;
    if (hRight != 0) { 
        ALLOC_HANDLE_T hChild = hRight;
		int iRet = Delete(stMalloc, hChild, pchKey,  pCmpCookie, pfComp);
		if(iRet < -1){
			return(iRet);
		}
        if (hChild != hRight) { 
            pstNode->m_hRight = hChild;
        }
        if (iRet > 0) { 
            return BalanceRightBranch(stMalloc, hNode);
        } else { 
            return iRet;
        }
    }
	
    return -1;
}

inline int _CTtreeNode::BalanceLeftBranch(CMallocator& stMalloc, ALLOC_HANDLE_T& hNode)
{
    CTtreeNode* pstNode;
	GET_OBJ(stMalloc, hNode, pstNode);
	
    if (pstNode->m_chBalance < 0) { 
        pstNode->m_chBalance = 0;
        return(1);
    } 
	else if (pstNode->m_chBalance == 0) { 
        pstNode->m_chBalance = 1;
        return(0);
    }
	else{ 
        ALLOC_HANDLE_T hRight = pstNode->m_hRight;
        CTtreeNode* pstRight;
		GET_OBJ(stMalloc, hRight, pstRight);
		
        if (pstRight->m_chBalance >= 0) { // single RR turn
            pstNode->m_hRight = pstRight->m_hLeft;
            pstRight->m_hLeft = hNode;
            if (pstRight->m_chBalance == 0) { 
                pstNode->m_chBalance = 1;
                pstRight->m_chBalance = -1;
                hNode = hRight;
                return 0;
            } else { 
                pstNode->m_chBalance = 0;
                pstRight->m_chBalance = 0;
                hNode = hRight;
                return 1;
            }
        } else { // double RL turn
            ALLOC_HANDLE_T hLeft = pstRight->m_hLeft;
            CTtreeNode* pstLeft;
			GET_OBJ(stMalloc, hLeft, pstLeft);
            pstRight->m_hLeft = pstLeft->m_hRight;
            pstLeft->m_hRight = hRight;
            pstNode->m_hRight = pstLeft->m_hLeft;
            pstLeft->m_hLeft = hNode;
            pstNode->m_chBalance = pstLeft->m_chBalance > 0 ? -1 : 0;
            pstRight->m_chBalance = pstLeft->m_chBalance < 0 ? 1 : 0;
            pstLeft->m_chBalance = 0;
            hNode = hLeft;
            return 1;
        }
    }
}

inline int _CTtreeNode::BalanceRightBranch(CMallocator& stMalloc, ALLOC_HANDLE_T& hNode)
{
    CTtreeNode* pstNode;
	GET_OBJ(stMalloc, hNode, pstNode);
	
    if (pstNode->m_chBalance > 0) { 
        pstNode->m_chBalance = 0;
        return(1);
    } 
	else if (pstNode->m_chBalance == 0) { 
        pstNode->m_chBalance = -1;
        return(0);
    }
	else{ 
		ALLOC_HANDLE_T hLeft = pstNode->m_hLeft;
        CTtreeNode* pstLeft;
		GET_OBJ(stMalloc, hLeft, pstLeft);
		if (pstLeft->m_chBalance <= 0) { // single LL turn
			pstNode->m_hLeft = pstLeft->m_hRight;
			pstLeft->m_hRight = hNode;
			if (pstLeft->m_chBalance == 0) { 
				pstNode->m_chBalance = -1;
				pstLeft->m_chBalance = 1;
				hNode = hLeft;
				return(0);
			}
			else { 
				pstNode->m_chBalance = 0;
				pstLeft->m_chBalance = 0;
				hNode = hLeft;
				return(1);
			}
		}
		else { // double LR turn
			ALLOC_HANDLE_T hRight = pstLeft->m_hRight;
			CTtreeNode* pstRight;
			GET_OBJ(stMalloc, hRight, pstRight);
			
			pstLeft->m_hRight = pstRight->m_hLeft;
			pstRight->m_hLeft = hLeft;
			pstNode->m_hLeft = pstRight->m_hRight;
			pstRight->m_hRight = hNode;
			pstNode->m_chBalance = pstRight->m_chBalance < 0 ? 1 : 0;
			pstLeft->m_chBalance = pstRight->m_chBalance > 0 ? -1 : 0;
			pstRight->m_chBalance = 0;
			hNode = hRight;
			return(1);
		}
	}
}

unsigned _CTtreeNode::AskForDestroySize(CMallocator& stMalloc, ALLOC_HANDLE_T hNode)
{
	unsigned size = 0;

	if(INVALID_HANDLE == hNode)
		return size;

	CTtreeNode* pstNode;
	GET_OBJ(stMalloc, hNode, pstNode);
	ALLOC_HANDLE_T hLeft = pstNode->m_hLeft;
	ALLOC_HANDLE_T hRight = pstNode->m_hRight;

	for(int i=0; i<pstNode->m_ushNItems; i++)
		size += ((CDataChunk*)(stMalloc.Handle2Ptr(pstNode->m_ahItems[i])))->AskForDestroySize(&stMalloc);

	size += stMalloc.AskForDestroySize(hNode);

	size += AskForDestroySize(stMalloc, hLeft);
	size += AskForDestroySize(stMalloc, hRight);

	return size;
}

int _CTtreeNode::Destroy(CMallocator& stMalloc, ALLOC_HANDLE_T hNode)
{
	if(hNode != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(stMalloc, hNode, pstNode);
		ALLOC_HANDLE_T hLeft = pstNode->m_hLeft;
		ALLOC_HANDLE_T hRight = pstNode->m_hRight;
		for(int i=0; i<pstNode->m_ushNItems; i++)
			//stMalloc.Free(pstNode->m_ahItems[i]);
			((CDataChunk*)(stMalloc.Handle2Ptr(pstNode->m_ahItems[i])))->Destroy(&stMalloc);
		stMalloc.Free(hNode);

		Destroy(stMalloc, hLeft);
		Destroy(stMalloc, hRight);
	}
	return(0);
}

int _CTtreeNode::Find(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T*& phRecord)
{
	int iDiff;
	
	phRecord = NULL;
	if(m_ushNItems == 0)
		return(0);
	
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
	if(iDiff == 0){
		phRecord = &(m_ahItems[0]);
		return(1);
	}
	else if(iDiff > 0){
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
		if(iDiff == 0){
			phRecord = &(m_ahItems[m_ushNItems-1]);
			return(1);
		}
		else if(iDiff > 0){
			if(m_hRight == INVALID_HANDLE){
				return(0);
			}
			CTtreeNode* pstNode;
			GET_OBJ(stMalloc, m_hRight, pstNode);
			return pstNode->Find(stMalloc, pchKey,  pCmpCookie, pfComp, phRecord);
		}
		
		int iLeft = 1;
		int iRight = m_ushNItems - 1;
		while(iLeft < iRight){
			int i = (iLeft + iRight) >> 1;
			iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[i]);
			if(iDiff == 0){
				phRecord = &(m_ahItems[i]);
				return(1);
			}
			if(iDiff > 0){
				iLeft = i + 1;
			}
			else{
				iRight = i;
			}
		}
		return(0);
	}
	else{
		if(m_hLeft == INVALID_HANDLE){
			return(0);
		}
		CTtreeNode* pstNode;
		GET_OBJ(stMalloc, m_hLeft, pstNode);
		return pstNode->Find(stMalloc, pchKey,  pCmpCookie, pfComp, phRecord);
	}
	
}

int _CTtreeNode::Find(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T& hRecord)
{
	int iRet;
	ALLOC_HANDLE_T* phItem;
	
	hRecord = INVALID_HANDLE;
	iRet = Find(stMalloc, pchKey, pCmpCookie, pfComp, phItem);
	if(iRet == 1 && phItem != NULL){
		hRecord = *phItem;
	}
	
	return(iRet);
}

int _CTtreeNode::FindHandle(CMallocator& stMalloc, ALLOC_HANDLE_T hRecord)
{
	
	if(m_ushNItems == 0)
		return(0);
	
	for(int i=0; i<m_ushNItems; i++)
		if(m_ahItems[i] == hRecord)
			return(1);

	CTtreeNode* pstNode;
	if(m_hRight != INVALID_HANDLE){
		GET_OBJ(stMalloc, m_hRight, pstNode);
		if(pstNode->FindHandle(stMalloc, hRecord) == 1)
			return(1);
	}
	
	if(m_hLeft != INVALID_HANDLE){
		GET_OBJ(stMalloc, m_hLeft, pstNode);
		if(pstNode->FindHandle(stMalloc, hRecord) == 1)
			return(1);
	}
	
	return(0);
	
}

int _CTtreeNode::FindNode(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T& hNode)
{
	int iDiff;
	
	hNode = INVALID_HANDLE;
	if(m_ushNItems == 0)
		return(0);
	
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
	if(iDiff == 0){
		hNode = stMalloc.Ptr2Handle(this);
		return(1);
	}
	else if(iDiff > 0){
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
		if(iDiff <= 0){
			hNode = stMalloc.Ptr2Handle(this);
			return(1);
		}
		else if(iDiff > 0){
			if(m_hRight == INVALID_HANDLE){
				return(0);
			}
			CTtreeNode* pstNode;
			GET_OBJ(stMalloc, m_hRight, pstNode);
			return pstNode->FindNode(stMalloc, pchKey,  pCmpCookie, pfComp, hNode);
		}
	}
	else{
		if(m_hLeft == INVALID_HANDLE){
			hNode = stMalloc.Ptr2Handle(this);
			return(1);
		}
		CTtreeNode* pstNode;
		GET_OBJ(stMalloc, m_hLeft, pstNode);
		return pstNode->FindNode(stMalloc, pchKey,  pCmpCookie, pfComp, hNode);
	}

	return(0);
}

int _CTtreeNode::TraverseForward(CMallocator& stMalloc, ItemVisit pfVisit, void* pCookie)
{
	int iRet;
	
	if(m_hLeft != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->TraverseForward(stMalloc, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}

	for(int i=0; i<m_ushNItems; i++){
		if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
			return(iRet);
		}
	}

	if(m_hRight != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->TraverseForward(stMalloc, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	return(0);
}

int _CTtreeNode::TraverseBackward(CMallocator& stMalloc, ItemVisit pfVisit, void* pCookie)
{
	int iRet;
	
	if(m_hRight != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->TraverseBackward(stMalloc, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	for(int i=m_ushNItems; --i >= 0; ){
		if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
			return(iRet);
		}
	}
	if(m_hLeft != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->TraverseBackward(stMalloc, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	return(0);
}

int _CTtreeNode::PostOrderTraverse(CMallocator& stMalloc, ItemVisit pfVisit, void* pCookie)
{
	int iRet;
	
	if(m_hRight != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->PostOrderTraverse(stMalloc, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	if(m_hLeft != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->PostOrderTraverse(stMalloc, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	for(int i=m_ushNItems; --i >= 0; ){
		if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
			return(iRet);
		}
	}
	
	return(0);
}

int _CTtreeNode::TraverseForward(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, int iInclusion, ItemVisit pfVisit, void* pCookie)
{
	int iDiff;
	int iRet;
	
	if(m_hLeft != INVALID_HANDLE){	
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
		if(iDiff < 0){
			if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->TraverseForward(stMalloc, pchKey,  pCmpCookie, pfComp, iInclusion, pfVisit, pCookie)) != 0){
				return(iRet);
			}
		}
	}

	int i=m_ushNItems;
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
	if(iDiff <= 0){
		for(i=0; i<m_ushNItems; i++){
			iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[i]);
			if(iDiff<=0 && iDiff>=0-iInclusion){
				if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
					return(iRet);
				}
			}
			else if(iDiff < 0-iInclusion){
				break;
			}
		}
	}
	
	if(i >= m_ushNItems && m_hRight != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->TraverseForward(stMalloc, pchKey,  pCmpCookie, pfComp, iInclusion, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	return(0);	
}


int _CTtreeNode::TraverseForward(CMallocator& stMalloc, const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	int iDiff;
	int iDiff1;
	int iRet;
	
	if(m_hLeft != INVALID_HANDLE){	
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
		if(iDiff < 0){
			if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->TraverseForward(stMalloc, pchKey, pchKey1, pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
				return(iRet);
			}
		}
	}

	int i;
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
	iDiff1 = pfComp(pchKey1,  pCmpCookie, stMalloc, m_ahItems[0]);
	if(iDiff1 < 0 || iDiff > 0){ // key1 < item[0]   OR   key > item[n]
		
	}
	else{
		for(i=0; i<m_ushNItems; i++){
			iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[i]);
			if(iDiff<=0){
				iDiff1 = pfComp(pchKey1,  pCmpCookie, stMalloc, m_ahItems[i]);
				if(iDiff1 >= 0){
					if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
						return(iRet);
					}
				}
			}
		}
	}
	
	iDiff1 = pfComp(pchKey1,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
	if(iDiff1 >= 0 && m_hRight != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->TraverseForward(stMalloc, pchKey, pchKey1, pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	return(0);	
}

int _CTtreeNode::TraverseForward(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	int iDiff;
	int iRet;
	
	if(m_hLeft != INVALID_HANDLE){	
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
		if(iDiff < 0){
			if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->TraverseForward(stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
				return(iRet);
			}
		}
	}

	int i;
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
	if(iDiff <= 0){
		for(i=0; i<m_ushNItems; i++){
			iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[i]);
			if(iDiff<=0){
				if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
					return(iRet);
				}
			}
		}
	}
	
	if(m_hRight != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->TraverseForward(stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	return(0);	
}

int _CTtreeNode::TraverseBackward(CMallocator& stMalloc, const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	int iDiff;
	int iDiff1;
	int iRet;
	int i;
	
	if(m_hRight != INVALID_HANDLE){
		iDiff1 = pfComp(pchKey1,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
		if(iDiff1 > 0){
			if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->TraverseBackward(stMalloc, pchKey, pchKey1, pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
				return(iRet);
			}
		}
	}
	
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
	iDiff1 = pfComp(pchKey1,  pCmpCookie, stMalloc, m_ahItems[0]);
	if(iDiff1 < 0 || iDiff > 0){ // key1 < item[0]   OR   key > item[n]
		
	}
	else{
		for(i=m_ushNItems; --i >= 0; ){
			iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[i]);
			if(iDiff <= 0){
				iDiff1 = pfComp(pchKey1,  pCmpCookie, stMalloc, m_ahItems[i]);
				if(iDiff1 >= 0){
					if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
						return(iRet);
					}
				}
			}
		}
	}
	
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
	if(iDiff <= 0 && m_hLeft != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->TraverseBackward(stMalloc, pchKey, pchKey1, pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	return(0);
}

int _CTtreeNode::TraverseBackward(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	int iDiff;
	int iRet;
	
	if(m_hRight != INVALID_HANDLE){
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
		if(iDiff > 0){
			if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->TraverseBackward(stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
				return(iRet);
			}
		}
	}
	
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
	if(iDiff >= 0){
		for(int i=m_ushNItems; --i >= 0; ){
			iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[i]);
			if(iDiff >= 0){
				if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
					return(iRet);
				}
			}
		}
	}
	
	if(m_hLeft != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->TraverseBackward(stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	return(0);
}

int _CTtreeNode::PostOrderTraverse(CMallocator& stMalloc, const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	int iDiff;
	int iDiff1;
	int iRet;
	
	if(m_hLeft != INVALID_HANDLE){	
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
		if(iDiff < 0){
			if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->PostOrderTraverse(stMalloc, pchKey, pchKey1, pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
				return(iRet);
			}
		}
	}

	iDiff1 = pfComp(pchKey1,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
	if(iDiff1 >= 0 && m_hRight != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->PostOrderTraverse(stMalloc, pchKey, pchKey1, pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	int i;
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
	iDiff1 = pfComp(pchKey1,  pCmpCookie, stMalloc, m_ahItems[0]);
	if(iDiff1 < 0 || iDiff > 0){ // key1 < item[0]   OR   key > item[n]
		
	}
	else{
		for(i=0; i<m_ushNItems; i++){
			iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[i]);
			if(iDiff<=0){
				iDiff1 = pfComp(pchKey1,  pCmpCookie, stMalloc, m_ahItems[i]);
				if(iDiff1 >= 0){
					if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
						return(iRet);
					}
				}
			}
		}
	}
	
	return(0);	
}

int _CTtreeNode::PostOrderTraverseGE(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	int iDiff;
	int iRet;
	
	if(m_hLeft != INVALID_HANDLE){	
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
		if(iDiff < 0){
			if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->PostOrderTraverseGE(stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
				return(iRet);
			}
		}
	}

	if(m_hRight != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->PostOrderTraverseGE(stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	int i;
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
	if(iDiff <= 0){
		for(i=0; i<m_ushNItems; i++){
			iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[i]);
			if(iDiff<=0){
				if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
					return(iRet);
				}
			}
		}
	}
	
	return(0);	
}

int _CTtreeNode::PostOrderTraverseLE(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	int iDiff;
	int iRet;
	
	if(m_hRight != INVALID_HANDLE){
		iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[m_ushNItems-1]);
		if(iDiff > 0){
			if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hRight))->PostOrderTraverseLE(stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
				return(iRet);
			}
		}
	}
	
	if(m_hLeft != INVALID_HANDLE){
		if((iRet = ((CTtreeNode*)stMalloc.Handle2Ptr(m_hLeft))->PostOrderTraverseLE(stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie)) != 0){
			return(iRet);
		}
	}
	
	iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[0]);
	if(iDiff >= 0){
		for(int i=m_ushNItems; --i >= 0; ){
			iDiff = pfComp(pchKey,  pCmpCookie, stMalloc, m_ahItems[i]);
			if(iDiff >= 0){
				if((iRet = pfVisit(stMalloc, m_ahItems[i], pCookie)) != 0){
					return(iRet);
				}
			}
		}
	}
	
	return(0);
}


CTtree::CTtree(CMallocator& stMalloc): m_stMalloc(stMalloc)
{
	m_hRoot = INVALID_HANDLE;
	m_szErr[0] = 0;
}

CTtree::~CTtree()
{

}

int CTtree::Insert(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T hRecord)
{
	ALLOC_HANDLE_T hNode;
	
	if(m_hRoot == INVALID_HANDLE){
		hNode = CTtreeNode::Alloc(m_stMalloc, hRecord);
		if(hNode == INVALID_HANDLE){
			snprintf(m_szErr, sizeof(m_szErr), "alloc tree-node error: %s", m_stMalloc.GetErrMsg());
			return(EC_NO_MEM);
		}
		m_hRoot = hNode;
	}
	else{
		hNode = m_hRoot;
		int iRet = CTtreeNode::Insert(m_stMalloc, hNode, pchKey,  pCmpCookie, pfComp, hRecord);
		if(iRet == -2){
			snprintf(m_szErr, sizeof(m_szErr), "key already exists.");
			return(EC_KEY_EXIST);
		}
		else if(iRet == -1){
			snprintf(m_szErr, sizeof(m_szErr), "alloc tree-node error: %s", m_stMalloc.GetErrMsg());
			return(EC_NO_MEM);
		}
		else if(iRet < 0){
			snprintf(m_szErr, sizeof(m_szErr), "insert error");
			return(-1);
		}
		if(hNode != m_hRoot){
			m_hRoot = hNode;
		}
	}
	
	return(0);
}

int CTtree::Delete(const char* pchKey, void* pCmpCookie, KeyComparator pfComp)
{
	if(m_hRoot == INVALID_HANDLE){
		return(0);
	}
	
	ALLOC_HANDLE_T hNode = m_hRoot;
	int iRet = CTtreeNode::Delete(m_stMalloc, hNode, pchKey,  pCmpCookie, pfComp);
	if(iRet < -1){
		snprintf(m_szErr, sizeof(m_szErr), "internal error");
		return(-1);
	}
	else if(iRet == -1){
		snprintf(m_szErr, sizeof(m_szErr), "tree error");
		return(-1);
	}
	if(hNode != m_hRoot)
		m_hRoot = hNode;
	
	return(0);
}

int CTtree::FindHandle(ALLOC_HANDLE_T hRecord)
{
	if(m_hRoot == INVALID_HANDLE){
		return(0);
	}
	
	CTtreeNode* pstNode;
	GET_OBJ(m_stMalloc, m_hRoot, pstNode);
	return pstNode->FindHandle(m_stMalloc, hRecord);
}

int CTtree::Find(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T& hRecord)
{
	hRecord = INVALID_HANDLE;
	if(m_hRoot == INVALID_HANDLE){
		return(0);
	}
	
	CTtreeNode* pstNode;
	GET_OBJ(m_stMalloc, m_hRoot, pstNode);
	return pstNode->Find(m_stMalloc, pchKey,  pCmpCookie, pfComp, hRecord);
}

int CTtree::Find(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T*& phRecord)
{
	phRecord = NULL;
	if(m_hRoot == INVALID_HANDLE){
		return(0);
	}
	
	CTtreeNode* pstNode;
	GET_OBJ(m_stMalloc, m_hRoot, pstNode);
	return pstNode->Find(m_stMalloc, pchKey,  pCmpCookie, pfComp, phRecord);
}

int CTtree::Destroy()
{
	CTtreeNode::Destroy(m_stMalloc, m_hRoot);
	m_hRoot = INVALID_HANDLE;
	return(0);
}

unsigned CTtree::AskForDestroySize(void)
{
	return CTtreeNode::AskForDestroySize(m_stMalloc, m_hRoot);
}

int CTtree::TraverseForward(ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		return pstNode->TraverseForward(m_stMalloc, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::TraverseBackward(ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		return pstNode->TraverseBackward(m_stMalloc, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::PostOrderTraverse(ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		return pstNode->PostOrderTraverse(m_stMalloc, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::TraverseForward(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, int64_t iInclusion, ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		
		return pstNode->TraverseForward(m_stMalloc, pchKey,  pCmpCookie, pfComp, iInclusion, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::TraverseForward(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		
		return pstNode->TraverseForward(m_stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::TraverseForward(const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		
		return pstNode->TraverseForward(m_stMalloc, pchKey, pchKey1, pCmpCookie, pfComp, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::TraverseBackward(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		
		return pstNode->TraverseBackward(m_stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::TraverseBackward(const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		
		return pstNode->TraverseBackward(m_stMalloc, pchKey, pchKey1, pCmpCookie, pfComp, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::PostOrderTraverse(const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		
		return pstNode->PostOrderTraverse(m_stMalloc, pchKey, pchKey1, pCmpCookie, pfComp, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::PostOrderTraverseGE(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		
		return pstNode->PostOrderTraverseGE(m_stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie);
	}
	
	return(0);
}

int CTtree::PostOrderTraverseLE(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie)
{
	if(m_hRoot != INVALID_HANDLE){
		CTtreeNode* pstNode;
		GET_OBJ(m_stMalloc, m_hRoot, pstNode);
		
		return pstNode->PostOrderTraverseLE(m_stMalloc, pchKey,  pCmpCookie, pfComp, pfVisit, pCookie);
	}
	
	return(0);
}

