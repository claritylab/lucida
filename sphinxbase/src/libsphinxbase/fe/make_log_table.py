#!/usr/bin/env python

import math

radix = 8
scale = 1<<radix

byx = 1.
logtab = []
while True:
    lobyx = math.log(1. + byx)
    k = int(round(lobyx * scale))
    logtab.append(k)
    if k == 0:
        break
    byx *= math.exp(-1./scale)

print "static const unsigned char logadd_table[] = {"
for i in range(0,len(logtab),10):
    if i+10 <= len(logtab):
        print ", ".join(str(x) for x in logtab[i:i+10]) + ","
    else:
        print ", ".join(str(x) for x in logtab[i:])
print "};"
