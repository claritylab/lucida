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
// $Id: Component.hh 6566 2007-06-15 12:04:56Z hoffmeister $

#ifndef _CORE_COMPONENT_HH
#define _CORE_COMPONENT_HH

#include <cstdarg>
#include "Configurable.hh"
#include "Channel.hh"
#include "Choice.hh"
#include "StringUtilities.hh"
#include "XmlStream.hh"

namespace Core {

    /**
     * Base class for configurable components with logging
     * and error logging facilities.
     * The error model supposes that there are three types of errors:
     *   -# warnings:        Things which are possibly wrong,
     *                       but may well be correct.
     *   -# errors:          Things are most certainly wrong,
     *                       but we can try to continue.
     *   -# critical errors: Things are most definitelly wrong and
     *                       there is no way to continue.
     *
     * Remember that this is for runtime errors only.  Logical program
     * errors should be handled by Assertions.
     *
     * For all types of errors a message is output on the corresponding
     * channel.  The default behaviour is to
     *   - continue after warnings (ignore)
     *   - immediatelly abort execution on errors (immediate-exit) -> andras used to mix up critical-errors and errors
     *   - immediatelly abort execution on ciritcal errors (immediate-exit)
     *
     * This behaviour can be changed using the configuration options
     *   - on-warning
     *   - on-error
     *   - on-critical-error
     *
     * Component predefines four channels:
     *   - log
     *   - warning
     *   - error
     *   - critical
     *
     * Messages are formatted to an indented XML text. Each call to
     * one of the message functions (log, warning, error,
     * criticalError) creates an XML with the full name of the
     * component and the system error ID. The message functions return
     * a Message object which can be used for streaming complex
     * information into the body of the message tag.  The
     * Xml... (XmlOpen, XmlClose, XmlEmpty, XmlComment) objects
     * can be useful for creating a structured message bodies.
     */

    class Component :
	public Configurable
    {
	typedef Configurable Precursor;
    public:
	class Message;
    protected:
	enum ErrorType {
	    ErrorTypeInfo,
	    ErrorTypeWarning,
	    ErrorTypeError,
	    ErrorTypeCriticalError,
	    nErrorTypes
	};

    private:
	enum ErrorAction {
	    ErrorActionIgnore,
	    ErrorActionDelayedExit,
	    ErrorActionImmediateExit
	};

	static const Choice errorActionChoice;
	static const char *errorNames[nErrorTypes];
	static const char *errorChannelNames[nErrorTypes];
	static const Channel::Default errorChannelDefaults[nErrorTypes];

	ErrorAction errorActions_[nErrorTypes];
	mutable int errorCounts_ [nErrorTypes];
	mutable XmlChannel *errorChannels_[nErrorTypes];

    private:
	friend class Component::Message;

	/**
	 *  Performs initilization common in the different constructors.
	 */
	void initialize();

	/**
	 * Get a channel for a warning/error message.
	 * If necessary the requested channel is created first.
	 * All channels will be deleted by the destrutor.
	 * @param mt type of the channel
	 * @return pointer to the channel to be used for messages of
	 * the given type.
	 **/
	XmlChannel *errorChannel(ErrorType mt) const;

	void errorOccured(ErrorType) const;

	/**
	 * Abort program execution.
	 * exit() will print a message and call Application::exit() to
	 * terminate the programm.  The exit status of the program can
	 * be configured by the "error-code" parameter.
	 **/
	void exit() const;

    protected:
	/**
	 * Output a warning/error message on the appropriate channel.
	 * @param mt type of the message
	 * @param msg a printf-like format string
	 * @return the channel the message was written to.
	 **/
	virtual XmlChannel *vErrorMessage(ErrorType mt, const char *msg, va_list) const;

    public:
	/**
	 * Helper class for generating error messages.
	 *
	 * The destructor of Message will trigger appropriate
	 * error action.  Consider the following example:
	 *
	 *    criticalError("operation XYZ failed");
	 *    criticalError("additional information about failure: %s", info);
	 *
	 * The additional information will never get printed because
	 * the program terminates after the first call to critical
	 * error.  Instead you can write:
	 *
	 *    criticalError("operation XYZ failed")
	 *        << "additional information about failure: " << info;
	 *
	 * This will work fine: criticalError() return an instance of
	 * Message, which is used to output the additional
	 * information.  After that the Message instance is
	 * destroyed, which (normally) causes the program to
	 * terminate.  Similar consideration apply to the proper
	 * formatting of XML style messages.
	 *
	 * Normally you will not need to mention Message
	 * explicitly, but for long, extensive messages you can do
	 * something like this:
	 *
	 *    {
	 *       Message m(error("error text"));
	 *       for (...) m << values;
	 *    }
	 *
	 * Note: Message uses an std::auto_ptr<> style copy policy.
	 *
	 * Rule: Call warning(), error() or criticalError() once per
	 * event you wish to report.  Use Message to avoid
	 * multiple calls.
	 */

	class Message {
	private:
	    XmlWriter *ostream_;
	    const Component *component_;
	    ErrorType type_;
	private:
	    friend class Component;
	    Message(const Component *c, ErrorType type, XmlChannel *ch) :
		ostream_(ch), component_(c), type_(type) {}
	public:
	    operator XmlWriter&() const {
		return *ostream_;
	    }
	    template <typename T> XmlWriter& operator<<(const T &v) {
		*ostream_ << v; return *ostream_;
	    }

	    Message& form(const char *msg, ...)
		__attribute__ ((format (printf, 2, 3)));

	    Message(const Message &m) :
		ostream_(m.ostream_), component_(m.component_), type_(m.type_)
	    {
		const_cast<Message&>(m).component_ = 0;
	    }

	    ~Message();
	};

    public:
	XmlWriter &clog() const { return *errorChannel(ErrorTypeInfo); }

	/**
	 * Print an information message.
	 * Use this for all information you what to give the use on
	 * what your are currently doing.  If anything abnormal occurs
	 * use waring() or error() instead.
	 * @param msg a printf-like format string
	 * @return The stream output operator may be applied to the return
	 * value.
	 * A line break is appended automatically.
	 * @see warning(), error(), criticalError()
	 **/
	Message log(const char *msg, ...) const
	    __attribute__ ((format (printf, 2, 3)));
	Message log() const;

	/**
	 * Print a warning message.
	 * @param msg a printf-like format string
	 * @return The stream output operator may be applied to the return
	 * value.
	 * Message should not include formatting characters ("\n" etc.).
	 * A line break is appended automatically.
	 * Example:
	 * warning("mismatch ") << a << "!=" << b;
	 **/
	Message warning(const char *msg = 0, ...) const
	    __attribute__ ((format (printf, 2, 3)));
	Message vWarning(const char *msg, va_list) const;

	/**
	 * Print an error message.
	 * Call error() whenever an error occurs in the current
	 * component.  The default behaviour is to continue until
	 * respondToDelayedErrors() is called.  Clients can check if
	 * an error occurred by calling hasFatalErrors().
	 * @param msg a printf-like format string
	 * @return The stream output operator may be applied to the return
	 * value.
	 * Message should not include formatting characters ("\n" etc.).
	 * A line break is appended automatically.
	 * @see warning()
	 * @see respondToDelayedErrors()
	 **/
	Message error(const char *msg = 0, ...) const
	    __attribute__ ((format (printf, 2, 3)));
	Message vError(const char *msg, va_list) const;

	/**
	 * Print a critical error message.
	 * Call criticalError() when an error occurs and there is no
	 * way to continue in a meaningful way.  The default behaviour
	 * is to abort execution immediatelly.  You should assume,
	 * that criticalError() does not return, although the user can
	 * overide this behaviour.
	 * @param msg a printf-like format string
	 * @return The stream output operator may be applied to the return
	 * value.
	 * Message should not include formatting characters ("\n" etc.).
	 * A line break is appended automatically.
	 * @see warning()
	 **/
	Message criticalError(const char *msg = 0, ...) const
	    __attribute__ ((format (printf, 2, 3)));
	Message vCriticalError(const char *msg, va_list) const;

    public:
	Component(const Configuration &c);
	Component(const Component &);
	virtual ~Component();

	Component &operator=(const Component &);

	/**
	 * Test whether non-ignored errors have occured.
	 * @see error()
	 */
	bool hasFatalErrors() const;

	/**
	 * Terminate execution iff an error has occured previously.
	 * More specifically, terminate iff hasFatalErrors() returns
	 * true.
	 * @see error()
	 */
	void respondToDelayedErrors() const;

    };

}

#endif // _CORE_COMPONENT
