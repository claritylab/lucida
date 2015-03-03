#!/usr/bin/env python

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

    kernels = [ 'fe', 'fd', 'gmm', 'regex', 'stemmer', 'crf', 'dnn-asr']
    platforms = [ 'baseline', 'pthread', 'gpu' ]

    # dictionary of lists
    dict_of_lists = defaultdict(list)

    # change this to collect other stats
    stat = 'abrv'
    for i in range(0, len(data)):
        dict_of_lists[data[i]["abrv"]].append(data[i][data[i][stat]])

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

    print 'kernel,platform,mean,median,stddev,min,max,speedup'
    for base in kernels:
        print "%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f" % (base, 'baseline', avg[base], median[base],
                                               stddev[base], mn[base],
                                               mx[base], float(avg[base]/avg[base]))
        pth = 'pthread_' + base
        print "%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f" % (base, 'pthread', avg[pth], median[pth],
                                               stddev[pth], mn[pth],
                                               mx[pth],float(avg[base]/avg[pth]))
        gpu = 'gpu_' + base
        if gpu == 'gpu_regex' or gpu == 'gpu_crf':
            continue
        print "%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f" % (base, 'gpu', avg[gpu], median[gpu],
                                               stddev[gpu], mn[gpu],
                                               mx[gpu], float(avg[base]/avg[gpu]))

if __name__=='__main__':
    sys.exit(main(sys.argv))
