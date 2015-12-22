#!/usr/bin/env python

import sys, os, re, subprocess, json, csv
from collections import defaultdict
import numpy as np

def get_data(fname, metrics):
    data = {}
    with open(fname, 'rb') as f:
        d = csv.DictReader(f)
        for row in d:
            for k,v in row.iteritems():
                if any(v[0] in s for s in metrics):
                    data[v[0]] = v[1]
    
    return data

def sirius_print(data, metrics):
    for m in metrics[:-1]:
        if type(m) is str:
            sys.stdout.write('%s,' % data[m])
        else:
            sys.stdout.write('%.2f,' % data[m])
    else:
        print '%s' % data[metrics[-1]]

def main( args ):
    if len(args) < 2:
        print "Usage: ./collect-stats.py <top-directory of kernels>"
        return
    kernels = [ 'fe', 'fd', 'gmm', 'dnn-asr','stemmer', 'crf', 'regex']
    platforms = ['smt']
    header  = ['Platform', 'Kernel']
    metrics = header + ['CPI Rate', 'Front-end Bound', 'Bad Speculation', 'Retiring', 'Back-End Bound',
                        'Memory Bound', 'Core Bound', 'Back-end Sum', 'Full Sum']
    # metrics.append(header + ['L1 Bound', 'L3 Bound', 'L3 Latency', 'DRAM Bound', 'Local DRAM', 'Remote DRAM', 'Store Bound']

    # top directory of kernels
    kdir = args[1]
    os.chdir(kdir)

    # for each kernel and platform.
    root = os.getcwd()

    # print 'kernel,',
    for m in metrics[:-1]:
        sys.stdout.write('%s,' % m)
    else:
        print '%s' % metrics[-1]

    data = {}
    for k in kernels:
        d = os.getcwd() + '/' + k
        os.chdir(d)
        kroot = os.getcwd() 
        for plat in platforms:
            if plat == 'smt' or plat == 'cores':
                os.chdir('pthread')
            else:
                os.chdir(plat)
            fname = 'sirius-suite-%s.report' % plat
            if os.path.isfile(fname):
                data = get_data(fname, metrics)
                data['Kernel'] = k
                data['Platform'] = plat
                data['Back-end Sum'] = float(data['Core Bound']) + float(data['Memory Bound'])
                data['Full Sum'] = float(data['Front-end Bound']) + float(data['Bad Speculation']) \
                              + float(data['Retiring']) + float(data['Back-End Bound'])
                sirius_print(data, metrics)
            os.chdir(kroot)
        os.chdir(root)

if __name__=='__main__':
    sys.exit(main(sys.argv))
