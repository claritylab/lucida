#!/bin/bash
# Script to parse output of perf tool

plat=pc

function get_data {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,$plat,$i,"
		cat ${1}_log${i}.txt | grep $3 | awk {'print $1'}
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

cd logs1 > /dev/null;
sed -i -e 's/,//g' *
echo "Benchmark, Stat, Platform, Run#, Value"
for kernel in regex crfsuite pocketsphinx porter
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
