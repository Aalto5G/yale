#include "tokentheft1cparser.h"
#include <sys/time.h>
#include <errno.h>

int main(int argc, char **argv)
{
  ssize_t consumed;
  struct tokentheft1_parserctx pctx = {};
  char tokentheft1[] = "content-type";

  tokentheft1_parserctx_init(&pctx);
  pctx.condval = 1;

  consumed = tokentheft1_parse_block(&pctx, tokentheft1, sizeof(tokentheft1)-1, 1);
  if (consumed == 12)
  {
    printf("SCCLL(1)\n");
    abort();
  }

  if (consumed == -EINVAL)
  {
    printf("CCLL(1)\n");
    return 0;
  }

  abort();
}
