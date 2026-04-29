#ifndef _YALEHDR_H_
#define _YALEHDR_H_

#include <string.h>

static inline uint32_t yale_htonl(uint32_t h)
{
  char buf[4];
  uint32_t n;
  buf[0] = h>>24;
  buf[1] = h>>16;
  buf[2] = h>>8;
  buf[3] = h>>0;
  memcpy(&n, buf, sizeof(n));
  return n;
}
static inline uint16_t yale_htons(uint16_t h)
{
  char buf[2];
  uint16_t n;
  buf[0] = h>>8;
  buf[1] = h>>0;
  memcpy(&n, buf, sizeof(n));
  return n;
}
static inline uint32_t yale_ntohl(uint32_t n)
{
  unsigned char buf[4];
  memcpy(buf, &n, sizeof(n));
  return (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | (buf[3]<<0);
}
static inline uint16_t yale_ntohs(uint16_t n)
{
  unsigned char buf[2];
  memcpy(buf, &n, sizeof(n));
  return (buf[0]<<8) | (buf[1]<<0);
}

static inline uint64_t yale_hdr_get64h(const void *buf)
{
  uint64_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline uint32_t yale_hdr_get32h(const void *buf)
{
  uint32_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline uint16_t yale_hdr_get16h(const void *buf)
{
  uint16_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline uint8_t yale_hdr_get8h(const void *buf)
{
  uint8_t res;
  memcpy(&res, buf, sizeof(res));
  return res;
}

static inline void yale_hdr_set64h(void *buf, uint64_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline void yale_hdr_set32h(void *buf, uint32_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline void yale_hdr_set16h(void *buf, uint16_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline void yale_hdr_set8h(void *buf, uint8_t val)
{
  memcpy(buf, &val, sizeof(val));
}

static inline uint32_t yale_hdr_get32n(const void *buf)
{
  return yale_ntohl(yale_hdr_get32h(buf));
}

static inline uint16_t yale_hdr_get16n(const void *buf)
{
  return yale_ntohs(yale_hdr_get16h(buf));
}

static inline void yale_hdr_set32n(void *buf, uint32_t val)
{
  yale_hdr_set32h(buf, yale_htonl(val));
}

static inline void yale_hdr_set16n(void *buf, uint16_t val)
{
  yale_hdr_set16h(buf, yale_htons(val));
}

#endif
