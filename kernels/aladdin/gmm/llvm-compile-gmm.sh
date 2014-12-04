export WORKLOAD=computeScore_seq

clang -g -O1 -S -fno-slp-vectorize -fno-vectorize -fno-unroll-loops -fno-inline -emit-llvm -o gmm.llvm gmm.c

opt -S -load=$TRACER_HOME/full-trace/full_trace.so -fulltrace gmm.llvm -o gmm-opt.llvm

llvm-link -o gmm.llvm gmm-opt.llvm $TRACER_HOME/profile-func/trace_logger.llvm

llc -filetype=asm -o full.s gmm.llvm
gcc -fno-inline -o gmm-instrumented full.s -lm

./gmm-instrumented


