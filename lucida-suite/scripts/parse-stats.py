#!/usr/bin/env python

import sys, os, re, subprocess, json
from collections import defaultdict
import numpy as np

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

def main( args ):
    if len(args) < 2:
        print "Usage: ./parse-stats.py <.json file>"
        return

    f = args[1]
    shcmd('./json-format.sh %s' % f)
    data = json.loads(open(f).read())

    kernels = [ 'fe', 'fd', 'gmm', 'dnn-asr', 'regex', 'stemmer', 'crf']
    platforms = [ 'baseline', 'pthread' ]

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

    print 'kernel,mean,median,stddev,min,max,mean-pth,median-pth,stddev-pth,min-pth,max-pth,speedup'
    for base in kernels:
        pth = 'pthread_' + base
        print "%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f" % \
                                               (base, avg[base], median[base],
                                               stddev[base], mn[base], mx[base],
                                               avg[pth], median[pth],
                                               stddev[pth], mn[pth],
                                               mx[pth], float(avg[base]/avg[pth]))

if __name__=='__main__':
    sys.exit(main(sys.argv))
