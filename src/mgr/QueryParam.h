/*
 * $Id$
 *
 * DEBUG: section 16    Cache Manager API
 *
 */

#ifndef SQUID_MGR_QUERY_PARAM_H
#define SQUID_MGR_QUERY_PARAM_H

#include "ipc/forward.h"
#include "RefCount.h"

namespace Mgr
{

class QueryParam: public RefCountable
{
public:
    typedef enum {ptInt = 1, ptString} Type;
    typedef RefCount<QueryParam> Pointer;

public:
    QueryParam(Type aType): type(aType) {}
    virtual ~QueryParam() {}
    virtual void pack(Ipc::TypedMsgHdr& msg) const = 0; ///< store parameter into msg
    virtual void unpackValue(const Ipc::TypedMsgHdr& msg) = 0; ///< load parameter value from msg

private:
    QueryParam(const QueryParam&); // not implemented
    QueryParam& operator= (const QueryParam&); // not implemented

public:
    Type type;
};

} // namespace Mgr

#endif /* SQUID_MGR_QUERY_PARAM_H */
