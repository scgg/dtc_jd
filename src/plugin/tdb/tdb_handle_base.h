#ifndef _TDB_HANDLE_BASE_H_
#define _TDB_HANDLE_BASE_H_

#include <string>

namespace tdb { namespace api {
 
class RspRecord
{
public:
  unsigned			retcode;
  std::string		retmsg;
  unsigned			stamp;
  std::string		value;
};

class RecordValue
{
public:
  std::string		key;
  std::string		value;
};

class RspSetRecord
{
public:
  unsigned			retcode;
  std::string		retmsg;
};

}}
#endif//_TDB_BASE_H_
