#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "yalecommon.h"

#include "httpparser2.h"

struct http_parserctx {
  uint8_t stacksz;
  struct ruleentry stack[8]; // WAS: uint8_t stack[...];
  struct http_rectx rctx;
  uint8_t saved_token;
  
  size_t bytesCounter;

};

#include "httpparser2.c"

int main(int argc, char **argv)
{
  struct http_rectx rectx = {};
  char buf[] = "GET / HTTP/1.1\r\n";
  size_t sz = sizeof(buf)-1;
  uint8_t state;
  ssize_t ret = 0;
  http_init_statemachine(&rectx);
  ret += http_feed_statemachine(&rectx, http_states_5, buf+ret, sz-ret, &state, NULL, NULL, 255);
  if (ret != 3 || state != 5)
  {
    abort();
  }
  printf("Processed %zd of %zu tk %d\n", ret, sz, (int)state);
  ret += http_feed_statemachine(&rectx, http_states_2, buf+ret, sz-ret, &state, NULL, NULL, 255);
  if (ret != 4 || state != 2)
  {
    abort();
  }
  printf("Processed %zd of %zu tk %d\n", ret, sz, (int)state);
  ret += http_feed_statemachine(&rectx, http_states_7, buf+ret, sz-ret, &state, NULL, NULL, 255);
  if (ret != 5 || state != 7)
  {
    abort();
  }
  printf("Processed %zd of %zu tk %d\n", ret, sz, (int)state);
  ret += http_feed_statemachine(&rectx, http_states_3, buf+ret, sz-ret, &state, NULL, NULL, 255);
  if (ret != 16 || state != 3)
  {
    abort();
  }
  printf("Processed %zd of %zu tk %d\n", ret, sz, (int)state);
  return 0;
}
