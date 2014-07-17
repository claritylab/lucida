#!/bin/bash

RESDIR=`pwd`/logs
mkdir -p $RESDIR
RESDIR1=`pwd`/logs1
mkdir -p $RESDIR
RESDIR2=`pwd`/logs2
mkdir -p $RESDIR2

stats=instructions,cycles,stalled-cycles-frontend,cache-misses,cache-references
#stats=instructions,cycles,branches,branch-misses,cache-misses,cache-references
stats1=L1-dcache-load-misses,L1-dcache-store-misses,L1-icache-load-misses,LLC-stores,LLC-loads,stalled-cycles-frontend 
#stalls,llc_refs,llc_miss,stalls_ldm_pending,arith,l0-icache-miss
stats2=r5301a2,r534f2e,r53412e,r65306a3,r530114,r530280

cd bins ;
for kernel in regex crfsuite pocketsphinx porter
do
	for i in 1 2 3 4 5 6 7 8 9 0
	do
		echo $kernel run: $i
		perf stat -e $stats -o ${kernel}_log${i}.txt ./${kernel}_script.sh
		mv ${kernel}_log${i}.txt $RESDIR
		perf stat -e $stats1 -o ${kernel}_log${i}.txt ./${kernel}_script.sh
		mv ${kernel}_log${i}.txt $RESDIR1
		perf stat -e $stats2 -o ${kernel}_log${i}.txt ./${kernel}_script.sh
		mv ${kernel}_log${i}.txt $RESDIR2
	done
done

cd ../../kernels/dnn/example-setup/recognition > /dev/null;

kernel=dnn
for i in 1 2 3 4 5 6 7 8 9 0
do
	echo $kernel run: $i
	perf stat -e $stats -o ${kernel}_log${i}.txt ./30_nn-recognition-hybrid.sh
	mv ${kernel}_log${i}.txt $RESDIR
	perf stat -e $stats1 -o ${kernel}_log${i}.txt ./30_nn-recognition-hybrid.sh
	mv ${kernel}_log${i}.txt $RESDIR1
	perf stat -e $stats2 -o ${kernel}_log${i}.txt ./30_nn-recognition-hybrid.sh
	mv ${kernel}_log${i}.txt $RESDIR2
done

