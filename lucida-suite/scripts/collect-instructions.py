#!/usr/bin/env python

import sys, os, re, subprocess

threads = 2
overlap = 50

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

def shcom(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    out = p.communicate()[0]
    return out

def get_args (k, plat):
    '''To change inputs or # of threads'''
    args = []
    if k == 'fe':
        args.append('surf-fe')
        if plat == 'pthread':
            args.append(threads)
            args.append(overlap)
        args.append('../input/2048x2048.jpg')
    elif k == 'fd':
        args.append('surf-fd')
        if plat == 'pthread':
            args.append(int(threads))
            args.append(int(overlap))
        args.append('../input/2048x2048.jpg')
    elif k == 'gmm':
        args.append('gmm_scoring')
        if plat == 'pthread':
            args.append(threads)
        args.append(100)
        args.append('../input/gmm_data.txt')
    elif k == 'regex':
        args.append('regex_slre')
        if plat == 'pthread':
            args.append(threads)
        args.append(100)
        args.append('../input/patterns.txt')
        args.append(10000)
        args.append('../input/questions-10k.txt')
    elif k == 'stemmer':
        args.append('stem_porter')
        if plat == 'pthread':
            args.append(threads)
        args.append(50000000)
        args.append('../input/voc-50M.txt')
    elif k == 'crf':
        args.append('crf_tag')
        if plat == 'pthread':
            args.append(threads)
        args.append('../input/model.la')
        args.append('../input/test-input.txt')
    elif k == 'dnn-asr':
        args.append('dnn_asr')
        if plat == 'pthread':
            args.append(threads)
        args.append('../model/asr.prototxt')
        args.append('../model/asr.caffemodel')
        args.append('../input/features.in')

    return args

def build_cmd (root, args):
    built = ''
    for k,a in enumerate(args):
        if k == 0:
            built = built + './'
        if type(a) == type('str'):
            built = built + root + '/' + a + ' '
        else:
            built = built + str(a) + ' '

    return built
        
def main( args ):
    if len(args) < 3:
        print "Usage: ./collect-stats.py <top-directory of kernels> <report name>"
        return

    kernels = [ 'fe', 'fd', 'gmm', 'dnn-asr', 'regex', 'stemmer', 'crf']
    platforms = ['baseline']

    # top directory of kernels
    kdir = args[1]
    flag = args[2]
    os.chdir(kdir)

    # for each kernel and platform.
    task = ''
    pin = '/home/jahausw/tools/pin/pin -t '
    tool = '/home/jahausw/tools/pin/source/tools/ManualExamples/obj-intel64/basetypes.so -- '
    root = os.getcwd()
    for k in kernels:
        for plat in platforms:
            build = re.sub(root + '/', '', root + '/' + k + '/' + plat)
            args = get_args(k, plat)
            cmd = build_cmd(build, args)
            # if plat == 'smt':
            #     task = 'taskset -c 0,8 '
            # elif plat == 'cores':
            #     task = 'taskset -c 0,1 '
            # else:
            #     task = 'taskset -c 0 '
            cmd = pin + tool + task + ' ' + cmd
            shcmd(cmd)
            shcmd('mv inscount.out %s.%s' % (k, flag))

if __name__=='__main__':
    sys.exit(main(sys.argv))
