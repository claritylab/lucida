#!/bin/python
import sys
import string

file_1 = open(sys.argv[1]).readlines()
file_2 = open(sys.argv[2]).readlines()

i=0
for i in range(0, len(file_1)):
	print string.strip(file_1[i])
	print string.strip(file_2[i])
	print
	
