
clang -g -O1 -S -fno-slp-vectorize -fno-vectorize -fno-unroll-loops -fno-inline -emit-llvm -o porter.llvm porter.c

opt -S -load=$TRACER_HOME/full-trace/full_trace.so -fulltrace porter.llvm -o porter-opt.llvm

llvm-link -o porter.llvm porter-opt.llvm $TRACER_HOME/profile-func/trace_logger.llvm

llc -filetype=asm -o full.s full.llvm
gcc -fno-inline -o porter-instrumented full.s

./porter-instrumented


