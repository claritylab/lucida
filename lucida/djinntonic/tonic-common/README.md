#DjiNN & Tonic#
[DjiNN](http://djinn.clarity-lab.org/djinn/) is a unified Deep Neural Network (DNN) web-service that allows DNN based applications to make requests to DjiNN.

[Tonic Suite](http://djinn.clarity-lab.org/tonic-suite/) is a series of 7 applications that span 3 domains:
- Image Processing: Image Classification (IMC), Facial Recognition (FACE), and Digit Recognition (DIG).
- Speech Recognition: Automatic Speech Recognition (ASR).
- Natural Language Processing: Part-Of-Speech (POS) tagging, Name Entity Recognition (NER), and Word Chunking (CHK).

DjiNN, Image Processing, and Natural Language Processing have been wraped in RPC framework using [Apache Thrift](http://thrift.apache.org/). 

###Basic Setup###
Make sure you have CAFFE installed. Follow these [instructions](http://djinn.clarity-lab.org/djinn/) to do so.

1. Set the Caffe installation path in `Makefile.config`, for example CAFFE=/home/jesswu/caffe-0.9-beta/distribute
2. Run these commands
```
$ make 
$ cd weights
$ ./dl_djinn_weights.sh
```

###Running Server and Client###
To run DjiNN, follow the instructions in the [DjiNN readme](https://github.com/Lilisys/clarityeco/tree/mergeTonic/djinn).

To run a service, follow the instructions in the readme's: 
- [Image Processing](https://github.com/Lilisys/clarityeco/tree/imgMerge/tonic-img) 
- [Natural Language Processing](https://github.com/Lilisys/clarityeco/tree/nlpMerge/tonic-nlp).


