#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include <arpa/inet.h>
#include "yale.h"
#include "yyutils.h"

typedef void *yyscan_t;
extern int yaleyyparse(yyscan_t scanner, struct yale *yale);
extern int yaleyylex_init(yyscan_t *scanner);
extern void yaleyyset_in(FILE *in_str, yyscan_t yyscanner);
extern int yaleyylex_destroy(yyscan_t yyscanner);

void yaleyydoparse(FILE *filein, struct yale *yale)
{
  yyscan_t scanner;
  yaleyylex_init(&scanner);
  yaleyyset_in(filein, scanner);
  if (yaleyyparse(scanner, yale) != 0)
  {
    fprintf(stderr, "parsing failed\n");
    exit(1);
  }
  yaleyylex_destroy(scanner);
  if (!feof(filein))
  {
    fprintf(stderr, "error: additional data at end of yale data\n");
    exit(1);
  }
}

void yaleyydomemparse(char *filedata, size_t filesize, struct yale *yale)
{
  FILE *myfile;
  myfile = fmemopen(filedata, filesize, "r");
  if (myfile == NULL)
  {
    fprintf(stderr, "can't open memory file\n");
    exit(1);
  }
  yaleyydoparse(myfile, yale);
  if (fclose(myfile) != 0)
  {
    fprintf(stderr, "can't close memory file\n");
    exit(1);
  }
}

static void *memdup(const void *mem, size_t sz)
{
  void *result;
  result = malloc(sz);
  if (result == NULL)
  {
    return result;
  }
  memcpy(result, mem, sz);
  return result;
}

struct escaped_string yy_escape_string(char *orig)
{
  char *buf = NULL;
  char *result = NULL;
  struct escaped_string resultstruct;
  size_t j = 0;
  size_t capacity = 0;
  size_t i = 1;
  while (orig[i] != '"')
  {
    if (j >= capacity)
    {
      char *buf2;
      capacity = 2*capacity+10;
      buf2 = realloc(buf, capacity);
      if (buf2 == NULL)
      {
        free(buf);
        resultstruct.str = NULL;
        return resultstruct;
      }
      buf = buf2;
    }
    if (orig[i] != '\\')
    {
      buf[j++] = orig[i++];
    }
    else if (orig[i+1] == 'x')
    {
      char hexbuf[3] = {0};
      hexbuf[0] = orig[i+2];
      hexbuf[1] = orig[i+3];
      buf[j++] = strtol(hexbuf, NULL, 16);
      i += 4;
    }
    else if (orig[i+1] == 't')
    {
      buf[j++] = '\t';
      i += 2;
    }
    else if (orig[i+1] == 'r')
    {
      buf[j++] = '\r';
      i += 2;
    }
    else if (orig[i+1] == 'n')
    {
      buf[j++] = '\n';
      i += 2;
    }
    else
    {
      buf[j++] = orig[i+1];
      i += 2;
    }
  }
  if (j >= capacity)
  {
    char *buf2;
    capacity = 2*capacity+10;
    buf2 = realloc(buf, capacity);
    if (buf2 == NULL)
    {
      free(buf);
      resultstruct.str = NULL;
      return resultstruct;
    }
    buf = buf2;
  }
  resultstruct.sz = j;
  buf[j++] = '\0';
  result = memdup(buf, j);
  resultstruct.str = result;
  free(buf);
  return resultstruct;
}

uint32_t yy_get_ip(char *orig)
{
  struct in_addr addr;
  if (inet_aton(orig, &addr) == 0)
  {
    return 0;
  }
  return ntohl(addr.s_addr);
}

void yaleyynameparse(const char *fname, struct yale *yale, int require)
{
  FILE *yalefile;
  yalefile = fopen(fname, "r");
  if (yalefile == NULL)
  {
    if (require)
    {
      fprintf(stderr, "File %s cannot be opened\n", fname);
      exit(1);
    }
#if 0
    if (yale_postprocess(yale) != 0)
    {
      exit(1);
    }
#endif
    return;
  }
  yaleyydoparse(yalefile, yale);
#if 0
  if (yale_postprocess(yale) != 0)
  {
    exit(1);
  }
#endif
  fclose(yalefile);
}

void yaleyydirparse(
  const char *argv0, const char *fname, struct yale *yale, int require)
{
  const char *dir;
  char *copy = strdup(argv0);
  char pathbuf[PATH_MAX];
  dir = dirname(copy); // NB: not for multi-threaded operation!
  snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dir, fname);
  free(copy);
  yaleyynameparse(pathbuf, yale, require);
}
