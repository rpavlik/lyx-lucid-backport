#!/bin/sh

test -x /usr/bin/kfmclient || exec see "$1" || exit 1

TMPFILE=`mktemp -q -t lyxview.XXXXXXXXXX` || exec see "$1" || exit 1

trap "rm -f $TMPFILE; exit 1" HUP INT QUIT TERM

if dcop 2>&1 | grep -qs konqueror-; then
    kfmclient exec "$1" 2> $TMPFILE
    if [ $? -eq 1 ] && grep -qs "cannot connect to X server" $TMPFILE; then
	see "$1"
    fi
else
    see "$1"
fi

rm -f $TMPFILE
