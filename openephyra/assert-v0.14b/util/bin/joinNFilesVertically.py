#!/bin/python
import os
import sys
import string

num_files_to_join = len(sys.argv) - 2
seperator = " "

file_objects = []
list_of_files_lines = []

i=0
for i in range(0, num_files_to_join):
	file_object = open(sys.argv[i+1]) 
	file_objects.append( file_object )
	list_of_files_lines.append(file_object.readlines())


fileJoinedName = sys.argv[num_files_to_join+1]
fileJoinedFile = open(fileJoinedName, "w")

fileJoinedLines = []

i=0
j=0
for i in range( 0, len(list_of_files_lines[0]) ):
	joinedString = ""
	for j in range( 0, len(list_of_files_lines) ):
		joinedString = "%s%s%s" % (joinedString, seperator, string.strip(list_of_files_lines[j][i])) 
	fileJoinedLines.append("%s\n" %(joinedString))

fileJoinedFile.writelines(fileJoinedLines)


def usage():
	print """
Usage: joinFilesVertically.py <file-1> <file-2> ... <file-n> <output-file>
"""
	
