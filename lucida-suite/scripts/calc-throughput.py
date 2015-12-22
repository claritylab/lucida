#!/usr/bin/env python

import sys, os, re, subprocess, json
from collections import defaultdict
import numpy as np

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

def main( args ):
    # if len(args) < 2:
    #     print "Usage: ./pa-stats.py <.json file>"
    #     return

    # dictionary of lists
    # dict_of_lists = defaultdict(list)

    instances = [2, 4, 6 ,8]

    kernels = {}
    # kernels['fe'] = 'rows'
    # kernels['fd'] = 'rows'
    kernels['gmm'] = 'scores'
    # kernels['dnn-asr'] = 'in_features'
    # kernels['stemmer'] = 'words'
    # kernels['regex'] = 'questions'
    # kernels['crf'] = 'sentences'
    # kernels['pthread_stemmer'] = 'words'

    # index into json and get throughput metric based on abrv
    print 'kernel,fname,instances,throughput'
    for k in kernels:
        for inst in instances:
            f = k + '-' + str(inst) + '.json'
            shcmd('./json-format.sh %s' % f)
            data = json.loads(open(f).read())
            dict_of_lists = defaultdict(list)
            for i in range(0, len(data)):
                dict_of_lists[kernels[data[i]["abrv"]]].append(data[i][kernels[data[i]["abrv"]]]) # get tp
                dict_of_lists[data[i]["abrv"]].append(data[i][data[i]["abrv"]]) # get timing
                if data[i]["abrv"] not in dict_of_lists['kernel']: # track kernel
                    dict_of_lists['kernel'].append(data[i]["abrv"])

                data = {}
                # for k, v in dict_of_lists.iteritems():
                for k in dict_of_lists['kernel']:
                    tp = []
                    cnt = 0
                    for unit,i in zip(dict_of_lists[kernels[k]], dict_of_lists[k]):
                        tp.append(float(unit/i))
                        cnt = cnt + 1
                    print '%s,%s,%d,%f' % (k,f, cnt, sum(tp))

    # for k in data['kernel']:

if __name__=='__main__':
    sys.exit(main(sys.argv))
