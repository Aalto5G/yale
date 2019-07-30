#include "smtpclientcparser.h"
#include "smtpclientcommon.h"
#include <sys/time.h>

int main(int argc, char **argv)
{
  ssize_t consumed;
  struct smtpclient_parserctx pctx = {};
  char smtp[] =
    "HELO relay.example.com\r\n"
    "MAIL FROM:<bob@example.com>\r\n"
    "RCPT TO:<alice@example.com>\r\n"
    "RCPT TO:<theboss@example.com>\r\n"
    "DATA\r\n"
    "From: \"Bob Example\" <bob@example.com>\r\n"
    "To: Alice Example <alice@example.com>\r\n"
    "Cc: theboss@example.com\r\n"
    "Date: Tue, 15 Jan 2008 16:02:43 -0500\r\n"
    "Subject: Test message\r\n"
    "\r\n"
    "Hello Alice.\r\n"
    "This is a test message with 5 header fields and 4 lines in the message body.\r\n"
    "Your friend,\r\n"
    "Bob\r\n"
    ".\r\n"
    "QUIT\r\n";

  smtpclient_parserctx_init(&pctx);
  consumed = smtpclient_parse_block(&pctx, smtp, sizeof(smtp)-1, 1);
  printf("consumed: %zd\n", consumed);
  if (consumed != sizeof(smtp)-1)
  {
    abort();
  }

  return 0;
}
