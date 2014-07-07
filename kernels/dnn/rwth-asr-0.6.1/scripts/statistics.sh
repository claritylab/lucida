#!/bin/sh

function sources() {
    find $1 -name '*.[cChH]' -or -name '*.cc' -or -name '*.hh' -or -name '*.py' | grep -v '#'
}

echo -e 'directory\t\t\t\t\t  lines\t  bytes'
echo '---------------------------------------------------------------'
for i in `find . -type d -not -path '*/.build*' -not -name CVS -not -path "*/.svn*" -print | sort`; do
	echo $i | awk '{ printf "%-41s",$1 }'
	echo -ne "\t"
	sources $i | xargs cat | wc -lc
done

echo '---------------------------------------------------------------'
echo -ne '.\t\t\t\t\t\t'
sources . | xargs cat | wc -lc
