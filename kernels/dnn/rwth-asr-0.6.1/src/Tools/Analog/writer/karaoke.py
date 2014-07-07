"""
Analog Plug-in for creating karaoke format
"""

__version__   = '$Revision: 6911 $'
__date__      = '$Date: 2008-10-30 15:35:27 +0100 (Thu, 30 Oct 2008) $'


import os, sys, string, tempfile
from analog import Writer

class KaraokeWriter(Writer):
    id = 'karaoke'
    defaultPostfix = '.k'

    nullToken = "---"
    infinityTime = 1e+10

    def __init__(self, options):
	super(KaraokeWriter, self).__init__(options)
	self.frameShift = float(options.frameShift)
	self.suppressDeletions = options.karaokeSuppressDeletions
	self.comparePostfix = options.karaokeComparePostfix
	self.reset()

    def reset(self):
	self.audio = None
	self.traceback = None
	self.startTime = None
	self.minStartTime = None
	self.maxEndTime = None

    def convertAudio(self, groupName):
	outputFilename = groupName + ".wav"

	if self.audio.rfind(".sph") == len(self.audio) - len(".sph"):
	    self.convertNistSphereToWav(self.audio, outputFilename,
					self.minStartTime, self.maxEndTime - self.minStartTime)
	else:
	    print >> sys.stderr, "warning: conversion from %s to wav format not implemeted yet." % (self.audio)

	return os.path.basename(outputFilename)

    def convertNistSphereToWav(self, inputFilename, outputFilename, startTime, length):
	tmpFile = tempfile.NamedTemporaryFile()
	commandDecode = "w_decode -opcm %s - " % (inputFilename)
	commandConvertToWav = "sox -t sph %s -t wav %s" % (tmpFile.name, outputFilename)
	extensionTrim = " trim %.1f" % (startTime)
	if self.maxEndTime < self.infinityTime:
	    extensionTrim += " %.1f" % (length)

	# Workaround: if results are piped instead of using tempfile, unclear error messages appear.
	decoder = os.popen2(commandDecode)
	for data in decoder[1]: tmpFile.write(data)
	os.system(commandConvertToWav + extensionTrim)

    def formatLine(self, startFrameIndex, nextStartFrameIndex, alignmentElement):
	result = None
	relativeStartTime = self.startTime - self.minStartTime
	if alignmentElement[1] != self.nullToken:
	    result = str(int((relativeStartTime + float(startFrameIndex) * self.frameShift) * 1000.0)) + ' '
	    result += alignmentElement[1] + " "
	    if alignmentElement[1] == alignmentElement[0]:
		result += "0" # correct recognized
	    elif alignmentElement[0] == self.nullToken:
		result += "2" # insertion
	    else:
		result += "1" # substitution
	elif alignmentElement[0] != self.nullToken:
	    result = str(int((relativeStartTime + float(nextStartFrameIndex) * self.frameShift) * 1000.0)) + ' '
	    if not self.suppressDeletions: result += alignmentElement[0] + " "
	    else: result += "_" + " "
	    result += "3" # deletion
	return result

    def findTracebackElement(self, fromElement, syntacticToken):
	while self.traceback[fromElement][2] != syntacticToken:
	    fromElement += 1
	    if fromElement == len(self.traceback):
		print >> sys.stderr, "Evaluation token not found in alignment."
		return None
	return fromElement

    def updateAudio(self, newAudio):
	if not self.audio:
	    self.audio = newAudio
	elif self.audio != newAudio:
	    print >> sys.stderr, "More audio files found in the group. Karaoke file can contain only one."
	    return False
	return True

    def updateTraceback(self, newTraceback):
	self.traceback = newTraceback

    def updateStartTime(self, newTime):
	self.startTime = newTime

    def updateMinMaxTimes(self, data):
	self.minStartTime = min([ record['start'] for record in data ])
	self.maxEndTime = max([ record['end'] for record in data ])

    def createLines(self, editDistanceAlignment):
	result = []
	tracebackIndex = 0
	for alignmentElement in editDistanceAlignment:
	    if alignmentElement[1] != self.nullToken:
		tracebackIndex = self.findTracebackElement(tracebackIndex + 1, alignmentElement[1])
		if not tracebackIndex: return False

	    startFrameIndex = int(self.traceback[max(tracebackIndex - 1, 0)][0])
	    nextStartFrameIndex = int(self.traceback[max(tracebackIndex, 0)][0])
	    line = self.formatLine(startFrameIndex, nextStartFrameIndex, alignmentElement)
	    if line: result += [ line ]
	return result

    def write(self, file, lines):
	print >> file, "karaoke"
	groupName = file.name[0:len(file.name) - len(self.postfix)]
	audioFilename = self.convertAudio(groupName)
	if self.comparePostfix: audioFilename += " " + os.path.basename(groupName + self.comparePostfix)
	print >> file, audioFilename

	startTimes = lines.keys()
	startTimes.sort()
	for startTime in startTimes:
	    for line in lines[startTime]:
		print >> file, line


    def __call__(self, file, data):
	self.reset()
	lines = {}
	self.updateMinMaxTimes(data)
	for record in data:
	    if not self.updateAudio(record['audio']):
		raise ValueError()
	    self.updateStartTime(record['start'])
	    self.updateTraceback(record['traceback'])
	    lines[self.startTime] = self.createLines(record['edit distance alignment'])
	self.write(file, lines)
