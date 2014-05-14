#!/bin/python

import string
import sys
import re
import os


if( len(sys.argv) == 1 ):
	print "Usage: add-pseudo-ne.py <file.fn>"
	sys.exit(1)

infile = open(sys.argv[1])

f_line = infile.readline()
p_line = infile.readline()
s_line = infile.readline()

while( f_line != "" ):
	print string.strip(f_line)
	print string.strip(p_line)

	f_line = re.sub("^.*?>", "", f_line)
	f_line = re.sub("</[CS]>", "", f_line)
	f_line = re.sub("/[^/ ]+ ", " ", f_line)
	f_line = re.sub("<C .*?>", "", f_line)
	f_line = re.sub("^ +", "", f_line)
	f_line = re.sub(" +$", "", f_line)
	f_line = re.sub(" +", " ", f_line)

	print string.strip(f_line)
	print

	f_line = infile.readline()
	p_line = infile.readline()
	s_line = infile.readline()

