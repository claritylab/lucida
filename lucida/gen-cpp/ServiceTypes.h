#ifndef ServiceTYPES_H
#define ServiceTYPES_H

#include <string>

using namespace std;

struct ServiceTypes {
    string ASR_SERVICE = "asr";
    string IMM_SERVICE = "imm";
    string QA_SERVICE = "qa";
    string SIRIUS_SERVICE = "sirius";
    string DjiNN_SERVICE = "djinn";
    string NLP_SERVICE = "nlp";
    string IMP_SERVICE = "imp";
    string RESIZE_SERVICE = "resize";

    string SERVICE_INPUT_AUDIO = "audio";
    string SERVICE_INPUT_IMAGE = "image";
    string SERVICE_INPUT_TEXT = "text";
};

#endif