#ifndef _YYUTILS_H_
#define _YYUTILS_H_

#include <stdio.h>
#include <stdint.h>
#include "yale.h"

void yaleyydoparse(FILE *filein, struct yale *yale);

void yaleyydomemparse(char *filedata, size_t filesize, struct yale *yale);

void yaleyynameparse(const char *fname, struct yale *yale, int require);

void yaleyydirparse(
  const char *argv0, const char *fname, struct yale *yale, int require);

char *yy_escape_string(char *orig);

uint32_t yy_get_ip(char *orig);

#endif

