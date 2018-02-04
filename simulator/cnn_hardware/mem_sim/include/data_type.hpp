#ifndef __DATA_TYPE__
#define __DATA_TYPE__

#include <string>
typedef unsigned long long uns64;
typedef uns64 Addr;

#include "Address.h"

struct ReqPacket
{
  bool isWrite;
  Addr addr;
  uint64_t nodeid;
  Addr byte_size;
};

struct MemValue 
{
  bool  isWrite;
  Addr addr;
};

struct MemValue_t
{
  bool isWrite;
  uint64_t nodeid;
  bool operator==(const MemValue_t& lhs)
  {
    return (lhs.isWrite == isWrite && lhs.nodeid == nodeid /* && lhs.nodeid == nodeid*/);
  }
};

#ifdef MY_DEBUG
#  define D(x) x
#else
#  define D(x)
#endif // DEBUG


#define ITER_COUNT  100
//ASSUMING PER ITERATION WILL NOT HAVE MORE THAN 500 OPS
#define ITER_COUNT_MEM  50000

#endif
