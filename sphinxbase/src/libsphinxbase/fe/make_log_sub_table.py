#!/usr/bin/env python

import math

radix = 8
scale = 1<<radix
out_radix = 10
out_scale = 1<<out_radix

byx = 0.
logtab = []
while True:
    if byx < 1:
         lobyx = (math.log(-math.expm1(-(byx + 1.)/scale)) - math.log((byx + 1.)/scale)) / 2.0
    else:
         lobyx = (math.log(-math.expm1(-byx/scale)) - math.log(byx/scale) +
                  math.log(-math.expm1(-(byx + 1.)/scale)) - math.log((byx + 1.)/scale)) / 2.0
    lobyx = -lobyx * out_scale
    k = int(round(lobyx))
    logtab.append(k)

    if byx > 0:
        stop = int(round(math.log(-math.expm1(-byx/scale)) * out_scale))
        if stop == 0:
            break

    byx = byx + 1.

print "static const uint16 logsub_table[] = {"
for i in range(0,len(logtab),10):
    if i+10 <= len(logtab):
        print ", ".join(str(x) for x in logtab[i:i+10]) + ","
    else:
        print ", ".join(str(x) for x in logtab[i:])
print "};"
