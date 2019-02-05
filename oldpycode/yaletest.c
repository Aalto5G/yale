#include "yale.h"
#include "yyutils.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  FILE *f;
  const struct yale yaleorig = {};
  struct yale yale = {};
  struct yale yalepaper = {};

  f = fopen("reallysimple.txt", "r");
  if (f == NULL)
  {
    abort();
  }
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

  f = fopen("httpnorepeat.txt", "r");
  if (f == NULL)
  {
    abort();
  }
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
  if (f == NULL)
  {
    abort();
  }
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

  printf("\n\n\n");
  check_python(&yalepaper);
  dump_python(stdout, &yalepaper);
  yale_free(&yalepaper);
  return 0;
}
