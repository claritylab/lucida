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
// $Id: Component.cc 8249 2011-05-06 11:57:02Z rybach $

#include "Component.hh"

#include <cerrno>
#include <cstring>

#include "Application.hh"
#include "Assertions.hh"
#include "Parameter.hh"

namespace Core {

    const Choice Component::errorActionChoice(
	"ignore",         ErrorActionIgnore,
	"delayed-exit",   ErrorActionDelayedExit,
	"immediate-exit", ErrorActionImmediateExit,
	Choice::endMark());

    const char *Component::errorNames[nErrorTypes] = {
	"information",
	"warning",
	"error",
	"critical-error"
    };

    const char *Component::errorChannelNames[nErrorTypes] = {
	"log",
	"warning",
	"error",
	"critical"
    };

    const Channel::Default Component::errorChannelDefaults[nErrorTypes] = {
	Channel::standard,
	Channel::error,
	Channel::error,
	Channel::error
    };

    Component::Component(const Configuration &c) :
	Precursor(c) {
	initialize();
    }

    Component::Component(const Component &component) :
	Configurable(component) {
	initialize();
	std::copy(component.errorCounts_, component.errorCounts_ + nErrorTypes, errorCounts_);
    }

    Component::~Component() {
	for (int e = 0 ; e < nErrorTypes ; ++e) {
	    delete errorChannels_[e];
	}
    }

    Component &Component::operator=(const Component &component) {
	Precursor::operator=(component);
	std::copy(component.errorCounts_, component.errorCounts_ + nErrorTypes, errorCounts_);
	return *this;
    }

    void Component::initialize() {
	static const ParameterChoice onWarning(
	    "on-warning",
	    &errorActionChoice,
	    "what happens when a warning occurs",
	    ErrorActionIgnore);

	static const ParameterChoice onError(
	    "on-error",
	    &errorActionChoice,
	    "what happens when an error occurs",
	    ErrorActionImmediateExit);

	static const ParameterChoice onCriticalError(
	    "on-critical-error",
	    &errorActionChoice,
	    "what happens when a critical error occurs",
	    ErrorActionImmediateExit);

	errorActions_[ErrorTypeInfo         ] = ErrorActionIgnore;
	errorActions_[ErrorTypeWarning      ] = ErrorAction(onWarning      (config));
	errorActions_[ErrorTypeError        ] = ErrorAction(onError        (config));
	errorActions_[ErrorTypeCriticalError] = ErrorAction(onCriticalError(config));

	for (int e = 0 ; e < nErrorTypes ; ++e) {
	    errorCounts_[e] = 0;
	    errorChannels_[e] = 0;
	}

	if (errorActions_[ErrorTypeCriticalError] != ErrorActionImmediateExit) {
	    warning("Critical errors will be delayed or ignored. Expect unpredictable behaviour!");
	}
    }

    XmlChannel *Component::errorChannel(ErrorType mt) const {
	require(0 <= mt && mt < nErrorTypes);
	if (!errorChannels_[mt]) {
	    errorChannels_[mt] = new XmlChannel(
		config,
		errorChannelNames[mt],
		errorChannelDefaults[mt]);
	}
	return errorChannels_[mt];
    }

    void Component::errorOccured(ErrorType mt) const {
	require(0 <= mt && mt < nErrorTypes);

	errorCounts_[mt] += 1;

	switch (errorActions_[mt]) {
	case ErrorActionIgnore:
	case ErrorActionDelayedExit:
	    break;
	case ErrorActionImmediateExit:
	    exit();
	    break;
	default: defect(); break;
	}
    }

    bool Component::hasFatalErrors() const {
	for (int et = nErrorTypes-1 ; et >= 0 ; --et) {
	    if (errorCounts_[et] > 0) {
		switch (errorActions_[et]) {
		case ErrorActionIgnore:
		    break;
		case ErrorActionDelayedExit:
		case ErrorActionImmediateExit:
		    return true;
		    break;
		default: defect(); break;
		}
	    }
	}
	return false;
    }

    void Component::respondToDelayedErrors() const {
	if (hasFatalErrors()) exit();
    }

    void Component::exit() const {
	static const ParameterInt errorCode(
	    "error-code",
	    "exit status in case of a critical error",
	    EXIT_FAILURE, 0, 255,
	    "This is the exit status to be returned when the program aborts "
	    "due to a runtime error within this component.");

	*errorChannel(ErrorTypeCriticalError)
	    << XmlOpen(errorNames[ErrorTypeCriticalError])
		+ XmlAttribute("component", fullName())
	    << "Terminating due to previous errors"
	    << XmlClose(errorNames[ErrorTypeCriticalError]);

	Application::us()->exit(errorCode(config));
    }

    XmlChannel *Component::vErrorMessage(
	ErrorType type, const char *msg, va_list ap) const
    {
	require(0 <= type && type < nErrorTypes);

	XmlChannel *chn = errorChannel(type);

	*chn << XmlOpen(errorNames[type]) + XmlAttribute("component", fullName());
	if (msg) {
	    (*chn) << vform(msg, ap);
	}

	if ((type != ErrorTypeInfo) && (type != ErrorTypeWarning) && errno) {
	    /*! \todo strerror() is not thread safe.  But strerror_r() does not seems to work. */
/*
	    char libCError[256];
	    strerror_r(errno, libCError, 256);
	    libCError[255] = 0;
*/
	    *chn << XmlOpen("system") << strerror(errno) << XmlClose("system");
	    errno = 0;
	}

	return chn;
    }

    Component::Message Component::log(const char *msg, ...) const{
	va_list ap ;
	require(msg);

	va_start(ap, msg);
	XmlChannel *chn =  vErrorMessage(ErrorTypeInfo, msg, ap);
	va_end(ap) ;
	return Message(this, ErrorTypeInfo, chn);
    }

    Component::Message Component::log() const {
	XmlChannel *chn = errorChannel(ErrorTypeInfo);
	*chn << XmlOpen(errorNames[ErrorTypeInfo]) + XmlAttribute("component", fullName());
	return Message(this, ErrorTypeInfo, chn);
    }

    Component::Message Component::warning(const char *msg, ...) const {
	va_list ap ;

	va_start(ap, msg) ;
	XmlChannel *chn = vErrorMessage(ErrorTypeWarning, msg, ap);
	va_end(ap) ;

	return Message(this, ErrorTypeWarning, chn);
    }

    Component::Message Component::vWarning(const char *msg, va_list ap) const {
	XmlChannel *chn = vErrorMessage(ErrorTypeWarning, msg, ap);
	return Message(this, ErrorTypeWarning, chn);
    }

    Component::Message Component::error(const char *msg, ...) const {
	va_list ap ;

	va_start(ap, msg) ;
	XmlChannel *chn = vErrorMessage(ErrorTypeError, msg, ap);
	va_end(ap) ;

	return Message(this, ErrorTypeError, chn);
    }

    Component::Message Component::vError(const char *msg, va_list ap) const {
	XmlChannel *chn = vErrorMessage(ErrorTypeError, msg, ap);
	return Message(this, ErrorTypeError, chn);
    }

    Component::Message Component::criticalError(const char *msg, ...) const {
	va_list ap ;

	va_start(ap, msg) ;
	XmlChannel *chn = vErrorMessage(ErrorTypeCriticalError, msg, ap);
	va_end(ap) ;

	return Message(this, ErrorTypeCriticalError, chn);
    }

    Component::Message Component::vCriticalError(const char *msg, va_list ap) const {
	XmlChannel *chn = vErrorMessage(ErrorTypeCriticalError, msg, ap);
	return Message(this, ErrorTypeCriticalError, chn);
    }


    Component::Message& Component::Message::form(const char *msg, ...)   {
	va_list ap ;
	va_start(ap, msg) ;
	(*ostream_) << Core::vform(msg, ap);
	va_end(ap) ;
	return *this;
    }

    Component::Message::~Message() {
	if (component_) {
	    *ostream_ << XmlClose(errorNames[type_]);
	    component_->errorOccured(type_);
	}
    }

} // namespace Core
