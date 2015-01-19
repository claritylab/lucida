### Conditional Random Fields (CRF)

Based on www.chokkan.org/software/crfsuite/  
Viterbi portion of CRF tagging parallelized  

1. Run sirius-crf.sh in [sirius-extras](sirius-extras) to download, apply patch, and rebuild.
2. Test using: crfsuite tag -qt -m model.model test.crfsuite.txt  
