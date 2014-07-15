"""
Analog Plug-in for evaluation of open-vocabulary recognition results.

Warning: This works correctly only for the WSJ corpus.  This
implementation should be considered transitional until this function
is integrated with Bliss.
"""

__version__   = '$Revision: 8349 $'
__date__      = '$Date: 2011-08-15 12:59:09 +0200 (Mon, 15 Aug 2011) $'


import sys
from analog import Collector, Field, Mutator
from sequitur_ import align


def assembleOovWords(orth):
    s = '|'.join(orth)
    s = s.replace('_|_', '')
    s = s.replace('_', '')
    return s.split('|')


class OvPostProcessor(Mutator):
    """
    Connect fragments from OV recognition to words.
    """
    id = 'ov'

    def assemble(self, orth):
	words = orth.split()
	words = assembleOovWords(words)
	return ' '.join(words)

    def work(self, segment):
	if 'recognized' in segment:
	    segment['recognized'] = self.assemble(segment['recognized'])
	if 'traceback' in segment:
	    traceback = []
	    for time, score, orth in segment['traceback']:
		if orth and orth.startswith('_'):
		    orth = orth[1:]
		    if shouldLink:
			notime, score, prefix = traceback.pop()
			orth = prefix + orth
		if orth and orth.endswith('_'):
		    orth = orth[:-1]
		    shouldLink = True
		else:
		    shouldLink = False
		traceback.append((time, score, orth))
	    segment['traceback'] = traceback


class WsjPostProcessor(Mutator):
    """
    Do some naive mapping from orthography to evaluation.
    """
    id = 'wsj'

    mapped = {
	'%PERCENT' : 'PERCENT',
	'.POINT'   : 'POINT',
	'"QUOTE'   : 'QUOTE' }

    def shouldEvalWord(self, w):
	if w == '[UNKNOWN]' or w == '@reject@':
	    return True
	if w.startswith('[') and w.endswith(']'):
	    return False
	return True

    def convertForEval(self, orth):
	if orth is None:
	    return []
	eval = [ self.mapped.get(w, w) for w in orth.split() ]
	eval = [ w for w in eval if self.shouldEvalWord(w) ]
	return ' '.join(eval)

    def work(self, segment):
	segment['reference'] = self.convertForEval(segment['reference'])
	segment['recognized'] = self.convertForEval(segment['recognized'])


class EditDistance(object):
    """
    Simple error rate based on edit distance of orthographic result.
    The edit distance alignment from the log file is not used!
    """

    def clear(self):
	self.nInsertions = 0
	self.nDeletions = 0
	self.nSubstitutions = 0
	self.nCorrect = 0
	self.nSymbols = 0
	self.nStrings = 0

    def accu(self, reference, candidate, alignment, weight = 1):
	nInsertions = 0
	nDeletions = 0
	nSubstitutions = 0
	for ss, rr in alignment:
	    if ss is None:
		nInsertions += 1
	    elif rr is None:
		nDeletions += 1
	    elif ss != rr:
		nSubstitutions += 1
	self.nInsertions    += weight * nInsertions
	self.nDeletions     += weight * nDeletions
	self.nSubstitutions += weight * nSubstitutions
	self.nSymbols       += weight * len(reference)
	self.nStrings       += weight

    filterMode = None

    def filter(self, reference, recognized, alignment):
	isOov = [ False for al in alignment ]
	state = False
	for ii in range(len(alignment)):
	    if alignment[ii][0] is not None:
		state = alignment[ii][0] not in self.vocabulary
	    isOov[ii] = isOov[ii] or state
	state = False
	for ii in reversed(range(len(alignment))):
	    if alignment[ii][0] is not None:
		state = alignment[ii][0] not in self.vocabulary
	    isOov[ii] = isOov[ii] or state

	if self.filterMode == 'oov':
	    alignment = [ al for al, flag in zip(alignment, isOov)
			  if flag ]
	elif self.filterMode == 'iv':
	    alignment = [ al for al, flag in zip(alignment, isOov)
			  if not flag ]

	reference  = [ r for r, s in alignment if r ]
	recognized = [ s for r, s in alignment if s ]

	return reference, recognized, alignment

    def __call__(self, data):
	self.clear()
	for reference, recognized in zip(data['reference'], data['recognized']):
	    reference  = self.convertForAlignment(reference)
	    recognized = self.convertForAlignment(recognized)

	    alignment, errors = align(reference, recognized)
	    if self.filterMode:
		reference, recognized, alignment = self.filter(reference, recognized, alignment)
	    self.accu(reference, recognized, alignment)

	nErrors = self.nDeletions + self.nInsertions + self.nSubstitutions
	return zip(self.fields, [
	    self.nStrings,
	    self.nDeletions, self.nInsertions, self.nSubstitutions,
	    nErrors, self.nSymbols,
	    100.0 * nErrors / self.nSymbols ])



class LetterErrorRate(Collector, EditDistance):
    id     = 'orth-ler'
    name   = 'letter errors'
    fields = [Field('seg'),
	      Field('del'),
	      Field('ins'),
	      Field('sub'),
	      Field('errors'),
	      Field('letters'),
	      Field('ler', 6, '%6.2f', '%') ]

    def convertForAlignment(self, orth):
	return list(' '.join(orth.split()))


class WordErrorRate(Collector, EditDistance):
    id     = 'orth-wer'
    name   = 'word errors OV'
    fields = [Field('seg'),
	      Field('del'),
	      Field('ins'),
	      Field('sub'),
	      Field('errors'),
	      Field('words'),
	      Field('wer', 6, '%6.2f', '%') ]

    def __init__(self, *args):
	if args:
	    self.filterMode = args[0]
	    self.vocabulary = set([
		line.strip() for line in open(args[1]).readlines() ])

    def convertForAlignment(self, orth):
	return orth.split()
