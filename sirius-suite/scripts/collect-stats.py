#!/usr/bin/env python

import sys, os, re, subprocess

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
        if plat == 'pthread':
            cmd = './surf-fe ' + str(threads) + ' ../input/2048x2048.jpg'
        else:
            cmd = './surf-fe ../input/2048x2048.jpg'
    elif k == 'fd':
        if plat == 'pthread':
            cmd = './surf-fd ' + str(threads) + ' ../input/2048x2048.jpg'
        else:
            cmd = './surf-fd ../input/2048x2048.jpg'
    elif k == 'gmm':
        if plat == 'pthread':
            cmd = './gmm_scoring ' + str(threads) + ' ../input/gmm_data.txt'
        else:
            cmd = './gmm_scoring ../input/gmm_data.txt'
    elif k == 'regex':
        if plat == 'pthread':
            cmd = './regex_slre ' + str(threads) + ' ../input/list ../input/questions'
        else:
            cmd = './regex_slre ../input/list ../input/questions'
    elif k == 'stemmer':
        if plat == 'pthread':
            cmd = './stem_porter ' + str(threads) + ' ../input/voc-1M.txt'
        else:
            cmd = './stem_porter ../input/voc-1M.txt'
    elif k == 'crf':
        cmd = './test-crf.sh'
    elif k == 'dnn-asr':
        if plat == 'pthread':
            cmd = './surf-fd ' + str(threads) + ' ../input/2048x2048.jpg'
        else:
            cmd = './surf-fd ../input/2048x2048.jpg'

    shcmd(cmd)
        
def main( args ):
    if len(args) < 3:
        print "Usage: ./collect-stats.py <top-directory of kernels> <# of runs>"
        return

    kernels = [ 'fe', 'fd', 'gmm', 'regex', 'stemmer', 'crf', 'dnn-asr']
    platforms = [ 'baseline', 'pthread', 'gpu' ]

    # top directory of kernels
    kdir = args[1]
    os.chdir(kdir)

    # how many times to run each kernel
    LOOP = int(args[2])

    # remove GPU if no NVCC installed
    if shcom("which nvcc") == "":
        platforms = [ 'baseline', 'pthread']

    # for each kernel and platform.
    # uses 'make test' input and config for each kernel
    root = os.getcwd()
    for k in kernels:
        d = os.getcwd() + '/' + k
        os.chdir(d)
        kroot = os.getcwd() 
        for plat in platforms:
            if not os.path.isdir(plat):
                continue
            os.chdir(plat)
            for i in range(1, LOOP):
                shcmd('make test')
            os.chdir(kroot)
        os.chdir(root)

if __name__=='__main__':
    sys.exit(main(sys.argv))
