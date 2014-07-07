#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
import sys
sys.path.append("../Bliss")
from optparse import OptionParser
from xmlWriterLib import openXml, closeXml, XmlWriter
from miscLib import uopen, uclose
from string import whitespace
from os.path import exists

class CartConverter(object):

	def __init__(self, pathToOldTree, pathToNewTree):
		""" initializes CartConverter-object
		arguments: paths to the cart tree files
		several default values are set:
		number of hmm-states = 3
		boundary style = super-pos-dep
		encoding of old file = ascii
		encoding of new file = utf-8
		silence token = si
		boundary token = #
		possible _contexts = history[0], central, future[0]
		"""
		self._pathToOldTree = pathToOldTree
		self._pathToNewTree = pathToNewTree

		# setting default values

		self._nHmmStates = 3
		self._boundaryStyle = "superPosDep"
		self._oldEncoding="ascii"
		self._newEncoding="utf-8"
		if self._boundaryStyle == "superPosDep":
			self._positions = ["single-phoneme-lemma",  "begin-of-lemma", "end-of-lemma"]
		elif self._boundaryStyle == "nPosDep":
			self._positions = []
		elif self._boundaryStyle == "posDep":
			self._positions == ["position-word-boundary"]

		self._silenceToken = "si"
		self._boundaryToken = "#"

		self._contexts =  ["history[0]", "central", "future[0]"]

		# initialization
		self._writer = None
		self._reader = None
		self._phonemes = []
		self._nQuestions = 0
		self._nOldQuestions = 0
		self._silenceIdx = 0
		self._boundaryIdx = 0
		self._indexMap = {}
		self._maxIndexOfCartClasses = 0

	def convert(self):
		""" reads the old cart tree and converts it to the new xml-format
		"""
		self._writer = openXml(self._pathToNewTree, self._newEncoding)
		self._setNumberOfCartClasses()
		self._reader = uopen(self._pathToOldTree)
		self._writer.open("decision-tree")
		self._convertPhonemeList()
		self._convertQuestionList()
		self._convertBinaryTree()
		self._writer.close("decision-tree")
		closeXml(self._writer)
		uclose(self._reader)
		#self._show()

	def _setNumberOfCartClasses(self):
		""" reads old cart file and determines the maximal index of a cart class
		    this is needed, because the index of the silence class is set to this value """
		self._reader = uopen(self._pathToOldTree)
		reachedTree = False
		while True:
			text = self._reader.readline().strip()
			if not reachedTree:
				if len(text) > 0:
					if text[0] == '(':
						reachedTree = True
			elif text == '':
				break
			else:
				text = text.strip("()")
				tupel = map(int, text.split(','))
				if len(tupel) == 2:
					self._maxIndexOfCartClasses = max(self._maxIndexOfCartClasses, tupel[0])
		uclose(self._reader)

	def _convertPhonemeList(self):
		""" reads phoneme list of old file and writes it to new file """
		self._writer.open("properties-definition")

		# hmm-states
		self._writeKey("hmm-state")
		self.__writeValueMap(map(str, range(self._nHmmStates)))

		# boundary
		self._writeKey("boundary")
		self.__writeValueMap(["begin-of-lemma", "end-of-lemma", "single-phoneme-lemma", "within-lemma"])

		# Phonemes
		self._readPhonemeList()
		self._silenceIdx = self._phonemes.index(self._silenceToken)
		self._boundaryIdx = self._phonemes.index(self._boundaryToken)

		assert len(self._phonemes) != 0
		for context in self._contexts:
			self._writeKey(context)
			self.__writeValueMap(self._phonemes)

		self._writer.close("properties-definition")

	def _readPhonemeList(self):
		""" reads phoneme list of old file and saves it in self._phonemes """
		while True:
			text = self._reader.readline().strip()
			if text == '':
				break
			else:
				# format check: phoneme token must not contain any spaces
				if text.find(' ') != -1:
					raise FormatError("phoneme token", text)
				else:
					self._phonemes.append(text)

	def _writeKey(self, nameOfKey):
		""" writes a key entry to the new file
		    nameOfKey: String
		    <key>nameOfKey<\\key>
		"""
		self._writer.element("key", nameOfKey)

	def _writeValue(self, value, theId=None):
		""" writes a value entry to the new file
		    value: string
		    theId: string
		    theId != None: adds the attribute id=theId
		    <value id=theId>value<\\value>
		"""
		if theId == None:
			self._writer.element("value", value)
		else:
			self._writer.element("value", value, id=theId)

	def __writeValueMap(self, values):
		""" writes a value map with values given in argument 'values' to the new file
		    values: list of strings
		    <value-map>
			<value id="1">values[0]</value>
		    </value-map>
		"""

		self._writer.open("value-map")
		for n in range(len(values)):
			self._writeValue(values[n], n)
		self._writer.close("value-map")

	def _convertQuestionList(self):
		""" reads question list from the old file and writes it to the new file
		    additional question according to the implementation of DecisionTree-legacy.c are written
		"""
		self.oldQuestionIdx = -1

		self._writer.open("questions")

		# silence-question
		self._writeQuestion("silence", "central", self._silenceToken, -1)

		# skip empty lines and list of triphones
		while True:
			questionStr = self._reader.readline()
			if questionStr.strip() != '' and questionStr.strip().find(' ') != -1:
				break

		# read questions from old cart tree and convert them to the new format
		while questionStr.strip() != '':
			self._nOldQuestions += 1
			self.oldQuestionIdx += 1
			try:
				(qName, qSet) = questionStr.split(" ", 1)
			except ValueError:
				raise FormatError("question specifier", questionStr)

			for context in self._contexts:
				self._writeQuestion(qName, context, qSet.strip(), self.oldQuestionIdx)
			questionStr = self._reader.readline()

		assert self._nOldQuestions != 0

		# hmm-state-questions
		for s in map(str, range(self._nHmmStates)):
			self.oldQuestionIdx += 1
			self._writeQuestion("hmm-state", "hmm-state", s, self.oldQuestionIdx)

		# boundary questions
		for position in self._positions:
			self.oldQuestionIdx += 1
			self._writeQuestion("boundary", "boundary", position, self.oldQuestionIdx)

		# single-phoneme questions (without silence and boundary symbol)
		for phoneme in self._phonemes:
			if phoneme != self._silenceToken and phoneme != self._boundaryToken:
				self.oldQuestionIdx += 1
				for context in self._contexts:
					self._writeQuestion("phone", context, phoneme, self.oldQuestionIdx)

		self._writer.close("questions")

	def _writeQuestion(self, qName, nameOfKey, qSet, oldQuestionIdx):
		""" writes a question to the new file and adds an entry to the index map self._indexMap
		    qName: string, name of the question
		    nameofKey: string, name of the key, e.g. central
		    qSet: string
		    oldQuestionIdx: integer, index of the question in the old format
		"""
		self._writer.open("question", description=qName)
		self._writeKey(nameOfKey)
		self._writeValue(qSet)
		self._writer.close("question")
		# create entry in self._indexMap
		try: # nameOfKey is entry of self._contexts
			context = self._contexts.index(nameOfKey) - 1
			self._indexMap[(oldQuestionIdx, context)] = self._nQuestions
		except ValueError: # not in list
			for i in range(len(self._contexts)):
				self._indexMap[(oldQuestionIdx, i)] = self._nQuestions
		self._nQuestions += 1

	def _convertBinaryTree(self):
		""" reads old tree and writes converted tree to the new file """
		self._writer.open("binary-tree")

		# skip empty lines
		while True:
			nodeStr = self._reader.readline().strip(whitespace + "()")
			if nodeStr != '':
				break

		self._writeRoot(nodeStr)

		self._writer.close("binary-tree")

	def _writeRoot(self, nodeStr):
		""" writes root of the new tree
		    nodeStr: string, containing the first line of the definition of the old tree
		"""
		self._writer.open("node", id="0")
		self._writeInformation(0)
		self._writer.open("node", id=self._maxIndexOfCartClasses)
		self._writeInformation(1)
		self._writer.close("node")
		self._writeSubTree(nodeStr)
		self._writer.close("node")

	def _writeInformation(self, order):
		""" writes information  attribute (order, size, score) to the new tree
		    since size and score of a node are not saved in the old format, dummy values are
		    given to them
		    order: integer
		"""
		self._writer.open("information")
		self._writer.element("order", str(order))
		self._writer.element("size", "0")
		self._writer.element("score", "0")
		self._writer.close("information")

	def _writeSubTree(self, nodeStr=None):
		""" writes a subtree to the new file
		    if nodeStr is not given, reads a line from the old file
		"""
		if nodeStr == None:
			nodeStr = self._reader.readline().strip(whitespace + "()")
		elif len(nodeStr) == 0:
			return
		try:
			tupel = map(int, nodeStr.split(','))
		except ValueError:
			raise FormatError("tree entry", nodeStr)

		# Leaf
		if len(tupel) == 2:
			self._writer.open("node", id=str(tupel[0] - 1))
			self._writeInformation(tupel[1])
			self._writer.close("node")
			return
		# Node
		elif len(tupel) == 3:
			try:
				qId = str(self._indexMap[(tupel[0], tupel[1])])
			except KeyError:
				indexStr = "\nentries of question index map:\n" + repr(self._indexMap) + "\n"
				raise FormatError("valid tree node", nodeStr + indexStr)
			self._writer.open("node", id=qId)
			self._writeInformation(tupel[2])
			self._writeSubTree()
			self._writeSubTree()
			self._writer.close("node")
			return
		else:
			raise FormatError("tree entry", nodeStr)

	def _show(self):
		 print "pathToOldTree " + repr(self._pathToOldTree )
		 print "pathToNewTree " + repr(self._pathToNewTree )
		 print "nHmmStates " + repr(self._nHmmStates )
		 print "boundaryStyle " + repr(self._boundaryStyle )
		 print "oldEncoding" + repr(self._oldEncoding)
		 print "newEncoding" + repr(self._newEncoding)
		 print "silenceToken " + repr(self._silenceToken )
		 print "boundaryToken " + repr(self._boundaryToken )
		 print "contexts " + repr(self._contexts )
		 print "writer " + repr(self._writer )
		 print "reader " + repr(self._reader )
		 print "phonemes " + repr(self._phonemes )
		 print "nQuestions " + repr(self._nQuestions )
		 print "nOldQuestions " + repr(self._nOldQuestions )
		 print "silenceIdx " + repr(self._silenceIdx )
		 print "boundaryIdx " + repr(self._boundaryIdx )
		 print "indexMap " + repr(self._indexMap )


class FormatError(Exception):

	def __init__(self, expected, found):
		self.message = "\nFormat error: expected " + expected + "\nfound: " + found

	def __str__(self):
		return self.message


if __name__ == '__main__':
	#  parse options
	parser = OptionParser()
	parser.add_option("-o", "--oldCartTree", dest="pathToOldTree", help="path to the old cart-tree-file", metavar="FILE")
	parser.add_option("-n", "--newCartTree", dest="pathToNewTree", help="path to the new cart-tree-file", metavar="FILE")
	(options, args) = parser.parse_args()

	if options.pathToOldTree == None:
		print "path to old cart file not given"
		parser.print_help()
		exit()
	elif not exists(options.pathToOldTree):
		print "path to old cart tree not valid"
		exit
	if options.pathToNewTree == None:
		pathToNewTree = options.pathToOldTree + ".xml"
	else:
		pathToNewTree = options.pathToNewTree

	# create converter and convert the tree
	converter = CartConverter(options.pathToOldTree, pathToNewTree)
	converter.convert()
