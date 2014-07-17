#!/bin/bash
file=results.csv
plat=vinipc

function calc_mpki {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,$plat,$i,"
		insns=$( cat $file | grep "$1,instructions,$plat,$i,.*" | awk 'BEGIN { FS = "," } {print $5}')
		v1=$( cat $file | grep "$1,$3,$plat,$i,.*" | awk 'BEGIN { FS = "," } {print $5}')
		echo $(awk "BEGIN {printf \"%.4f\",$(( $v1*1000 ))/$insns}")
	done
}

function calc_rate {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,$plat,$i,"
		v1=$( cat $file | grep "$1,$3,$plat,$i,.*" | awk 'BEGIN { FS = "," } {print $5}')
		v2=$( cat $file | grep "$1,$4,$plat,$i,.*" | awk 'BEGIN { FS = "," } {print $5}')
		echo $(awk "BEGIN {printf \"%.4f\",$v2/$v1}")
	done
}

./parse.sh > $file

for kernel in regex crfsuite pocketsphinx porter dnn
#for kernel in porter
do
    calc_mpki $kernel "D-Cache MPKI" D-Cache-Accesses >> $file
    calc_mpki $kernel "I-Cache MPKI" icache-misses >> $file
    calc_mpki $kernel "L1 I-Cache MPKI" L1-I-Cache-Load-Misses >> $file
    calc_mpki $kernel "L1 D-Cache MPKI" L1-D-Cache-Accesses >> $file
    calc_mpki $kernel "LLC MPKI" LLC-Accesses >> $file
    calc_rate $kernel "LDM-stall-rate" cycles stalls-ldm >> $file
    calc_rate $kernel "arith-unit-active-rate" cycles arith-units >> $file
    calc_rate $kernel "resource-stall-rate" cycles resource-stalls >> $file
    calc_rate $kernel "cpi" instructions cycles  >> $file
done
