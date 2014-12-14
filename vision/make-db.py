#!/usr/bin/python

import pickledb
import subprocess, re, os, sys

if __name__ == '__main__':
    pwd = os.getcwd()
    db_name = sys.argv[1]
    ddb = 'matching/%s/db' % db_name
    pickdb = pickledb.load(db_name+'.db', False)

    imgs = os.listdir(ddb)
    for i in imgs:
        # remove extension and tokenize
        data = os.path.splitext(i)[0].split('-')
        text = ' '.join(data)
        print text
        pickdb.set(i, data)
    
    pickdb.dump()
