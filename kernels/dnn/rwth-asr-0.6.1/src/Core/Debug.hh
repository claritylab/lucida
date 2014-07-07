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
#ifndef _CORE_DEBUG_HH
#define _CORE_DEBUG_HH

/**
 * debug macros to make stuff compiler removable
 */
#ifdef __cplusplus
#include <Core/Application.hh>
#endif


// ---------------------------------------------------

#ifndef DBG_LEVEL
#define DBG_LEVEL -1  // define and ignore if DBG_LEVEL is unknwon
#endif

#if defined(RELEASE)
#define DBG_LEVEL -1  // always ignore DBG_LEVEL in RELEASE mode
#endif

// ---------------------------------------------------

/**
 * execute debug command depending on debug level
 *
 * C++ example:  DBGCMD(myLevel, callMyTimeConsumingDebugFunction());
 *
 * C example:    DBGCMD(myLevel, fprintf(stdout, "(%d/%d) %s:%d:%s filename='%s'\n",
 *                                         myLevel, DBG_LEVEL,
 *                                         __FILE__, __LINE__, __PRETTY_FUNCTION__,
 *                                         filename));
 *
 */
#define DBGCMD(level,cmd) if(DBG_LEVEL>=level) cmd


// ---------------------------------------------------
// following commands require C++
// ---------------------------------------------------
#ifdef __cplusplus


/**
 * return binary stream
 */
#define DBGCHN(level) if(DBG_LEVEL>=level) (Core::Application::us()->debugChannel())

/**
 * print debug information on binary stream
 */
#define DBG(level) if(DBG_LEVEL>=level) (Core::Application::us()->debugChannel()) << "DBG(" << ((int)(level)) <<"/"<<(int)(DBG_LEVEL) << ") [" << __FILE__<<":"<<__LINE__<<":"<<__PRETTY_FUNCTION__<< "]"

/**
 * debug variable name and value on binary stream
 *
 * e.g. DBG(10) << VAR(x) << std::endl;
 */
#define VAR(x)  #x " = " << (x)

/**
 * end debugging on binary stream
 */
#define ENDDBG std::endl


// ---------------------------------------------------

/**
 * return XML stream
 */
#define DBGXCHN(level) if(DBG_LEVEL>=level) (Core::Application::us()->debugXmlChannel())


/**
 * print debug information on XML stream
 */
#define DBGX(level) if(DBG_LEVEL>=level) (Core::Application::us()->debugXmlChannel()) << Core::XmlOpen("debug")+Core::XmlAttribute("level", (int)(level))+Core::XmlAttribute("max-level", (int)(DBG_LEVEL))+Core::XmlAttribute("file", __FILE__)+Core::XmlAttribute("line", __LINE__)+Core::XmlAttribute("function", __PRETTY_FUNCTION__)


/**
 * debug variable name and value on binary stream
 *
 * e.g. DBGX(10) << VARX(x) << ENDDBGX;
 */
#define VARX(x)  (Core::XmlEmpty("variable")+Core::XmlAttribute("name", #x)+Core::XmlAttribute("value", (x)))

/**
 * end debugging on XML stream
 */
#define ENDDBGX Core::XmlClose("debug")


namespace Core {

bool AmIBeingDebugged();

}


// ---------------------------------------------------
// following commands can be used in C code
// ---------------------------------------------------
#else

/**
 * print debug information on binary stream
 * example: DBG(myLevel); VAR(myLevel, "filename='%s'", filename); ENDDBG(myLevel);
 *
 * @todo add support for configurable debug channel
 *
 */
#define DBG(level) if(DBG_LEVEL>=level) fprintf(stdout, "(%d/%d) %s:%d:%s ", (int)(level), (int)(DBG_LEVEL), __FILE__, __LINE__, __PRETTY_FUNCTION__)

/**
 * debug variable name and value on binary stream
 *
 * examples: VAR(myLevel, "myVar1=%lf myVar1=%d", myVar1, myVar2);
 *           VAR(myLevel, "just a string without arguments");
 */
#define VAR(level, format, ...) if(DBG_LEVEL>=level) fprintf(stdout, format, ## __VA_ARGS__)

/**
 * end debugging on binary stream
 */
#define ENDDBG(level) if(DBG_LEVEL>=level) fprintf(stdout, "\n")


/**
 * print error message on stderr binary stream
 *
 * examples: ERROR("couldn't open file='%s'", filename);
 *
 */
#define ERROR(format, ...) fprintf(stderr, "ERROR %s:%d:%s ", __FILE__, __LINE__, __PRETTY_FUNCTION__);fprintf(stderr, format, ## __VA_ARGS__)

/**
 * print warning message on stderr binary stream
 *
 * examples: WARNING("unknown value='%s'", value);
 *
 */
#define WARNING(format, ...) fprintf(stderr, "WARNING %s:%d:%s ", __FILE__, __LINE__, __PRETTY_FUNCTION__);fprintf(stderr, format, ## __VA_ARGS__)


// ---------------------------------------------------
// ---------------------------------------------------
#endif // end cplusplus

#endif // _CORE_DEBUG_HH
