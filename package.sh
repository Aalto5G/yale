#!/bin/sh

die()
{
  echo "$@"
  exit 1
}

tmpdir="`mktemp -d`"
pwd="`pwd`"
cd "$tmpdir" || die "Can't goto tmp dir"
git clone --recursive https://github.com/Aalto5G/yale || die "Can't clone"
cd yale
smka yy/yale.lex.c yy/yale.lex.h yy/yale.tab.c yy/yale.tab.h
vname="`git describe --tags`"
if [ "$vname" = "" ]; then
  die "git describe failed"
fi
rm -rf .git
cd ..
mv yale yale-"$vname" || die "Can't rename directory"
tar czvf yale-"$vname".tar.gz yale-"$vname" || die "Can't create tar.gz"
rm -rf yale-"$vname"
cd "$pwd" || die "Can't chdir back to old directory"
if [ -e "yale-$vname.tar.gz" ]; then
  die "Package yale-$vname.tar.gz already exists"
else
  mv "$tmpdir/yale-$vname.tar.gz" . || die "Can't move package"
fi
rmdir "$tmpdir" || die "Can't rm temporary dir"
echo
echo "Package created:"
echo "yale-$vname.tar.gz"
