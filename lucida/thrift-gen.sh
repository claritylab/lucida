# thrift generator script
thrift --gen cpp service.thrift

cd command-center;
thrift --gen cpp --gen java --gen js:node commandcenter.thrift
thrift --gen cpp --gen js:node filetransfer_svc.thrift
thrift --gen cpp ../qa/lucida/qaservice.thrift
thrift --gen cpp ../asr/lucida/kaldi.thrift
thrift --gen cpp ../imm/lucida/service.thrift

cd -; cd qa/lucida;
thrift --gen java --gen cpp qaservice.thrift
