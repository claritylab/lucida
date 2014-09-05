
import os


#for q in file('question_list.txt'):
for r in file('regexp_list.txt'):
    os.system(os.getcwd() +'/grep ' + r + ' question_list.txt')
        

