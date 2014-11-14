/*                               -*- Mode: C -*- 
 * Keywords: Node节点
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
 *  $Id: node.h 7428 2010-10-29 09:26:45Z foryzhou $
 */

/* Change log:
 * 
 * 
 * 
 */

/* Code: */
#ifndef __NODE_TTC_H
#define __NODE_TTC_H

#include <stdint.h>
#include "namespace.h"
#include "global.h"
#include "node_group.h"
#include "node_index.h"

TTC_BEGIN_NAMESPACE

class  CNGInfo;
class  CNodeIndex;

class CNode
{
    public:
	CNode(NODE_GROUP_T * ng = NULL, int idx = 0) : _owner(ng), _index(idx) {}
	CNode(const CNode&n) : _owner(n._owner), _index(n._index) {}
	~CNode() {}

    public:
	int Index(void) { return _index; }
	NODE_GROUP_T* Owner() { return _owner; }

	/* attribute op*/
	NODE_ID_T& LruPrev() {
		NODE_ID_T *p = NodeLru();
		return p[LRU_PREV];
	}

	NODE_ID_T& LruNext() {
		NODE_ID_T *p = NodeLru();
		return p[LRU_NEXT];
	}

	NODE_ID_T& NextNodeID() { return _owner->NextNodeID(_index); }
	NODE_ID_T  NodeID() { return _owner->NodeID(_index); }

	MEM_HANDLE_T& VDHandle() { return _owner->VDHandle(_index); }

	/* return time-marker time */
	unsigned int Time(){return (unsigned int)VDHandle();}

	/* dirty flag*/
	bool IsDirty() const { return _owner->IsDirty(_index); }
	void SetDirty() { return _owner->SetDirty(_index); }
	void ClrDirty() { return _owner->ClrDirty(_index); }


    public:
	/* used for timelist */
	CNode Next() { return FromID(LruNext()); }
	CNode Prev() { return FromID(LruPrev()); }

	/* used for hash */
	CNode NextNode (void) { return FromID(NextNodeID()); }

	/* for copyable */	
	CNode& operator= (const CNode&n) {
		_owner = n._owner;
		_index = n._index;
		return *this;
	}
	int operator!() const { return _owner == NULL || _index >= NODE_GROUP_INCLUDE_NODES;}
	int operator!=(CNode& node) {return _owner != node.Owner() || _index != node.Index();}
	int operator==(CNode& node) {return _owner == node.Owner() && _index == node.Index();}
        /*判断Node是否在lru链表*/
	int NotInLruList() { return LruPrev() == NodeID() || LruNext() == NodeID(); }
	static CNode Empty(void) { CNode node; return node; }

    private:
	/* init or delete this */
	int Reset()
	{
		NextNodeID() = INVALID_NODE_ID;
		LruPrev()    = NodeID();
		LruNext()    = NodeID();

		ClrDirty();
		return 0;
	}

	int Release()
	{
		_owner->ReleaseNode(*this);
		Reset();
		_owner = NULL;
		_index = 0;
		return 0;
	}

	static inline CNode FromID(NODE_ID_T id) { return I_SEARCH(id); }

    private: 
	// [0] = prev, [1] = next
	NODE_ID_T * NodeLru() { return _owner->NodeLru(_index); }

    private:
	NODE_GROUP_T *_owner;
	int 	      _index;

    public:
	/* friend class */
	friend class CNGInfo;
	friend class CNodeIndex;
	friend struct node_group;
};

TTC_END_NAMESPACE

#endif

/* ends here */
