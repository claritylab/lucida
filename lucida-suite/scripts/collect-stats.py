#!/usr/bin/env python

import sys, os, re, subprocess

threads = 2
overlap = 15

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

def shcom(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = p.communicate()[0]
    return out

def run_kernel (k, plat):
    '''To change inputs or # of threads'''
    cmd = ''
    if k == 'fe':
        inp = ' ../input/2048x2048.jpg'
        if plat == 'smt' or plat == 'cores':
            cmd = './surf-fe ' + str(threads) + ' ' + str(overlap) + inp
        else:
            cmd = './surf-fe' + inp
    elif k == 'fd':
        inp = ' ../input/2048x2048.jpg'
        if plat == 'smt' or plat == 'cores':
            cmd = './surf-fd ' + str(threads) + ' ' + str(overlap) + inp
        else:
            cmd = './surf-fd' + inp
    elif k == 'gmm':
        inp = ' 100 ../input/gmm_data.txt'
        if plat == 'smt' or plat == 'cores':
            cmd = './gmm_scoring ' + str(threads) + inp
        else:
            cmd = './gmm_scoring ' + inp
    elif k == 'regex':
        inp = ' 100 ../input/patterns.txt 10000 ../input/questions-10k.txt'
        if plat == 'smt' or plat == 'cores':
            cmd = './regex_slre ' + str(threads) + inp
        else:
            cmd = './regex_slre' + inp
    elif k == 'stemmer':
        inp = ' 50000000 ../input/voc-50M.txt'
        if plat == 'smt' or plat == 'cores':
            cmd = './stem_porter ' + str(threads) + inp
        else:
            cmd = './stem_porter' + inp
    elif k == 'crf':
        inp = ' ../input/model.la ../input/test-input.txt'
        if plat == 'smt' or plat == 'cores':
            cmd = './crf_tag ' + str(threads) + inp
        else:
            cmd = './crf_tag' + inp
    elif k == 'dnn-asr':
        inp = ' ../model/asr.prototxt ../model/asr.caffemodel ../input/features.in'
        if plat == 'smt' or plat == 'cores':
            cmd = './dnn_asr ' + str(threads) + inp
        else:
            cmd = './dnn_asr' + inp    
    return cmd
        
def main( args ):
    if len(args) < 3:
        print "Usage: ./collect-stats.py <top-directory of kernels> <# of runs>"
        return

    kernels = [ 'fe', 'fd', 'gmm', 'dnn-asr', 'regex', 'stemmer', 'crf']
    platforms = ['baseline', 'smt']

    # top directory of kernels
    kdir = args[1]
    os.chdir(kdir)

    # how many times to run each kernel
    LOOP = int(args[2])

    # for each kernel and platform.
    root = os.getcwd()
    for k in kernels:
        d = os.getcwd() + '/' + k
        os.chdir(d)
        kroot = os.getcwd() 
        for plat in platforms:
            fname = 'sirius-suite-%s' % plat
            if plat == 'smt' or plat == 'cores' or plat == 'pthread':
                os.chdir('pthread')
            else:
                os.chdir(plat)
            for i in range(0, LOOP):
                if plat == 'smt':
                    task = 'taskset -c 0,8 '
                elif plat == 'cores':
                    task = 'taskset -c 0,1 '
                else:
                    task = 'taskset -c 0 '
                cmd = task + ' ' + run_kernel(k, plat)
                shcmd(cmd)
            os.chdir(kroot)
        os.chdir(root)

if __name__=='__main__':
    sys.exit(main(sys.argv))
