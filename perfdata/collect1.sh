#!/bin/bash

RESDIR=`pwd`/logs1
mkdir -p $RESDIR
# rm -rf $RESDIR/*

cd bins ;
for kernel in regex crfsuite pocketsphinx porter
do
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo $kernel run: $i
		perf stat -e L1-dcache-load-misses,L1-dcache-store-misses,L1-icache-load-misses,LLC-stores,LLC-loads,stalled-cycles-frontend -o ${kernel}_log${i}.txt ./${kernel}_script.sh
		mv ${kernel}_log${i}.txt $RESDIR
	done
done
