#ifndef _YALECONTAINEROF_H_
#define _YALECONTAINEROF_H_

#define YALE_CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - (((char*)&(((type*)0)->member)) - ((char*)0))))

#endif
