

import re
import os

s = ''.join(file('TREC15.xml').readlines())

#print s

# <Q LANG="EN" QID="TREC15-1">What position did Moon play in professional football?</Q>
question_list = re.findall('<Q.*?>(.*?)<', s)

   

#NEaward							(what|which|name|give|tell) (.* )?(accolade|award|certification|decoration|honoring|honouring|medal|prize|reward)

s = ''.join(file('answertypepatterns').readlines())

pattern_list = re.findall('(.*?)\t+(.*)', s)

for p in pattern_list:
    print p[0], ',', p[1][:-1]

out = file('question_list.txt','w')
for q in question_list:
    print >>out, q.lower()
out.close()

out = file('regexp_list.txt','w')
for q in pattern_list:
    print >>out, q[0], ',', q[1][:-1]

    os.system('time ' + os.getcwd() +'/grep "' + q[1][:-1] + '" question_list.txt')


out.close()
