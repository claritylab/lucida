#!/bin/python

import string
import sys
import os
import re

if( len(sys.argv) == 1 ):
	print "Usage: add-first-word-flag.py <file>"
	sys.exit(1)


in_file = open(sys.argv[1])
in_file_line = in_file.readline()
while( in_file_line != "" ):
	if( string.strip(in_file_line) == "" ):
		print
	else:
		list = string.split(in_file_line)

		flag = "0"
		if( int(list[2]) == 0 and int(list[3]) == 0 ):
			flag = "1"
		else:
			flag = "0"
		print string.join(list[0:-1]) + " " + flag + " " + list[-1]

	in_file_line = in_file.readline()

