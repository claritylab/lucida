// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// $Id: Assertions.cc 9621 2014-05-13 17:35:55Z golik $

#include "Assertions.hh"

#include <errno.h>
#ifdef OS_linux
#include <execinfo.h>
#endif
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <Application.hh>

namespace AssertionsPrivate {

void stackTrace(std::ostream &os, int cutoff) {
#ifdef OS_linux
#if defined(DEBUG) || defined(DEBUG_LIGHT)
    static const size_t maxTraces = 100;

    // Get backtrace lines
    void *array[maxTraces];
    size_t nTraces = backtrace(array, maxTraces);
    char **strings = backtrace_symbols(array, nTraces);

    // Extract addresses
    const char *tmpNamBuf1 = "./bt-addresses.tmp";
    const char *tmpNamBuf2 = "./bt-results.tmp";

    std::ofstream out(tmpNamBuf1);
    for (size_t i=cutoff+1; i<nTraces; ++i) {
	std::string line(strings[i]);
	size_t firstAddrPos = line.find("[")+1;
	size_t addrLength   = line.find("]" - firstAddrPos);
	out << line.substr(firstAddrPos, addrLength) << std::endl;
    }
    out.close();

    // Run addr2line to get usable stack trace information
    Core::Application *thisApp = Core::Application::us();
    std::string addr2lineCmd = "addr2line -C -f -e " + thisApp->getPath() + "/" + thisApp->getBaseName() + " < " + tmpNamBuf1 + " > " + tmpNamBuf2;

    os << std::endl << "Analyzing stack trace with command " << addr2lineCmd << std::endl;
    os << "Please be patient (approx. 30 s)..." << std::endl;
    system(addr2lineCmd.c_str());

    // Evaluate addr2line output
    os << "Stack trace (innermost first):" << std::endl;
    std::ifstream in(tmpNamBuf2);
    for (size_t i=cutoff+1; i<nTraces; ++i){
	std::string line1; getline(in, line1);
	std::string line2; getline(in, line2);
	std::string line(strings[i]);
	size_t firstAddrPos = line.find("[");
	size_t addrLength  = line.find("]") - firstAddrPos + 2;
	os << "#" << i << ":\t" << line1 << std::endl;
	os << "\t   at: " << line2 << " " << line.substr(firstAddrPos, addrLength) << std::endl;
    }

    // Clean up.
    free(strings);
    unlink(tmpNamBuf1);
    unlink(tmpNamBuf2);

#else
    os << "Creating stack trace (innermost first):" << std::endl;
    static const size_t maxTraces = 100;
    void *array[maxTraces];
    size_t nTraces = backtrace(array, maxTraces);
    char **strings = backtrace_symbols(array, nTraces);
    for (size_t i = cutoff+1; i < nTraces; i++)
	os << '#' << i << "  " << strings[i] << std::endl;
    free(strings);
#endif // DEBUG
#endif // OS_LINUX
}

void abort() __attribute__ ((noreturn));

FailedAssertion::FailedAssertion(const char *type,
				 const char *expr,
				 const char *function,
				 const char *filename,
				 unsigned int line) {
    std::cerr << std::endl << std::endl
	      << "PROGRAM DEFECTIVE:"
	      << std::endl
	      << type << ' ' << expr << " violated" << std::endl
	      << "in " << function
	      << " file " << filename << " line " << line << std::endl;
}

FailedAssertion::~FailedAssertion() {
    std::cerr << std::endl;
    stackTrace(std::cerr, 1);
    std::cerr << std::endl << std::flush;
    abort() ;
}

void assertionFailed(const char *type,
		     const char *expr,
		     const char *function,
		     const char *filename,
		     unsigned int line) {
    FailedAssertion(type, expr, function, filename, line);
}

void hopeDisappointed(const char *expr,
		      const char *function,
		      const char *filename,
		      unsigned int line) {
    std::cerr << std::endl << std::endl
	      << "RUNTIME ERROR:"
	      << std::endl
	      << "hope " << expr << " disappointed" << std::endl
	      << "in " << function
	      << " file " << filename << " line " << line;
    if (errno) std::cerr << ": " << strerror(errno);
    std::cerr << std::endl << std::endl;
    stackTrace(std::cerr, 1);
    std::cerr << std::endl
	      << "PLEASE CONSIDER ADDING PROPER ERROR HANDLING !!!" << std::endl
	      << std::endl << std::flush;
    abort() ;
}

class ErrorSignalHandler {
    static volatile sig_atomic_t isHandlerActive;
    static void handler(int);
public:
    ErrorSignalHandler();
    void abort() __attribute__((noreturn));
};

volatile sig_atomic_t ErrorSignalHandler::isHandlerActive = 0;

void ErrorSignalHandler::handler(int sig) {
    if (!isHandlerActive) {
	isHandlerActive = 1;
	std::cerr << std::endl << std::endl
		  << "PROGRAM DEFECTIVE (TERMINATED BY SIGNAL):" << std::endl
		  << strsignal(sig) << std::endl
		  << std::endl;
	stackTrace(std::cerr, 1);
	std::cerr << std::endl << std::flush;
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

ErrorSignalHandler::ErrorSignalHandler() {
    signal(SIGBUS,  handler);
    signal(SIGFPE,  handler);
    signal(SIGILL,  handler);
    signal(SIGABRT, handler);
    signal(SIGSEGV, handler);
    signal(SIGSYS,  handler);
}

void ErrorSignalHandler::abort() {
    signal(SIGABRT,  SIG_DFL);
    ::abort();
    signal(SIGABRT,  handler);
}

static ErrorSignalHandler errorSignalHandler;

void abort() {
    errorSignalHandler.abort();
}

} // namespace AssertionsPrivate
