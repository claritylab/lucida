#!/usr/bin/python

import pickledb
import subprocess, re, os, sys

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

if __name__ == '__main__':
    pwd = os.getcwd()
    db_name = sys.argv[1]
    ddb = 'matching/%s/db' % db_name

    # make protodb
    cmd = './match --build %s' % ddb
    print cmd
    shcmd(cmd)

    # make pickledb
    pickdb = pickledb.load(db_name+'.pickle', False)
    imgs = os.listdir(ddb)
    for i in imgs:
        if i.endswith('.jpg'):
            # remove extension and tokenize
            data = os.path.splitext(i)[0].split('-')
            text = ' '.join(data)
            print text
            pickdb.set(i, data)
    
    pickdb.dump()
