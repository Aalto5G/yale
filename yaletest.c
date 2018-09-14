#include "yale.h"
#include "yyutils.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  FILE *f = fopen("http.txt", "r");
  struct yale yale;
  yaleyydoparse(f, &yale);
  fclose(f);
  return 0;
}

