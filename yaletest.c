#include "yale.h"
#include "yyutils.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  FILE *f = fopen("http.txt", "r");
  struct yale yale;
  printf("Parsing HTTP...\n");
  yaleyydoparse(f, &yale);
  fclose(f);
  printf("HTTP parsed\n");

  f = fopen("ssl.txt", "r");
  printf("Parsing SSL...\n");
  yaleyydoparse(f, &yale);
  fclose(f);
  printf("SSL parsed\n");
  return 0;
}
