#!/bin/sh

for i in `find $1 -name "$2" -print | sort`; do
        echo $i
	ln -s $i .
done
