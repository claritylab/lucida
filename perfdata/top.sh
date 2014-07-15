#!/bin/bash
file=results.csv
plat=pc

function calc_mpki {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,pc,$i,"
        insns=$( cat $file | grep "$1,instructions,$plat,$i,.*" | awk 'BEGIN { FS = "," } {print $5}')
        v1=$( cat $file | egrep "$1,$3,$plat,$i,.*" | awk 'BEGIN { FS = "," } {print $5}')
        echo $(awk "BEGIN {printf \"%.2f\",$(( $v1*1000 ))/$insns}")
	done
}

# ./collect.sh
# ./collect1.sh
#
./parse.sh > $file
./parse1.sh >> $file

for kernel in regex crfsuite pocketsphinx porter
do
    calc_mpki $kernel "D-Cache MPKI" D-Cache-Accesses >> $file
    calc_mpki $kernel "L1 D-Cache MPKI" L1-D-Cache-Accesses >> $file
    calc_mpki $kernel "LLC MPKI" LLC-Accesses >> $file
done
