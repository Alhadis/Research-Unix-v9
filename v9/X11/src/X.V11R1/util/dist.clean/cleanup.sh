#!/bin/sh

case "$0" in
*/*)	DIR=`expr "$0" : '\(.*\/\).*'`;;
*)	DIR=;;
esac

case "$1" in
"")	echo "Usage: $0 dir"
	exit 1
	;;
esac

trap "rm -f /tmp/cleanup.*; exit 0" 0 1 2
find $1 \
    \( \
	\( -type f \
		\( -name '*.[oa]' \
		-o -name '*~' \
		-o -name '#*' \
		-o -name '*.old' \
		-o -name '*.diff' \
		-o -name 'diffout' \
		-o -name '*.orig' \
		-o -name '*.rej' \
		-o -name '*.log' \
		-o -name 'tags' \
		-o -name 'TAGS' \
		-o -name 'core' \
		-o -name 'Makefile.bak' \
		-o -name '*.ln' \
		\) \
	\) \
	-o \
	\( -type d -a -name RCS \) \
    \) -print \
| grep -v doc/Xlib/Xsrc | split -100 - /tmp/cleanup.

for file in /tmp/cleanup.??
do
	rm -ri `cat $file`
done
rm -i `find $1 -perm -711 -type f -print`
rm -ri $1/fonts/snf/* \
	$1/clients/xmh \
	$1/server/servertype \
	$1/server/allfiles \
	$1/bin \
	$1/lib

echo "cleaning up symbolic links"
${DIR}makelinks.sh $1 > $1/link-setup

rm -i `find $1 -type l -print`
