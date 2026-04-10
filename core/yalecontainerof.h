#ifndef _YALECONTAINEROF_H_
#define _YALECONTAINEROF_H_

#define YALE_CONTAINER_OF(ptr, type, member) \
  ((type*)(((char*)ptr) - offsetof(type, member)))


#endif
