# thrift generator script

# services
cd $LUCIDAROOT/questionanswering/lucida;
thrift --gen java --gen cpp qaservice.thrift

cd $LUCIDAROOT/speechrecognition/lucida;
thrift --gen cpp kaldi.thrift
rm -rf $LUCIDAROOT/speechrecognition/lucida/gen-cpp/*.skeleton.cpp

cd $LUCIDAROOT/commandcenter;
thrift -r --gen cpp --gen java --gen js:node commandcenter.thrift
thrift -r --gen cpp --gen js:node filetransfer_svc.thrift
thrift -r --gen cpp $LUCIDAROOT/questionanswering/lucida/qaservice.thrift
thrift -r --gen cpp $LUCIDAROOT/speechrecognition/lucida/kaldi.thrift
thrift -r --gen cpp $LUCIDAROOT/imagematching/lucida/service.thrift
rm -rf $LUCIDAROOT/commandcenter/gen-cpp/*.skeleton.cpp

cd $LUCIDAROOT/commandcenter-py
thrift -r --gen py $LUCIDAROOT/commandcenter/commandcenter.thrift
thrift -r --gen py $LUCIDAROOT/questionanswering/lucida/qaservice.thrift
thrift -r --gen py $LUCIDAROOT/speechrecognition/lucida/kaldi.thrift
thrift -r --gen py $LUCIDAROOT/imagematching/lucida/service.thrift
thrift -r --gen py $LUCIDAROOT/djinntonic/lucidaservice.thrift

cd $LUCIDAROOT/djinntonic
thrift -r --gen cpp $LUCIDAROOT/djinntonic/lucidaservice.thrift
rm -rf $LUCIDAROOT/djinntonic/gen-cpp/*.skeleton.cpp
