#!/bin/sh

test -x /usr/bin/kprinter || exec lpr "$@" || exit 1

TMPFILE=`mktemp -q -t lyxview.XXXXXXXXXX` || exec lpr "$@" || exit 1

trap "rm -f $TMPFILE; exit 1" HUP INT QUIT TERM

kprinter "$@" 2> $TMPFILE

if [ $? -eq 1 ] && grep -qs "cannot connect to X server" $TMPFILE; then
    lpr "$@"
fi

rm -f $TMPFILE
