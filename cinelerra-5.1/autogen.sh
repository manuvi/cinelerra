#!/bin/bash -x

rm -f global_config configure Makefile Makefile.in
rm -f aclocal.m4 depcomp compile install-sh ltmain.sh
rm -f config.{log,guess,h,h.in,sub,status} missing
rm -rf autom4te.cache m4

if [ "$1" = "clean" ]; then exit 0; fi

#autoupdate
mkdir m4
autoreconf --install

if [ "$(uname -o)" = "Android" ] || [ -e "/system/bin/app_process" ]; then
sed -i 's/usr\/bin\/sh/usr\/bin\/bash/' configure
fi