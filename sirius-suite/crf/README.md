## [Sirius-suite QA]: Conditional Random Fields

The Conditional Random Fields (CRF) algorithm assigns each word in OpenEphyra''s
question-answering service a part-of-speech (POS) which is influenced by
neighboring words. Viterbi portion of CRF tagging parallelized  

### Backend
The kernel is based the [Crfsuite](www.chokkan.org/software/crfsuite/).

### Directory structure
`./input/` contains a pretrained model on the CoNLL2000 dataset and test input
data following [crfsuite''s
tutorial](http://www.chokkan.org/software/crfsuite/tutorial.html) 

### Running the kernel
1. From the top directory `make` will run the required scripts to download,
patch and build the crfsuite.
2. When making changes to `tag.c`, run `./rebuild.sh` to rebuild the suite.
3. To run the kernel:
```bash
$ make test
```
or
```bash
$ ./test-crf.sh
```
or
```bash
$ ./crfsuite tag -qt -m ../input/model.model ../input/test.crfsuite.txt
```
The kernel does the following:
  1.TODO
