# thrift generator script

# services
cd $LUCIDAROOT/questionanswering/lucida;
thrift --gen java --gen cpp qaservice.thrift

cd $LUCIDAROOT/commandcenter;
thrift --gen cpp --gen java --gen js:node commandcenter.thrift
thrift --gen cpp --gen js:node filetransfer_svc.thrift
thrift --gen cpp $LUCIDAROOT/questionanswering/lucida/qaservice.thrift
thrift --gen cpp $LUCIDAROOT/speechrecognition/lucida/kaldi.thrift
thrift --gen cpp $LUCIDAROOT/imagematching/lucida/service.thrift

cd $LUCIDAROOT/commandcenter-py
thrift --gen py $LUCIDAROOT/commandcenter/commandcenter.thrift
thrift -r --gen py $LUCIDAROOT/questionanswering/lucida/qaservice.thrift
thrift -r --gen py $LUCIDAROOT/speechrecognition/lucida/kaldi.thrift
thrift -r --gen py $LUCIDAROOT/imagematching/lucida/service.thrift
thrift -r --gen py $LUCIDAROOT/djinntonic/lucidaservice.thrift
