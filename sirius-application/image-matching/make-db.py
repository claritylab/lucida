#!/usr/bin/env python

#
# Copyright (c) 2015, University of Michigan.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#
#

import pickledb
import subprocess, re, os, sys

def shcmd(cmd):
    subprocess.call(cmd, shell=True)

if __name__ == '__main__':
    pwd = os.getcwd()
    db_name = sys.argv[1]
    db_dir = sys.argv[2]

    # make protodb
    cmd = './match --build %s' % db_dir
    print cmd
    shcmd(cmd)

    # make pickledb
    pickdb = pickledb.load(db_name+'.pickle', False)
    imgs = os.listdir(db_dir)
    for i in imgs:
        if i.endswith('.jpg'):
            # remove extension and tokenize
            data = os.path.splitext(i)[0].split('-')
            text = ' '.join(data)
            print text
            pickdb.set(i, data)

    pickdb.dump()
