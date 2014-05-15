
from xml.dom import minidom

#xmldoc = minidom.parse("new-siri-benchmark-cpu-live2.xml")
#xmldoc = minidom.parse("openephyra-12-requests-profiling-new.xml")
xmldoc = minidom.parse("cpu-live.xml")

method_tab = {}
method_tab_conso = {}
total_time = 0.0
for r in xmldoc.getElementsByTagName('TableRow'):

    cols = r.getElementsByTagName('TableColumn')
    
    method_name = str(cols[0].childNodes[0].data)
    if method_name.startswith('java') or method_name.startswith('sun') or method_name.startswith('org.eclipse'):
        continue        
     
    # consolidate method names  

    if 'info.ephyra.search.Search.waitForResults' in method_name:
        continue
        
#    if 'cmu.sphinx' in method_name:
#        print method_name
#    else:
#        continue

    if 'info.ephyra.answerselection.AnswerPattern.apply' in method_name:
        continue
        
    if 'edu.cmu.sphinx.decoder.search.WordPruningBreadthFirstSearchManager' in method_name:
        method_name = 'sphinx.decoder.search.WordPruningBreadthFirstSearchManager'
    
#    if 'edu.cmu.sphinx.linguist.lextree' in method_name:
#        method_name = 'edu.cmu.sphinx.linguist.lextree'

    vals = method_name.split('.')
            
#    if method_name.startswith('edu.cmu.sphinx.decoder.search'):
#        method_name = 'sphinx.decoder.search'            
#    elif method_name.startswith('edu.cmu.sphinx'):

#        method_name = '.'.join(vals[2:6])

    if method_name.startswith('edu.cmu.sphinx.linguist.lextree.LexTreeLinguist$LexTreeHMMState'):
        method_name = 'edu.cmu.sphinx.linguist.lextree.LexTreeLinguist$LexTreeHMMState'
    if method_name.startswith('org.tartarus.snowball'):
#        method_name = 'org.tartarus.snowball'    
        method_name = '.'.join(vals[1:5])            
    if method_name.startswith('lemurproject.indri'):
        method_name = 'lemurproject.indri'
    elif method_name.startswith('info.ephyra.nlp'):
        method_name = '.'.join(vals[1:5])
    if method_name.startswith('info.ephyra.util'):
        method_name = '.'.join(vals[1:4])     
    if method_name.startswith('edu.stanford.nlp'):
        method_name = '.'.join(vals[1:5])        
    if method_name.startswith('opennlp.tools.lang.english.NameFinder'):
        method_name = 'opennlp.NameFinder'        
                
    # self time %
#    self_time_per = cols[1].childNodes[0].data         
    
    # self time ms
    self_time_ms = float(cols[2].childNodes[0].data.split(' ')[0])
    total_time += self_time_ms    
    if method_name not in method_tab:  
        method_tab[method_name] = self_time_ms
    else:
        method_tab[method_name] += self_time_ms

    # total time
#    total_time_ms = cols[3].childNodes[0].data         
    
    # num invocations
#    num_invoc = cols[4].childNodes[0].data             


method_tab_per = []
for m in method_tab:
    method_tab_per += [(method_tab[m] / total_time, m)]
    

method_tab_per.sort(reverse=True)    

for i in range(10):
    print method_tab_per[i][1], '\t', method_tab_per[i][0]
    
    
# Speech recog:    
# edu.cmu.sphinx
# (1) decoder.search.WordPruningBreadthFirstSearchManager
# (2) linguist.lextree.LexTreeLinguist$LexTreeHMMState

# NLP
# edu.stanford.nlp
# (1) process.WordShapeClassifier    

# info.ephyra.
# (1) nlp.RegExMatcher.markAllMatches
    

