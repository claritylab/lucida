#!/bin/python
import os
import sys
import string

fileOneName = sys.argv[1]
fileJoinedName = sys.argv[2]
seperator = sys.argv[3]

fileOneFile = open(fileOneName, "r")
fileJoinedFile = open(fileJoinedName, "w")

fileOneLines = fileOneFile.readlines()
fileJoinedLines = []

i=0
joinedString = string.strip(fileOneLines[0])
for i in range( 1, len(fileOneLines) ):
	joinedString = "%s%s%s" % (joinedString, seperator, string.strip(fileOneLines[i]))

fileJoinedLines.append(joinedString)
fileJoinedFile.writelines(fileJoinedLines)
					
