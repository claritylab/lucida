#!/usr/bin/env python 
import os
import sys

kernels = {
'triad' : 'triad',
}

def main (directory, source):
  
  if not 'TRACER_HOME' in os.environ:
    raise Exception('Set TRACER_HOME directory as an environment variable')
  
  os.chdir(directory)
  obj = source + '.llvm'
  opt_obj = source + '-opt.llvm'
  executable = source + '-instrumented'
  os.environ['WORKLOAD']=kernels[source]

  source_file = source + '.c'
  os.system('clang -g -O1 -S -fno-slp-vectorize -fno-vectorize -fno-unroll-loops -fno-inline -emit-llvm -o ' + obj + ' '  + source_file)
  os.system('opt -disable-inlining -S -load=' + os.getenv('TRACER_HOME') + '/full-trace/full_trace.so -fulltrace ' + obj + ' -o ' + opt_obj)
  os.system('llvm-link -o full.llvm ' + opt_obj + ' ' + os.getenv('TRACER_HOME') + '/profile-func/trace_logger.llvm')
  os.system('llc -O0 -disable-fp-elim -filetype=asm -o full.s full.llvm')
  os.system('gcc -O0 -fno-inline -o ' + executable + ' full.s -lm')
  os.system('./' + executable)

if __name__ == '__main__':
  directory = sys.argv[1]
  source = sys.argv[2]
  print directory, source
  main(directory, source)
