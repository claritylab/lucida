#!/usr/bin/env python

kernels = [ 'fe', 'fd', 'gmm', 'regex', 'stemmer', 'crf', 'dnn-asr']
platforms = [ 'baseline', 'pthread', 'gpu' ]

import sys, os, re, subprocess, json
from collections import defaultdict
import numpy as np

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

def shcom(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = p.communicate()[0]
    return out

def main( args ):
    if len(args) < 2:
        print "Usage: ./parse-stats.py <.json file>"
        return

    file = args[1]
    data = json.loads(open(file).read())

    # dictionary of lists
    dict_of_lists = defaultdict(list)
    for i in range(0, len(data)):
        dict_of_lists[data[i]["abrv"]].append(data[i][data[i]["abrv"]])

    avg = {}
    median = {}
    stddev = {}
    mn = {}
    mx = {}

    for k, v in dict_of_lists.iteritems():
        avg[k] = np.average(v)
        median[k] = np.median(v)
        stddev[k] = np.std(v)
        mn[k] = min(v)
        mx[k] = max(v)

    print 'kernel,mean,median,stddev,min,max'
    for base in kernels:
        print "%s,%.2f,%.2f,%.2f,%.2f,%.2f" % (base, avg[base], median[base],
                                               stddev[base], mn[base],
                                               mx[base])

    for pth in kernels:
        base = 'pthread_' + pth
        print "%s,%.2f,%.2f,%.2f,%.2f,%.2f" % (base, avg[base], median[base],
                                               stddev[base], mn[base],
                                               mx[base])

    if shcom("which nvcc") == "":
        return

    for gpu in kernels:
        base = 'gpu_' + gpu
        if base == 'gpu_regex' or base == 'gpu_crf':
            continue
        print "%s,%.2f,%.2f,%.2f,%.2f,%.2f" % (base, avg[base], median[base],
                                               stddev[base], mn[base],
                                               mx[base])

if __name__=='__main__':
    sys.exit(main(sys.argv))
