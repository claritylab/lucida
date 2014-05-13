#!/usr/bin/env python

import math

def q15sqrt(x):
    return int(32768. * math.sqrt(x / 32768.))

def remainder_table():
    table = []
    for x in range(32, 128):
        log2 = math.log(x) / math.log(2)
        int_rem = x - 32
        factor = (log2 - 5) / 2
        table.append(int((2**factor - 1) * 32768))
    return table

remtab = remainder_table()
def fixq15sqrt(x):
    # 0 and 1 aren't handled by code below
    if x == 0:
        return 0
    elif x == 1:
        return 181
    log2 = math.log(x) / math.log(2)
    # Nearest odd log2 ("exponent" of x)
    nearest_odd_log2 = int(log2)
    if not nearest_odd_log2 & 1:
        nearest_odd_log2 -= 1
    # "Base" of square root
    base = 1<<(8 + nearest_odd_log2 / 2)
    # Index into remtab
    int_rem = x - (1<<nearest_odd_log2)
    # Scale it to fit remtab
    scale = (1<<(nearest_odd_log2 + 2)) - (1<<nearest_odd_log2)
    int_rem = int_rem * len(remtab) / scale
    return base + int(base * remtab[int_rem] / 32768)

if __name__ == '__main__':
    print "#define REMTAB_SIZE %d" % len(remtab)
    print "static const guint16 remtab[REMTAB_SIZE] = {"
    print ", ".join([str(x) for x in remtab])
    print "};"

