"""
Analog Plug-in for search space statistics of word-conditioned tree search
"""

__version__   = '$Revision: 9227 $'
__date__      = '$Date: 2013-11-29 15:47:07 +0100 (Fri, 29 Nov 2013) $'


from statutils import MinAvgMaxStatistic

class StatesBeforePruning(MinAvgMaxStatistic):
    id   = 'states-before-pruning'
    name = 'states bef. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'states before pruning'

class StatesAfterPruning(MinAvgMaxStatistic):
    id   = 'states-after-pruning'
    name = 'states aft. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'states after pruning'

class TreesBeforePruning(MinAvgMaxStatistic):
    id   = 'trees-before-pruning'
    name = 'trees bef. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'trees before pruning'

class TreesAfterPruning(MinAvgMaxStatistic):
    id   = 'trees-after-pruning'
    name = 'trees aft. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'trees after pruning'

class EndingWordsBeforePruning(MinAvgMaxStatistic):
    id   = 'word-ends-before-pruning'
    name = 'word ends bef. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'ending words before pruning'

class EndingWordsAfterPruning(MinAvgMaxStatistic):
    id   = 'word-ends-after-pruning'
    name = 'word ends aft. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'ending words after pruning'

class WordsAfterRecombination(MinAvgMaxStatistic):
    id   = 'word-ends-after-recombination'
    name = 'word ends aft. recomb.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'ending words after recombi'

class EndingWordsAfter2ndPruning(MinAvgMaxStatistic):
    id   = 'word-ends-after-2nd-pruning'
    name = 'word ends aft. 2nd prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'ending words after 2nd pruning'

class FsaHmmStatesBeforePruning(MinAvgMaxStatistic):
    id   = 'hmm-states-before-pruning'
    name = 'hmm states bef. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'hmm states before pruning'

class FsaHmmStatesAfterPruning(MinAvgMaxStatistic):
    id   = 'hmm-states-after-pruning'
    name = 'hmm states aft. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'hmm states after pruning'

class FsaArcsBeforePruning(MinAvgMaxStatistic):
    id   = 'arcs-before-pruning'
    name = 'arcs bef. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'active network arcs before pruning'

class FsaArcsAfterPruning(MinAvgMaxStatistic):
    id   = 'arcs-after-pruning'
    name = 'arcs aft. prun.'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'active network arcs after pruning'

class FsaStatesAfterPruning(MinAvgMaxStatistic):
    id   = 'active-network-states'
    name = 'active netw. states'
    statisticContainerName = 'search-space-statistics'
    statisticName          = 'active network states after pruning'
