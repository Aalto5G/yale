#include "yale.h"
#include "yyutils.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  FILE *f;
  const struct yale yaleorig = {};
  struct yale yale = {};
  struct yale yalepaper = {};

#if 0
  f = fopen("http.txt", "r");
  printf("Parsing HTTP...\n");
  yaleyydoparse(f, &yale);
  fclose(f);
  printf("HTTP parsed\n");
  yale_free(&yale);
  yale = yaleorig;
#endif

  f = fopen("reallysimple.txt", "r");
  printf("Parsing really simple...\n");
  yaleyydoparse(f, &yale);
  fclose(f);
  printf("really simple parsed\n");
  if (check_actions(&yale) != 0)
  {
    printf("Fail action\n");
    exit(1);
  }
  yale_free(&yale);
  yale = yaleorig;

#if 0
  f = fopen("httpsimple.txt", "r");
  printf("Parsing HTTP simple...\n");
  yaleyydoparse(f, &yale);
  fclose(f);
  printf("HTTP simple parsed\n");
  yale_free(&yale);
  yale = yaleorig;
#endif

  f = fopen("httpnorepeat.txt", "r");
  printf("Parsing HTTP norepeat...\n");
  yaleyydoparse(f, &yale);
  fclose(f);
  printf("HTTP norepeat parsed\n");
  if (check_actions(&yale) != 0)
  {
    printf("Fail action\n");
    exit(1);
  }
  yale_free(&yale);
  yale = yaleorig;

  f = fopen("httppaper.txt", "r");
  printf("Parsing HTTP paper...\n");
  yaleyydoparse(f, &yalepaper);
  fclose(f);
  printf("HTTP paper parsed\n");
  if (check_actions(&yalepaper) != 0)
  {
    printf("Fail action\n");
    exit(1);
  }

  printf("HTTP parser code snippets: %s\n", yalepaper.cs.data);

  f = fopen("ssl.txt", "r");
  printf("Parsing SSL...\n");
  yaleyydoparse(f, &yale);
  fclose(f);
  printf("SSL parsed\n");
  yale_free(&yale);

  dump_python(&yalepaper);
  yale_free(&yalepaper);
  return 0;
}
