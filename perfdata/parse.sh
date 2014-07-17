#!/bin/bash
# Script to parse output of perf tool
plat=vinipc

function get_data {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,$plat,$i,"
		cat ${1}_log${i}.txt | grep -m 1 $3 | awk {'print $1'}
	done
}

function get_rate {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,$plat,$i,"
		cat ${1}_log${i}.txt | grep -m 1 $3 | awk {'print $4'}
	done
}

function sum_data {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,$plat,$i,"
        v1=$( cat ${1}_log${i}.txt | grep $3 | awk {'print $1'} | sed 's/,//g')
        v2=$( cat ${1}_log${i}.txt | grep $4 | awk {'print $1'} | sed 's/,//g')
        echo $(awk "BEGIN {printf \"%d\",$(( $v1+$v2 ))}")
	done
}

echo "Benchmark, Stat, Platform, Run#, Value"
cd logs > /dev/null;
sed -i -e 's/,//g' *
for kernel in regex crfsuite pocketsphinx porter dnn
do
	get_data $kernel "instructions" instructions
	get_data $kernel "cycles" cycles
	get_rate $kernel "ipc" insns
	get_rate $kernel "idle cycles" idle
	get_data $kernel "D-Cache-Accesses" cache-references
	get_data $kernel "D-Cache Misses" cache-misses
	get_rate $kernel "D-Cache Miss rate" refs
#	get_data $kernel "Branches" branches
#	get_data $kernel "Branch Misses" branch-misses
	get_data $kernel "Runtime" seconds
done

cd logs1 > /dev/null;
sed -i -e 's/,//g' *
for kernel in regex crfsuite pocketsphinx porter dnn
do
	get_data $kernel "L3 Loads" LLC-loads
	get_data $kernel "L3 Stores" LLC-stores
	sum_data $kernel "LLC-Accesses" LLC-loads LLC-stores
	get_data $kernel "L1 I-Cache Load Misses" L1-icache-load-misses
	get_data $kernel "L1 D-Cache Load Misses" L1-dcache-load-misses
	get_data $kernel "L1 D-Cache Store Misses" L1-dcache-store-misses
	sum_data $kernel "L1-D-Cache-Accesses" L1-dcache-load-misses L1-dcache-store-misses
	get_data $kernel "Stalled cycles" stalled
done

cd logs2 > /dev/null;
sed -i -e 's/,//g' *
for kernel in regex crfsuite pocketsphinx porter dnn
do
	get_data $kernel "Resource-stalls" 5301a2 
	get_data $kernel "Stalls-ldm" 65306a3
	get_data $kernel "LLC-refs" 534f2e 
	get_data $kernel "LLC-misses" 53412e 
	get_data $kernel "Arith-units" r530114
done

