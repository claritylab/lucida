#!/bin/python
from Iterators import iterfile, itergroup
from ParseTree import ParseTreeFactory
from optparse import OptionParser
import re
import sys
import string
import sets

class WordFeatureExtractor:

    def __init__(self, words):
        self.words = words

    def getWordSet(self, words, punctuationMatcher=re.compile(r'[%s]' % string.punctuation)):
        return sets.Set(words + [punctuationMatcher.sub('', word) for word in words])

    def getParseWords(self, begin, end, semanticParse,
                      textMatcher = re.compile(r'<S TPOS="[^"]*">(.*?)</S>'),
                      sgmlMatcher = re.compile(r'</?[A-z]+[^>]*>')):
        text, = textMatcher.findall(semanticParse)
        return sgmlMatcher.sub('', text).split()[begin:end]

    def getFeatureValues(self, begin, end, semanticParse, syntacticParse):
        parseWordsSet = self.getWordSet(self.getParseWords(begin, end, semanticParse))
        return [int(word in parseWordsSet) for word in self.words]

class WordCountFeatureExtractor(WordFeatureExtractor):
    def __init__(self, wordsFileName):
        WordFeatureExtractor.__init__(self, wordsFileName)
        self.wordSet = self.getWordSet(self.words)
    
    def getFeatureValues(self, begin, end, semanticParse, syntacticParse):
        count = 0
        for word in self.getParseWords(begin, end, semanticParse):
            if word in self.wordSet:
                count += 1
        return [count]

class WordScoreSumFeatureExtractor(WordFeatureExtractor):
    def __init__(self, wordScoreMap):
        self.wordScoreMap = wordScoreMap

    def getFeatureValues(self, begin, end, semanticParse, syntacticParse):
        parseWords = self.getWordSet(self.getParseWords(begin, end, semanticParse))
        score = 0.0
        for word in parseWords:
            score += self.wordScoreMap.get(word, 0.0)
        return [int(round(score))]

class POSWordScoreSumFeatureExtractor(WordFeatureExtractor):
    partOfSpeechMatcher = re.compile(r'[(](\S*)\s+\S+[)]')

    def __init__(self, posPrefix, wordScoreMap):
        self.posPrefix = posPrefix
        self.wordScoreMap = wordScoreMap
    
    def getFeatureValues(self, begin, end, semanticParse, syntacticParse):
        partsOfSpeech = self.partOfSpeechMatcher.findall(syntacticParse)[begin:end]
        parseWords = self.getParseWords(begin, end, semanticParse)

        try:
            assert len(parseWords) == len(partsOfSpeech)
        except:
            print >> sys.stderr, parseWords
            print >> sys.stderr, partsOfSpeech
            raise

        parseWords = self.getWordSet(parseWords)

        score = 0.0
        for word, pos in zip(parseWords, partsOfSpeech):
            if pos.startswith(self.posPrefix):
                score += self.wordScoreMap.get(word, 0.0)
        return [int(round(score))]

class PhraseTypeFeatureExtractor:
    def __init__(self, phraseType):
        self.phraseType = phraseType
        self.parseTreeMap = {}

    def getParseTree(self, syntacticParse):
        if not self.parseTreeMap.has_key(syntacticParse):
            self.parseTreeMap[syntacticParse] = ParseTreeFactory.fromString(syntacticParse)
        return self.parseTreeMap[syntacticParse]

    def getParsePhraseTypes(self, begin, end, syntacticParse):
        tree = self.getParseTree(syntacticParse)
        for subtree in tree[begin:end]:
            for childSubtree in subtree.subtrees():
                yield childSubtree.tag

    def getFeatureValues(self, begin, end, semanticParse, syntacticParse):
        parsePOSSet = sets.Set(self.getParsePhraseTypes(begin, end, syntacticParse))
        return [int(self.phraseType in parsePOSSet)]

if __name__ == "__main__":
    # parse the command line arguments
    optionParser = OptionParser(usage='%prog [options] sameerFile svmFile')
    optionParser.add_option('-w', '--words', metavar='WORDS_FILE', action='append', dest='wordFileNames', default=[],
                            help='Append a binary feature for each word in the words file indicating whether or not the constituent contains that word.')
    optionParser.add_option('-c', '--count', metavar='WORDS_FILE', action='append', dest='wordCountFileNames', default=[],
                            help='Append a feature that is the count of words in the constituent occurring in the words file.')
    optionParser.add_option('-s', '--word-score-sum', metavar='WORD_SCORES_FILE', action='append', dest='wordScoreSumFileNames', default=[],
                            help='Append a feature that is the sum of the scores from the file for each word in the constituent (discretized into integers).')
    optionParser.add_option('-p', '--pos-word-score-sum', metavar='POS:WORD_SCORES_FILE', action='append', dest='posPrefixWordScoresFilePairs', default=[],
                            help='Append a feature that is the sum of the scores from the file for each word in the constituent whose part of speech has the given prefix (discretized into integers).')
    optionParser.add_option('-P', '--phrase-type', metavar='PHRASE_TYPE', action='append', dest='phraseTypes', default=[],
                            help='Append a binary feature indicating that the constituent contains this syntactic-phrase-type.')
    options, args = optionParser.parse_args()

    # extract the file names
    if len(args) != 2:
        optionParser.error('wrong number of arguments')
    sameerFileName, svmFileName = args

    # a function for turning a word file into a list of words
    def getWords(wordFile):
        words = []
        for line in iterfile(wordFile):
            columns = line.split()
            words.append(columns[0])
        return words

    # a function for turning a word-scores file into a mapping from word to score
    def getWordScoreMap(wordScoreFile):
        wordScoreMap = {}
        for line in iterfile(wordScoreFile):
            columns = line.split()
            word, score = columns[0], columns[-1]
            score = float(score)
            if not wordScoreMap.has_key(word) or score > wordScoreMap[word]:
                wordScoreMap[word] = score
        return wordScoreMap

    # assemble a list of feature extractors
    extractors = []
    for wordFileName in options.wordFileNames:
        extractors.append(WordFeatureExtractor(getWords(wordFileName)))
    for wordCountFileName in options.wordCountFileNames:
        extractors.append(WordCountFeatureExtractor(getWords(wordCountFileName)))
    for wordScoreSumFileName in options.wordScoreSumFileNames:
        extractors.append(WordScoreSumFeatureExtractor(getWordScoreMap(wordScoreSumFileName)))
    for posPrefixWordScoresFilePair in options.posPrefixWordScoresFilePairs:
        posPrefix, wordScoresFileName = posPrefixWordScoresFilePair.split(':')
        extractors.append(POSWordScoreSumFeatureExtractor(posPrefix, getWordScoreMap(wordScoresFileName)))
    for phraseType in options.phraseTypes:
        extractors.append(PhraseTypeFeatureExtractor(phraseType))
        
    # build a map from parse id to the list of words in the parse
    idMatcher = re.compile(r'<S TPOS="([^"]*)">')
    idSemanticParseMap = {}
    idSyntacticParseMap = {}
    for semanticParse, syntacticParse, neLine, blankLine in itergroup(iterfile(sameerFileName), 4, ''):
        id, = idMatcher.findall(semanticParse)
        idSemanticParseMap[id] = semanticParse
        idSyntacticParseMap[id] = syntacticParse

    for line in iterfile(svmFileName):
		if not line:
			print line
			continue
		
		columns = line.split()
		id, begin, end, classVar = columns[0], columns[2], columns[3], columns[-1]

		#if( columns[7] == "a" ):
		#	columns[7] = "after"
		#else:
		#	columns[7] = "before"
		#
		#
		#if( columns[11] == "a" ):
		#	columns[11] = "ACT"
		#else:
		#	columns[11] = "PASS"
			
		begin, end = int(begin), int(end) + 1
		if not idSemanticParseMap.has_key(id):
			raise Exception, 'no parse for id "%s"' % id
		semanticParse, syntacticParse = idSemanticParseMap[id], idSyntacticParseMap[id]
        
		print ' '.join(columns[:-1]),
		for extractor in extractors:
			values = extractor.getFeatureValues(begin, end, semanticParse, syntacticParse)
			print ' '.join(map(str, values)),
		print classVar
