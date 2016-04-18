# thrift generator script

set -ex

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

cd $LUCIDAROOT/djinntonic
thrift -r --gen cpp $LUCIDAROOT/djinntonic/lucidaservice.thrift
rm -rf $LUCIDAROOT/djinntonic/gen-cpp/*.skeleton.cpp

cd $LUCIDAROOT/learn
thrift --gen java --gen py parser_service.thrift
