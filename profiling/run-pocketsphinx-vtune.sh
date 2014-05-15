#!/bin/bash


count=1

IFS=$'\n'; 
for line in `cat umvoice_questions_new.txt`; 
#for line in `cat umvoice_questions_new_2q.txt`; 
do
#while read line; 
#do 
        echo "(1) Your voice search (text) is:"
        echo "$line"      
        
        filename=${line// /.}
        filename=`echo $filename | tr '[:upper:]' '[:lower:]'`

#        play $filename.wav 1> /dev/null 2>/dev/null      

        echo "(2) Calling pocketsphinx_continuous"
         pocketsphinx/src/programs/pocketsphinx_continuous -infile ./wav/$filename.wav -lm models/lm_giga_64k_nvp_3gram.lm.DMP -hmm models/voxforge_en_sphinx.cd_cont_5000 -dict models/cmu07a.dic
                
        echo "***********************************************"

#done < siri_questions.txt
done

