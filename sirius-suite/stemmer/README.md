## [Sirius-suite QA]: Porter Stemming

This is a word stemming kernel used in OpenEphyra''s question-answering
system. The stemming algorithm attempts to extract the root of each word by
matching common word endings. For example, adaptability becomes adapt.

### Backend
The stemming kernel is based on the original
[Stemming](http://tartarus.org/martin/PorterStemmer/) algorithm.

### Directory structure
`./input/` contains the original list of 29,401 words that can be stemmed. The
larger input files are copies of this original file.

### Running the kernel
1. Build all:  
```bash
$ make
```
2. Run the kernel on all platforms with the default inputs:
```bash
$ make test
```
or
```bash
$ ./stem_porter ../input/voc.txt
```
The kernel does the following:
  1. Reads in the list of words
  2. Using 6 different steps, matches the current word against common word
  endings
