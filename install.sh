#!/bin/sh

if [ '!' -f "main/yaleparser" ]; then
  echo "yale not made"
  exit 1
fi

PREFIX="$1"

if [ "a$PREFIX" = "a" ]; then
  PREFIX=/usr/local
fi

P="$PREFIX"
H="`hostname`"

if [ '!' -w "$P" ]; then
  echo "No write permissions to $P"
  exit 1
fi
if [ '!' -d "$P" ]; then
  echo "Not a valid directory: $P"
  exit 1
fi

instbin()
{
  if [ -e "$P/bin/$1" ]; then
    ln "$P/bin/$1" "$P/bin/.$1.yaleinstold.$$.$H" || exit 1
  fi
  cp "$1/$2" "$P/bin/.$2.yaleinstnew.$$.$H" || exit 1
  mv "$P/bin/.$2.yaleinstnew.$$.$H" "$P/bin/$2" || exit 1
  if [ -e "$P/bin/.$2.yaleinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/bin/.$2.yaleinstold.$$.$H" || exit 1
  fi
}

instlib2()
{
  if [ -e "$P/lib/$2" ]; then
    ln "$P/lib/$2" "$P/lib/.$2.yaleinstold.$$.$H" || exit 1
  fi
  cp "$1/$2" "$P/lib/.$2.yaleinstnew.$$.$H" || exit 1
  mv "$P/lib/.$2.yaleinstnew.$$.$H" "$P/lib/$2" || exit 1
  if [ -e "$P/lib/.$2.yaleinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/lib/.$2.yaleinstold.$$.$H" || exit 1
  fi
}
instinc2()
{
  if [ -e "$P/include/$2" ]; then
    ln "$P/include/$2" "$P/include/.$2.yaleinstold.$$.$H" || exit 1
  fi
  cp "$1/$2" "$P/include/.$2.yaleinstnew.$$.$H" || exit 1
  mv "$P/include/.$2.yaleinstnew.$$.$H" "$P/include/$2" || exit 1
  if [ -e "$P/include/.$2.yaleinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/include/.$2.yaleinstold.$$.$H" || exit 1
  fi
}
instexample2()
{
  if [ -e "$P/share/examples/yaleparser/$1/$2" ]; then
    ln "$P/share/examples/yaleparser/$1/$2" "$P/share/examples/yaleparser/$1/.$2.yaleinstold.$$.$H" || exit 1
  fi
  cp "test/$2" "$P/share/examples/yaleparser/$1/.$2.yaleinstnew.$$.$H" || exit 1
  mv "$P/share/examples/yaleparser/$1/.$2.yaleinstnew.$$.$H" "$P/share/examples/yaleparser/$1/$2" || exit 1
  if [ -e "$P/share/examples/yaleparser/$1/.$2.yaleinstold.$$.$H" ]; then
    # If you mount binaries across NFS, and run this command on the NFS server,
    # you might want to comment out this rm command.
    rm "$P/share/examples/yaleparser/$1/.$2.yaleinstold.$$.$H" || exit 1
  fi
}
instman()
{
  mkdir -p "$P/man/man$2" || exit 1
  cp "$1.$2" "$P/man/man$2/.$1.$2.yaleinstnew.$$.$H" || exit 1
  mv "$P/man/man$2/.$1.$2.yaleinstnew.$$.$H" "$P/man/man$2/$1.$2" || exit 1
}

instbin main yaleparser
instinc2 runtime yalecommon.h
instman yaleparser 1

mkdir -p $P/share/examples/yaleparser
mkdir -p $P/share/examples/yaleparser/ssl
mkdir -p $P/share/examples/yaleparser/http1
mkdir -p $P/share/examples/yaleparser/http2
mkdir -p $P/share/examples/yaleparser/httpresp
mkdir -p $P/share/examples/yaleparser/smtp
mkdir -p $P/share/examples/yaleparser/cond

instexample2 ssl ssl1.txt
instexample2 ssl ssl2.txt
instexample2 ssl ssl3.txt
instexample2 ssl ssl4.txt
instexample2 ssl ssl5.txt
instexample2 ssl ssl6.txt
instexample2 ssl sslcmainprint.c
instexample2 ssl sslcommon.h
B="$P/share/examples/yaleparser/ssl/build.sh"
echo "#!/bin/sh" > "$B"
echo "yaleparser ssl1.txt b" >> "$B"
echo "yaleparser ssl2.txt b" >> "$B"
echo "yaleparser ssl3.txt b" >> "$B"
echo "yaleparser ssl4.txt b" >> "$B"
echo "yaleparser ssl5.txt b" >> "$B"
echo "yaleparser ssl6.txt b" >> "$B"
echo "cc -O3 -I/usr/local/include ssl?cparser.c sslcmainprint.c" >> "$B"
chmod a+x "$B"

instexample2 http1 httppaper.txt
instexample2 http1 httpcmainprint.c
instexample2 http1 httpcommon.h
B="$P/share/examples/yaleparser/http1/build.sh"
echo "#!/bin/sh" > "$B"
echo "yaleparser httppaper.txt b" >> "$B"
echo "cc -O3 -I/usr/local/include httpcparser.c httpcmainprint.c" >> "$B"
chmod a+x "$B"

instexample2 http2 httppy.txt
instexample2 http2 httppyperf.c
instexample2 http2 httppycommon.h
B="$P/share/examples/yaleparser/http2/build.sh"
echo "#!/bin/sh" > "$B"
echo "yaleparser httppy.txt b" >> "$B"
echo "cc -O3 -I/usr/local/include httppycparser.c httppyperf.c" >> "$B"
chmod a+x "$B"

instexample2 httpresp httpresp.txt
instexample2 httpresp httprespcommon.h
instexample2 httpresp httprespcmain.c
B="$P/share/examples/yaleparser/httpresp/build.sh"
echo "#!/bin/sh" > "$B"
echo "yaleparser httpresp.txt b" >> "$B"
echo "cc -O3 -I/usr/local/include httprespcparser.c httprespcmain.c" >> "$B"
chmod a+x "$B"

instexample2 smtp smtpclient.txt
instexample2 smtp smtpclientcommon.h
instexample2 smtp smtpclientmain.c
B="$P/share/examples/yaleparser/smtp/build.sh"
echo "#!/bin/sh" > "$B"
echo "yaleparser smtpclient.txt b" >> "$B"
echo "cc -O3 -I/usr/local/include smtpclientcparser.c smtpclientmain.c" >> "$B"
chmod a+x "$B"

instexample2 cond condparser.txt
instexample2 cond condparsercommon.h
instexample2 cond condtest.c
B="$P/share/examples/yaleparser/cond/build.sh"
echo "#!/bin/sh" > "$B"
echo "yaleparser condparser.txt b" >> "$B"
echo "cc -O3 -I/usr/local/include condparsercparser.c condtest.c" >> "$B"
chmod a+x "$B"

echo "All done, yale has been installed to $P"
