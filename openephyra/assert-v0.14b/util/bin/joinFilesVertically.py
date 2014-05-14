#!/bin/python
import os
import sys
import string

fileOneName = sys.argv[1]
fileTwoName = sys.argv[2]
fileJoinedName = sys.argv[3]

if ( len(sys.argv) == 5 ):
	seperator = sys.argv[4]
else:
	seperator = ' '

fileOneFile = open(fileOneName, "r")
fileTwoFile = open(fileTwoName, "r")
fileJoinedFile = open(fileJoinedName, "w")

fileOneLines = fileOneFile.readlines()
fileTwoLines = fileTwoFile.readlines()
fileJoinedLines = []

i=0
for i in range( 0, len(fileOneLines) ):
	joinedString = "%s%s%s" % (string.strip(fileOneLines[i]), seperator, fileTwoLines[i]) 
	fileJoinedLines.append(joinedString)

fileJoinedFile.writelines(fileJoinedLines)
