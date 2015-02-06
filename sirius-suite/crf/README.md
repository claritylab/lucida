### Conditional Random Fields (CRF)

Based on www.chokkan.org/software/crfsuite/  
Viterbi portion of CRF tagging parallelized  

#### Serial version:
- Download library from www.chokkan.org/software/crfsuite/
- Copy files from `input` into `crfsuite/frontend`
- In `frontend/`
```bash 
$ ./crfsuite tag -qt -m model.model test.crfsuite.txt  
```

#### Pthread version:
- Change the number of threads in `tag.c`
- Run `sirius-crf.sh` to download, apply patch, and rebuild.
- In `frontend/`
```bash 
$ ./crfsuite tag -qt -m model.model test.crfsuite.txt  
```
