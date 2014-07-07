__version__   = '$Revision: 9227 $'
__date__      = '$Date: 2013-11-29 15:47:07 +0100 (Fri, 29 Nov 2013) $'

import os, threading, sys
try:
    from qt import *
    from qttable import *
except ImportError:
    sys.stderr.write("Can not load PyQt. Gui will not work.\n")
    class QObject:
	pass
    class QTabDialog:
	pass
    class QCustomEvent:
	pass



class Formatter(object):
    shouldShowTotal = False

    def __init__(self, out, fields):
	self.out = out
	self.fields = fields
	self.makeHeader()

    headerString = None
    def header(self):
	if self.headerString:
	    print >> self.out, self.headerString

    separatorString = None
    def sep(self):
	if self.separatorString:
	    print >> self.out, self.separatorString

    def makeHeader(self) : raise NotImplementedError
    def row(self, layer, values): raise NotImplementedError
    def file(self, fname): raise NotImplementedError


class TableFormatter(Formatter):
    shouldShowTotal = False

    def makeHeader(self):
	header = [ field.name for stat, field in self.fields ]
	self.headerString = '\t'.join(header)

    def file(self, *args):
	pass

    def row(self, id, layer, values):
	if layer is None:
	    row = [ id ]
	else:
	    row = [ id + '/' + layer.replace(' ', '_') ]
	row += [ str(values.get(sf, 'NA')) for sf in self.fields ]
	print >> self.out, '\t'.join(row)


class PrettyFormatter(Formatter):
    shouldShowTotal = True
    rowNameWidth = 30

    def file(self, fname, highlight = False, prefix = ''):
#	result = prefix + os.path.basename(fname)
	result = prefix + fname
	ll = len(result)
	if highlight:
	    result = '\033[01;31m' + result + '\033[0;0m'
	if ll < self.rowNameWidth:
	    line = result + (self.rowNameWidth - ll) * ' ' + self.fileString
	else:
	    line = result + self.fileString[ll - self.rowNameWidth :]
	print >> self.out, line

    def makeHeader(self):
	currentStat = None
	self.statFields = []
	for stat, field in self.fields:
	    if stat == currentStat:
		self.statFields[-1][1].append(field)
	    else:
		self.statFields.append((stat, [field]))
		currentStat = stat

	header1 = [ self.rowNameWidth * ' ' ] + \
		  [ stat.name.center(sum([field.width+1 for field in fields])-1)
		    for stat, fields in self.statFields ]
	header2 = [ 'file/subcorpus'.ljust(self.rowNameWidth) ] + \
		  [ ' '.join([field.name.center(field.width) for field in fields])
		    for stat, fields in self.statFields ]

	self.headerString = '|'.join(header1) + '\n'+ '|'.join(header2)
	self.separatorString = '+'.join([len(h) * '-' for h in header2])
	self.fileString = '|' + '|'.join([len(h) * ' ' for h in header2[1:]])

    def row(self, id, layer, values):
	def formValue(stat, field):
	    if (stat, field) in values:
		return field.format % values[(stat, field)]
	    else:
		return 'n/a'
	if layer is None:
	    row = [ id.ljust(self.rowNameWidth) ]
	else:
	    row = [ (' - ' + layer).ljust(self.rowNameWidth) ]
	row += [ ' '.join([ formValue(stat, field).rjust(field.width)
			    for field in fields])
		 for stat, fields in self.statFields ]
	row = '|'.join(row)
	print >> self.out, row


class QtFormatter(Formatter, QObject):

    class StatDialog(QTabDialog):
	def __init__(self):
	    QTabDialog.__init__(self)
	    self.tables = {}

	def customEvent(self, event):
	    if event.__class__ == QtFormatter.NewRowEvent:
		self.addRow(event.stat, event.row)

	def createTab(self, item):
	    tab = QVBox(self)
	    box = QVBox(tab)
	    box.setMargin(5)
	    table = QTable(box)
	    table.setNumCols(len(item.fields) + 2)
	    table.horizontalHeader().setLabel(0, "File")
	    table.horizontalHeader().setLabel(1, "Subcorpus")
	    i = 2
	    for field in item.fields:
		table.horizontalHeader().setLabel(i, field.name)
		i += 1
	    self.tables[item.stat.name] = table
	    self.addTab(tab, item.stat.name)

	def addRow(self, statName, row):
	    table = self.tables[statName]
	    r = table.numRows()
	    table.setNumRows(r + 1)
	    for col in range(len(row)):
		table.setText(r, col, row[col])
		table.setEditMode(QTable.NotEditing, r, col)
		table.adjustColumn(col)


    class GuiThread(threading.Thread):
	def __init__(self):
	    threading.Thread.__init__(self)
	    self.app = QApplication([])
	    self.dialog = QtFormatter.StatDialog()

	def run(self):
	    self.dialog.resize(700, 500)
	    self.dialog.setCaption("Analog")
	    self.dialog.show()
	    self.app.setMainWidget(self.dialog)
	    self.app.exec_loop()


    class NewRowEvent(QCustomEvent):

	def __init__(self, stat, row):
	    QCustomEvent.__init__(self, 1001)
	    self.stat = stat
	    self.row = row


    class Item:
	def __init__(self, stat, field):
	    self.stat = stat
	    self.fields = [ field ]



    def __init__(self, out, fields):
	self.gui = self.GuiThread()
	self.resizeInProgress = False
	self.currentFile = ""
	Formatter.__init__(self, out, fields)
	self.gui.start()

    def header(self):
	pass

    def sep(self):
	pass

    def file(self, fname, highlight = False, prefix = ''):
	self.currentFile = prefix + fname


    def makeHeader(self):
	currentStat = None
	self.items = []
	for stat, field in self.fields:
	    if stat == currentStat:
		self.items[-1].fields.append(field)
	    else:
		self.items.append( self.Item(stat, field) )
		currentStat = stat

	for item in self.items:
	    self.gui.dialog.createTab(item)


    def row(self, id, layer, values):
	for item in self.items:
	    row = []
	    if layer is None:
		label = id
	    else:
		label = id + '/' + layer
	    row.append(self.currentFile)
	    row.append(label)
	    for field in item.fields:
		if (item.stat, field) in values:
		    row.append( field.format % values[(item.stat, field)] )
		else:
		    row.append("n/a")
	    event = self.NewRowEvent(item.stat.name, row)
	    QApplication.postEvent(self.gui.dialog, event)
