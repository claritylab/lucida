#!/bin/python
import string
import sys
import os

wordLimit = string.atoi(sys.argv[1])
input = open(sys.argv[2], 'r')
output= open(sys.argv[3], 'w')

lines = input.readlines()
noOfLines = len(lines)

somelines = []

for i in range(0, noOfLines):
	#print lines[i]
	somelines = string.strip(lines[i])
	somelines = string.split(lines[i])

	noOfWords = len(somelines)
	if( noOfWords > wordLimit ):
		#print noOfWords
		#print wordLimit
		print "WARNING: LINE %s HAD TO BE FOLDED IN %s\n" % (i, input)
		count = noOfWords/wordLimit

		for j in range(0, count+1):
			start = j*wordLimit;
			if (((j*wordLimit)+(wordLimit-1)) < noOfWords):
				end = ((j*wordLimit)+(wordLimit))
			else:
				end = noOfWords
			
			for k in range(start, end):
				output.write(somelines[k])
				output.write(" ")
				
			output.write('\012')
	else:
		output.write(lines[i])

