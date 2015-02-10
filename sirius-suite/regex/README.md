## [Sirius-suite QA]: Regular Expression

This is a regular expression kernel reflective of OpenEphyra''s
question-answering system.

### Backend
The kernel uses [SLRE](https://github.com/cesanta/slre) for the regular
expression matching.

### Directory structure
`./input/` contains a list of patterns and questions to match. These are
patterns and questions used in OpenEphyra''s Question-Answering system. There
is a list of 100 patters and two files each with 200 and 300 sentences.

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
$ ./regex_slre ../input/list ../input/questions
```
The kernel does the following:
  1. Reads and compiles the list of regular expressions
  2. Reads in the list of questions
  3. Applies each regexp to each sentence
