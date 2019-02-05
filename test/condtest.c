#include "condparsercparser.h"
#include "condparsercommon.h"

ssize_t setzero(const char *buf, size_t siz, int start, struct condparser_parserctx *btn)
{
  btn->condval = 0;
  return -EAGAIN;
}
ssize_t setone(const char *buf, size_t siz, int start, struct condparser_parserctx *btn)
{
  btn->condval = 1;
  return -EAGAIN;
}

int main(int argc, char **argv)
{
  ssize_t consumed;
  struct condparser_parserctx pctx = {};
  char msggood[] =
    "\x00\x00\x00\x00"
    "\x00\x01\x00\x01"
    "\x00\x00\x00\x00"
    "\x00\x01\x00\x01"
    "\x00\x00\x00\x00"
    "\x00\x01\x00\x01";
  char msgbad[] =
    "\x00\x00\x00\x00"
    "\x00\x01\x00\x00";

  condparser_parserctx_init(&pctx);
  consumed = condparser_parse_block(&pctx, msggood, sizeof(msggood)-1, 1);
  printf("Consumed: %zd\n", consumed);
  if (consumed != sizeof(msggood)-1 && consumed != -EAGAIN)
  {
    printf("Fail 1\n");
    abort();
  }

  condparser_parserctx_init(&pctx);
  consumed = condparser_parse_block(&pctx, msgbad, sizeof(msgbad)-1, 1);
  printf("Consumed: %zd\n", consumed);
  if (consumed == sizeof(msgbad)-1 || consumed == -EAGAIN)
  {
    printf("Fail 2\n");
    abort();
  }
  printf("OK\n");
  return 0;
}
