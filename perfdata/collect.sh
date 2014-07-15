#!/bin/bash

RESDIR=`pwd`/logs
mkdir -p $RESDIR

cd bins ;
for kernel in regex crfsuite pocketsphinx porter
do
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo $kernel run: $i
		perf stat -e instructions,cycles,stalled-cycles-frontend,cache-misses,cache-references -o ${kernel}_log${i}.txt ./${kernel}_script.sh
		# perf stat -e instructions,cycles,branches,branch-misses,cache-misses,cache-references -o ${kernel}_log${i}.txt ./${kernel}_script.sh
		mv ${kernel}_log${i}.txt $RESDIR
	done
done
