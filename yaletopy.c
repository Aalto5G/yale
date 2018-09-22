#include "yale.h"
#include "yyutils.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  FILE *f, *fout;
  struct yale yale = {};

  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s file.txt file.py\n", argv[0]);
    exit(1);
  }

  f = fopen(argv[1], "r");
  if (f == NULL)
  {
    fprintf(stderr, "Can't open input file\n");
    exit(1);
  }
  yaleyydoparse(f, &yale);
  fclose(f);
  if (check_actions(&yale) != 0)
  {
    printf("Fail action\n");
    exit(1);
  }

  fout = fopen(argv[2], "w");
  if (fout == NULL)
  {
    fprintf(stderr, "Can't open input file\n");
    exit(1);
  }
  check_python(&yale);
  dump_python(fout, &yale);
  yale_free(&yale);
  fclose(fout);
  return 0;
}
