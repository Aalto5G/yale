#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

size_t freadall(void **pbuf, size_t *pcap, FILE *stream)
{
  char *buf = *pbuf;
  size_t cap = *pcap;
  size_t sz = 0;
  size_t ret;
  for (;;)
  {
    if (sz >= cap)
    {
      cap += 1;
      cap *= 2;
      buf = realloc(buf, cap);
    }
    ret = fread(&buf[sz], 1, cap - sz, stream);
    if (ret == 0)
    {
      if (ferror(stream))
      {
        *pbuf = buf;
        *pcap = cap;
        return 0;
      }
      else if (feof(stream))
      {
        *pbuf = buf;
        *pcap = cap;
        return sz;
      }
      else
      {
        abort();
      }
    }
    sz += ret;
  }
}

void testmain(char *cmd, char *exp)
{
  FILE *p;
  size_t sz;
  void *buf = NULL;
  size_t cap = 0;
  if (access(cmd, X_OK) != 0)
  {
    printf("test %s missing: skip\n", cmd);
    return;
  }
  p = popen(cmd, "r");
  if (p == NULL)
  {
    abort();
  }
  sz = freadall(&buf, &cap, p);
  if (sz == 0 && ferror(p))
  {
    abort();
  }
  if (memcmp(buf, exp, strlen(exp)) != 0)
  {
    printf("test %s fail\n", cmd);
    exit(1);
  }
  free(buf);
  if (pclose(p) != 0)
  {
    printf("test %s exit status fail\n", cmd);
    exit(1);
  }
}

void backtracktestcbmain(void)
{
  char *expected =
    "1: <a>\n"
    "2: <a>\n"
    "Succeed i = 0\n"
    "1: [b]\n"
    "2: []!\n"
    "Succeed i = 1\n"
    "1: [c]\n"
    "Succeed i = 2\n"
    "1: [d]\n"
    "Succeed i = 3\n"
    "2: <bc>\n"
    "2: [d]\n"
    "1: []!\n"
    "2: [f]\n"
    "Succeed i = 4\n"
    "1: <a>\n"
    "2: []-\n"
    "Succeed i = 5\n"
    "1: [b]\n"
    "Succeed i = 6\n"
    "1: [d]\n"
    "Succeed i = 7\n"
    "1: [e]\n"
    "Succeed i = 8\n";
  testmain("./backtracktestcbmain", expected);
}

void backtracktestmain(void)
{
  char *expected =
    "Succeed i = 0\n"
    "Succeed i = 1\n"
    "Succeed i = 2\n"
    "Succeed i = 3\n"
    "Succeed i = 4\n"
    "Succeed i = 5\n"
    "Succeed i = 6\n"
    "Succeed i = 0\n"
    "Succeed i = 1\n"
    "Succeed i = 2\n"
    "Succeed i = 3\n"
    "Succeed i = 4\n"
    "Succeed i = 5\n"
    "Succeed i = 6\n"
    "Succeed i = 7\n"
    "Succeed i = 8\n"
    "Succeed i = 9\n"
    "Succeed i = 10\n"
    "Succeed i = 11\n"
    "Succeed i = 0\n"
    "Succeed i = 1\n"
    "Succeed i = 2\n"
    "Succeed i = 3\n"
    "Succeed i = 4\n"
    "Succeed i = 5\n"
    "Succeed i = 6\n"
    "Succeed i = 7\n"
    "Succeed i = 8\n"
    "Succeed i = 9\n"
    "Succeed i = 10\n"
    "Succeed i = 11\n"
    "Succeed i = 0\n"
    "Succeed i = 1\n"
    "Succeed i = 2\n"
    "Succeed i = 3\n"
    "Succeed i = 4\n"
    "Succeed i = 5\n"
    "Succeed i = 6\n"
    "Succeed i = 7\n"
    "Succeed i = 8\n"
    "Succeed i = 9\n"
    "Succeed i = 10\n"
    "Succeed i = 11\n"
    "Succeed i = 12\n"
    "Succeed i = 13\n"
    "Succeed i = 14\n"
    "Succeed i = 15\n"
    "Succeed i = 16\n";
  testmain("./backtracktestmain", expected);
}

void httpcmainprint(void)
{
  char *expected =
    "<www.google.fi>\n"
    "<>-\n"
    "< >\n"
    "<www.google2.fi>\n"
    "<>-\n"
    "Consumed: 665\n";
  testmain("./httpcmainprint", expected);
}

void sslcmainprint(void)
{
  char *expected =
    "sz: 136\n"
    "<localhost>\n"
    "----------------\n"
    "<l>\n"
    "[o]\n"
    "[c]\n"
    "[a]\n"
    "[l]\n"
    "[h]\n"
    "[o]\n"
    "[s]\n"
    "[t]\n";
  testmain("./sslcmainprint", expected);
}

void lenprefixcmain(void)
{
  char *expected =
    "<\\x00\\x00>A\n"
    "[\\x00\\x00]A\n"
    "[]A\n"
    "<>\n"
    "[\\x00\\x00]A\n"
    "<>\n"
    "[\\x00\\x01]A\n"
    "[G]A\n"
    "<G>\n"
    "[\\x00\\x00]A\n"
    "<>\n"
    "[\\x00\\x02]A\n"
    "[GH]A\n"
    "<GH>\n"
    "[\\x00\\x00]A\n"
    "<>\n"
    "[\\x00\\x03]A\n"
    "[GHI]A\n"
    "<GHI>\n"
    "[\\x00\\x00]A\n"
    "<>\n"
    "[\\x00\\x04]A\n"
    "[GHIJ]A\n"
    "<GHIJ>\n"
    "[\\x00\\x00]A\n"
    "<>\n"
    "[\\x00\\x05]A\n"
    "[GHIJK]A\n"
    "<GHIJK>\n"
    "[\\x00\\x01]A\n"
    "<>\n"
    "[\\x00\\x00]A\n"
    "[]A\n"
    "<>\n"
    "<>C\n"
    "[\\x00\\x01]A\n"
    "<>\n"
    "<>C\n"
    "[\\x00\\x01]A\n"
    "[g]A\n"
    "<g>\n"
    "<g>C\n"
    "[\\x00\\x01]A\n"
    "<>\n"
    "<>C\n"
    "[\\x00\\x02]A\n"
    "[gh]A\n"
    "<gh>\n"
    "<gh>C\n"
    "[\\x00\\x01]A\n"
    "<>\n"
    "<>C\n"
    "[\\x00\\x03]A\n"
    "[ghi]A\n"
    "<ghi>\n"
    "<ghi>C\n"
    "[\\x00\\x01]A\n"
    "<>\n"
    "<>C\n"
    "[\\x00\\x04]A\n"
    "[ghij]A\n"
    "<ghij>\n"
    "<ghij>C\n"
    "[\\x00\\x01]A\n"
    "<>\n"
    "<>C\n"
    "[\\x00\\x05]A\n"
    "[ghijk]A\n"
    "<ghijk>\n"
    "<ghijk>C\n"
    "----------------\n"
    "<\\x00>A\n"
    "[\\x00]A\n"
    "[\\x00]A\n"
    "[\\x00]A\n"
    "[]A\n"
    "<>\n"
    "[\\x00]A\n"
    "<>\n"
    "<>\n"
    "[\\x00]A\n"
    "[\\x00]A\n"
    "[\\x01]A\n"
    "[G]A\n"
    "<G>\n"
    "[\\x00]A\n"
    "<>\n"
    "<>\n"
    "[\\x00]A\n"
    "[\\x00]A\n"
    "[\\x02]A\n"
    "[G]A\n"
    "<G>\n"
    "[H]A\n"
    "[H]\n"
    "[\\x00]A\n"
    "<>\n"
    "<>\n"
    "[\\x00]A\n"
    "[\\x00]A\n"
    "[\\x03]A\n"
    "[G]A\n"
    "<G>\n"
    "[H]A\n"
    "[H]\n"
    "[I]A\n"
    "[I]\n"
    "[\\x00]A\n"
    "<>\n"
    "<>\n"
    "[\\x00]A\n"
    "[\\x00]A\n"
    "[\\x04]A\n"
    "[G]A\n"
    "<G>\n"
    "[H]A\n"
    "[H]\n"
    "[I]A\n"
    "[I]\n"
    "[J]A\n"
    "[J]\n"
    "[\\x00]A\n"
    "<>\n"
    "<>\n"
    "[\\x00]A\n"
    "[\\x00]A\n"
    "[\\x05]A\n"
    "[G]A\n"
    "<G>\n"
    "[H]A\n"
    "[H]\n"
    "[I]A\n"
    "[I]\n"
    "[J]A\n"
    "[J]\n"
    "[K]A\n"
    "[K]\n"
    "[\\x00]A\n"
    "<>\n"
    "<>\n"
    "[\\x01]A\n"
    "[\\x00]A\n"
    "[\\x00]A\n"
    "[]A\n"
    "<>\n"
    "<>C\n"
    "[\\x00]A\n"
    "<>\n"
    "<>C\n"
    "<>\n"
    "<>C\n"
    "[\\x01]A\n"
    "[\\x00]A\n"
    "[\\x01]A\n"
    "[g]A\n"
    "<g>\n"
    "<g>C\n"
    "[\\x00]A\n"
    "<>\n"
    "<>C\n"
    "<>\n"
    "<>C\n"
    "[\\x01]A\n"
    "[\\x00]A\n"
    "[\\x02]A\n"
    "[g]A\n"
    "<g>\n"
    "<g>C\n"
    "[h]A\n"
    "[h]\n"
    "[h]C\n"
    "[\\x00]A\n"
    "<>\n"
    "<>C\n"
    "<>\n"
    "<>C\n"
    "[\\x01]A\n"
    "[\\x00]A\n"
    "[\\x03]A\n"
    "[g]A\n"
    "<g>\n"
    "<g>C\n"
    "[h]A\n"
    "[h]\n"
    "[h]C\n"
    "[i]A\n"
    "[i]\n"
    "[i]C\n"
    "[\\x00]A\n"
    "<>\n"
    "<>C\n"
    "<>\n"
    "<>C\n"
    "[\\x01]A\n"
    "[\\x00]A\n"
    "[\\x04]A\n"
    "[g]A\n"
    "<g>\n"
    "<g>C\n"
    "[h]A\n"
    "[h]\n"
    "[h]C\n"
    "[i]A\n"
    "[i]\n"
    "[i]C\n"
    "[j]A\n"
    "[j]\n"
    "[j]C\n"
    "[\\x00]A\n"
    "<>\n"
    "<>C\n"
    "<>\n"
    "<>C\n"
    "[\\x01]A\n"
    "[\\x00]A\n"
    "[\\x05]A\n"
    "[g]A\n"
    "<g>\n"
    "<g>C\n"
    "[h]A\n"
    "[h]\n"
    "[h]C\n"
    "[i]A\n"
    "[i]\n"
    "[i]C\n"
    "[j]A\n"
    "[j]\n"
    "[j]C\n"
    "[k]A\n"
    "[k]\n"
    "[k]C\n";
  testmain("./lenprefixcmain", expected);
}

void recursivecbmain(void)
{
  char *expected =
    "2: <d>\n"
    "4: <d>\n"
    "1: <e>\n"
    "2: <e>\n"
    "4: <>\n"
    "3: <e>\n"
    "1: <>\n"
    "2: <>\n"
    "3: <>\n";
  testmain("./recursivecbmain", expected);
}

void reprefixcmain(void)
{
  char *expected =
    "<\\x00\\x01>A\n"
    "[\\x00\\x02]A\n"
    "[GH]A\n"
    "<GH>C\n"
    "<GH>\n"
    "----------------\n"
    "<\\x00>A\n"
    "[\\x01]A\n"
    "[\\x00]A\n"
    "[\\x02]A\n"
    "[G]A\n"
    "<G>C\n"
    "<G>\n"
    "[H]A\n"
    "[H]C\n"
    "[H]\n";
  testmain("./reprefixcmain", expected);
}

void tokentheft1main(void)
{
  char *expected = "CCLL(1)";
  testmain("./tokentheft1main", expected);
}

void tokentheft1smain(void)
{
  char *expected = "SCCLL(1)";
  testmain("./tokentheft1smain", expected);
}

int main(int argc, char **argv)
{
  backtracktestcbmain();
  backtracktestmain();
  httpcmainprint();
  sslcmainprint();
  lenprefixcmain();
  recursivecbmain();
  reprefixcmain();
  tokentheft1main();
  tokentheft1smain();
  return 0;
}
