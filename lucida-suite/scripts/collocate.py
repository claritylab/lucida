#!/usr/bin/env python

import sys, os, re, subprocess

threads = 4
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

def build_cmd (root, args, procs):
    built = ''
    for i in range(0, procs):
        for k,a in enumerate(args):
            if k == 0:
                built = built + './'
            if type(a) == type('str'):
                built = built + root + '/' + a + ' '
            else:
                built = built + str(a) + ' '
        if i < procs-1:
            built = built + '& '

    return built

def main( args ):
    if len(args) < 2:
        print "Usage: ./collect-stats.py <top-directory of kernels>"
        return

    kernels = {}
    kernels['fe'] = [2, 4, 6, 8]
    kernels['fd'] = [2, 4, 6, 8]
    kernels['gmm'] = [2, 4, 6, 8]
    kernels['dnn-asr'] = [2, 4, 6, 8]
    kernels['stemmer'] = [2, 4, 6, 8]
    kernels['regex'] = [2, 4, 6, 8]
    kernels['crf'] = [2, 4, 6, 8]
    platforms = ['baseline']

    # top directory of kernels
    kdir = args[1]
    os.chdir(kdir)

    # remove GPU if no NVCC installed
    # if shcom("which nvcc") == "":
    #     platforms = [ 'baseline', 'pthread']

    # for each kernel and platform.
    # task = 'taskset -c 0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15 '
    task = 'taskset -c 0,8,1,9 '
    root = os.getcwd()
    for k in kernels:
        for n in kernels[k]:
            f=open('run.sh', 'w')
            fname = 'scripts/' + k + '-' + str(n) + '.json'
            output=open(fname,'w')
            for plat in platforms:
                build = re.sub(root + '/', '', root + '/' + k + '/' + plat)
                args = get_args(k, plat)
                cmd = build_cmd(build, args, n)
            f.write(cmd)
            f.close()
            shcmd('chmod +x run.sh')
            cmd = task + './run.sh'
            # shcmd(cmd)
            out= shcom(cmd)
            output.write(out)
            output.close()

if __name__=='__main__':
    sys.exit(main(sys.argv))
