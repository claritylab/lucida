"""
Analog Plug-in for word error rate
"""

__version__   = '$Revision: 9621 $'
__date__      = '$Date: 2014-05-13 19:35:55 +0200 (Tue, 13 May 2014) $'


from analog import Collector, Field, pivot

class EditErrorRate(Collector):
    def makeFields(cls, unit):
	return [
	    Field('seg'),
	    Field('del'),
	    Field('ins'),
	    Field('sub'),
	    Field('errors'),
	    Field(unit),
	    Field(unit[0] + 'er', 6, '%6.2f', '%') ]
    makeFields = classmethod(makeFields)
    dataField = None

    def __call__(self, data):
	data = data[self.dataField]
	nSegments = len(data)
	data = pivot(data)
	nInsertions = sum(data['insertion'])
	nDeletions = sum(data['deletion'])
	nSubstitutions = sum(data['substitution'])
	nTokens = sum(data['token'])
	nErrors = nDeletions + nInsertions + nSubstitutions
	errorPercent = 100
	if nTokens  != 0: errorPercent = 100.0 * nErrors / nTokens
	return zip(self.fields, [
	    nSegments, nDeletions, nInsertions, nSubstitutions,
	    nErrors, nTokens, errorPercent ])

class SingleBestWordErrorRate(EditErrorRate):
    id     = 'sb-wer'
    name   = 'word errors'
    dataField = 'word errors'
    fields = EditErrorRate.makeFields('words')
class LatticeWordErrorRate(EditErrorRate):
    id     = 'lattice-wer'
    name   = 'lattice word errors'
    dataField = 'lattice word errors'
    fields = EditErrorRate.makeFields('words')

class SingleBestLetterErrorRate(EditErrorRate):
    id     = 'sb-ler'
    name   = 'letter errors'
    dataField = 'letter errors'
    fields = EditErrorRate.makeFields('letter')
class LatticeLetterErrorRate(EditErrorRate):
    id     = 'lattice-ler'
    name   = 'lattice letter errors'
    dataField = 'lattice letter errors'
    fields = EditErrorRate.makeFields('letter')

class SingleBestPhonemeErrorRate(EditErrorRate):
    id     = 'sb-per'
    name   = 'phoneme errors'
    dataField = 'phoneme errors'
    fields = EditErrorRate.makeFields('phonemes')
class LatticePhonemeErrorRate(EditErrorRate):
    id     = 'lattice-per'
    name   = 'lattice phoneme errors'
    dataField = 'lattice phoneme errors'
    fields = EditErrorRate.makeFields('phonemes')


class LatticeDensity(Collector):
    id     = 'lattice-density'
    name   = 'word lattice density'
    fields = [ Field('min', 6, '%6.1f'),
	       Field('avg', 6, '%6.1f'),
	       Field('max', 6, '%6.1f') ]

    def __call__(self, data):
	nan = float("-nan")
	inf = float("inf")
	dens = [ d for d in data['lattice density'] if d != nan and d != inf ]
	minDns = min(dens)
	avgDns = sum(dens) / len(dens)
	maxDns = max(dens)
	return zip(self.fields, [ minDns, avgDns, maxDns ])
