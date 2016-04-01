# Question Answering

The question-answer program in template/ is a stand-alone service for use in any application.

## Basic Setup

1) Compile server: `./compile-qa.sh`

2) Start server: `./start-qa.sh (PORT)`

3) Run the tests:

```
./qaclient --factoid "who directed inception?" (PORT)
```

Last Modified: 07/01/15
