#ifndef __POCKETSPHINXCOMMANDLINE__
#define __POCKETSPHINXCOMMANDLINE__

static char* fake_argv[]= {"pocketsphinx_continuous.exe",
    	"-live", "yes",
		"-mmap","no",
    	"-verbose","1",
    	"-fwdflat", "no",
    	"-bestpath", "no",
    //	"-samprate","8000",
	//	"-nfft","256",
    	"-lm", "/model/lm/tidigits/tidigits.lm",
    	"-dict", "/model/lm/tidigits/tidigits.dic",
    	"-hmm", "/model/hmm/tidigits",NULL};
#endif