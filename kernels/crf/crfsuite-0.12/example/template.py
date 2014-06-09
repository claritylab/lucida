#!/usr/bin/env python

import re
import sys

class FeatureExtractor:
    def __init__(self):
        self.macro = re.compile(r'%x\[(?P<row>[\d-]+),(?P<col>[\d]+)\]')
        self.inst = []
        self.t = 0
        self.templates = []

    def read(self, fi):
        self.templates = []
        for line in fi:
            line = line.strip()
            if line.startswith('#'):
                continue
            if line.startswith('U'):
                self.templates.append(line.replace(':', '='))
            elif line == 'B':
                continue
            elif line.startswith('B'):
                sys.stderr(
                    'ERROR: bigram templates not supported: %s\n' % line)
                sys.exit(1)

    def replace(self, m):
        row = self.t + int(m.group('row'))
        col = int(m.group('col'))
        if row in range(0, len(self.inst)):
            return self.inst[row]['x'][col]
        else:
            return ''

    def apply(self, inst, t):
	self.inst = inst
	self.t = t
        for template in self.templates:
            f = re.sub(self.macro, self.replace, template)
            self.inst[t]['F'].append(f)

def readiter(fi, sep=None):
    X = []
    for line in fi:
        line = line.strip('\n')
        if not line:
            yield X
            X = []
        else:
            fields = line.split(sep)
            item = {
                'x': fields[0:-1],
                'y': fields[-1],
                'F': []
                }
            X.append(item)

if __name__ == '__main__':
    import optparse

    fi = sys.stdin
    fo = sys.stdout

    # Parse the command-line arguments.
    parser = optparse.OptionParser(usage="""usage: %prog <template>
This utility reads a data set from STDIN, applies feature templates compatible
with CRF++, and outputs attributes to STDOUT. Each line of a data set must
consist of field values separated by SEPARATOR characters (customizable with
-s option)."""
        )
    parser.add_option(
        '-s', dest='separator', default='\t',
        help='specify the separator of columns of input data [default: "\\t"]'
        )
    (options, args) = parser.parse_args()

    F = FeatureExtractor()
    F.read(open(args[0]))

    for inst in readiter(fi, options.separator):
        for t in range(len(inst)):
            F.apply(inst, t)
            fo.write('%s' % inst[t]['y'])
            for attr in inst[t]['F']:
                fo.write('\t%s' % attr.replace(':', '__COLON__'))
            fo.write('\n')
        fo.write('\n')
