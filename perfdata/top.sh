#!/bin/bash
file=results.csv

function calc_mpki {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,pc,$i,"
        insns=$( cat $file | grep "$1,instructions,pc,$i,*" | awk 'BEGIN { FS = "," } {print $5}')
        insns=$( cat $file | grep "$1,$3,pc,$i,*" | awk 'BEGIN { FS = "," } {print $5}')
        echo $(awk "BEGIN {printf \"%d\",$(( $v1*1000 ))/$v2 ))}")
	done
}

./collect.sh
./collect1.sh

./parse.sh > $file
./parse1.sh >> $file


for kernel in regex crfsuite pocketsphinx porter
do
    calc_mpki $kernel "D-Cache MPKI" D-Cache Accesses >> $file
    calc_mpki $kernel "L1 D-Cache MPKI" L1 D-Cache Accesses >> $file
    calc_mpki $kernel "LLC MPKI" LLC Accesses >> $file
done
