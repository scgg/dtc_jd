#ifndef __TTC_NODE_LIST_H
#define __TTC_NODE_LIST_H

#include "namespace.h"
#include "global.h"
#include "node.h"

TTC_BEGIN_NAMESPACE

#define INIT_NODE_LIST_HEAD(node, id) do {	\
    node.LruPrev() = id;			\
    node.LruNext() = id;			\
}while(0)


inline void __NODE_LIST_ADD( CNode p,
			     CNode prev,
			     CNode next)
{
    next.LruPrev() = p.NodeID();
    p.LruNext() = next.NodeID();
    p.LruPrev() = prev.NodeID();
    prev.LruNext() = p.NodeID();
}

inline void NODE_LIST_ADD( CNode p, CNode head)
{
    __NODE_LIST_ADD(p, head, head.Next());
}

inline void NODE_LIST_ADD_TAIL( CNode p, CNode head)
{
    __NODE_LIST_ADD(p, head.Prev(), head);
}

inline void __NODE_LIST_DEL(CNode prev, CNode next)
{
    next.LruPrev() = prev.NodeID();
    prev.LruNext() = next.NodeID();
}

inline void NODE_LIST_DEL(CNode p)
{
    __NODE_LIST_DEL(p.Prev(), p.Next());
    p.LruPrev() = p.NodeID();
    p.LruNext() = p.NodeID();
}

inline void NODE_LIST_MOVE(CNode p, CNode head)
{
    __NODE_LIST_DEL(p.Prev(), p.Next());
    NODE_LIST_ADD(p, head);
}

inline void NODE_LIST_MOVE_TAIL(CNode p, CNode head)
{
    __NODE_LIST_DEL(p.Prev(), p.Next());
    NODE_LIST_ADD_TAIL(p, head);
}

inline int NODE_LIST_EMPTY(CNode head)
{
   return head.LruNext() == head.NodeID();
}

/*正向遍历*/
#define NODE_LIST_FOR_EACH(pos, head) \
    for(pos = head.Next(); pos != head; pos = pos.Next())

/*反向遍历*/
#define NODE_LIST_FOR_EACH_RVS(pos, head) \
    for(pos = head.Prev(); pos != head; pos = pos.Prev())

TTC_END_NAMESPACE

#endif
