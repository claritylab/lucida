"""
Analog Plug-in for processing time (esp. real time factor)
"""

__version__   = '$Revision: 9227 $'
__date__      = '$Date: 2013-11-29 15:47:07 +0100 (Fri, 29 Nov 2013) $'


from analog import Collector, Field


class RealTime(Collector):
    id     = 'time'
    name   = 'time'
    fields = [Field('duration', 7, '%7.1f', 's'),
	      Field('CPU',      7, '%7.1f', 's'),
	      Field('rtf',      6, '%6.2f') ]

    def __call__(self, data):
	cpuTime  = sum(data['user time'])
	duration = sum(data['real time'])
	return zip(self.fields, [
	    duration, cpuTime, cpuTime / duration ])
