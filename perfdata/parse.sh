#!/bin/bash
# Script to parse output of perf tool

function get_data {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,pc,$i,"
		cat ${1}_log${i}.txt | grep -m 1 $3 | awk {'print $1'}
	done
}

function get_rate {
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo -n "$1,$2,pc,$i,"
		cat ${1}_log${i}.txt | grep -m 1 $3 | awk {'print $4'}
	done
}

cd logs > /dev/null;
sed -i -e 's/,//g' *
echo "Benchmark, Stat, Platform, Run#, Value"
for kernel in regex crfsuite pocketsphinx porter
do
	get_data $kernel "instructions" instructions
	get_data $kernel "cycles" cycles
	get_rate $kernel "ipc" insns
	get_rate $kernel "idle cycles" idle
	get_data $kernel "D-Cache Accesses" cache-references
	get_data $kernel "D-Cache Misses" cache-misses
	get_rate $kernel "D-Cache Miss rate" refs
	# get_data $kernel "Branches" branches
	# get_data $kernel "Branch Misses" branch-misses
	get_data $kernel "Runtime" seconds
done

