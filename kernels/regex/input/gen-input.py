import re
import os

s = ''.join(file('TREC15.xml').readlines())

question_list = re.findall('<Q.*?>(.*?)<', s)

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
