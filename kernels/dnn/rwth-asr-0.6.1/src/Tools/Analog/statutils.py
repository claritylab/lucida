"""
Utility classes and fucntions for statistics.
"""

__version__   = '$Revision: 9227 $'
__date__      = '$Date: 2013-11-29 15:47:07 +0100 (Fri, 29 Nov 2013) $'


from analog import Collector, Field, pivot

class MinAvgMaxStatistic(Collector):
    # required
    statisticName          = None
    # optional
    statisticContainerName = None

    fields = [
	Field('min', 7, '%7.0f'),
	Field('avg', 9, '%7.1f'),
	Field('max', 7, '%7.0f') ]
    fieldProcessor = [
	('min', min),
	('avg', lambda s: sum(s) / len(s)),
	('max', max) ]

    def __call__(self, dataCollection):
	if len(self.fields) != len(self.fieldProcessor):
	    raise 'in MinAvgMaxStatistic: number of fields and field processor does not match'

	if self.statisticContainerName:
	    dataCollection = dataCollection.get(self.statisticContainerName, {})
	data = []
	for entry in dataCollection:
	    dataPoint = entry.get(self.statisticName, None)
	    if dataPoint:
		data.append(dataPoint)
	data = pivot(data)

	values = []
	for fp in self.fieldProcessor:
	    sample = data.get(fp[0], None)
	    if sample:
		values.append(fp[1](sample))
	    else:
		values.append(float('NaN'))
	return zip(self.fields, values)
