#!/usr/bin/env python

import sys, re

print '<?xml version="1.0" encoding="UTF-8"?>'
print '<corpus name="AN4">'

reFile = re.compile("\(((.*)-(.*)-(.*))\)")
for line in file(sys.argv[1]):
    l = line.split()
    info = reFile.match(l[-1])
    audio = info.group(3) + "/" + info.group(1) + ".wav"
    print '  <recording audio="%s" name="%s">' % (audio, info.group(1))
    print '    <segment start="0.0" end="inf" name="%s">' % info.group(1)
    print '      <orth>'
    print ' ' * 8,
    if l[1] == "<s>":
        start = 1
    else:
        start = 0
    if l[-2] == "</s>":
        end = -2
    else:
        end = -1
    print " ".join(l[start:end])
    print '      </orth>'
    print '    </segment>'
    print '  </recording>'
print '</corpus>'


