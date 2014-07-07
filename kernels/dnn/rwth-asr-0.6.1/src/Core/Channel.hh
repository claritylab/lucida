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
// $Id: Channel.hh 6261 2006-12-12 15:08:44Z rybach $

#ifndef _CORE_CHANNEL_HH
#define _CORE_CHANNEL_HH

#include <Core/Configurable.hh>
#include <Core/Hash.hh>
#include <Core/Parameter.hh>
#include <Core/StringUtilities.hh>
#include <Core/Thread.hh>
#include <Core/Utility.hh>
#include <Core/XmlStream.hh>
#include <fstream>
#include <iostream>
#include <list>


namespace Core {

    /**
     * @page Channels
     *
     * Instead of using cout and cerr directly you should use
     * channels.  Channels are named output streams that can be
     * configured and redirected individually.  Channels also take
     * care of character set conversion and (optionally) XML
     * formatting.  (Using cerr is only allowed in those places of
     * Core where the channel sub-system has not been set up yet.
     *
     * Channels cause NO (!) overhead compared to standard streams if
     * you use only a single destination stream.  Multiple destination
     * streams through redirection have the overhead of making multiple
     * copies of the data to be written (conversion to character strings
     * occurs only once!)
     *
     * Upon creation a Channel determines its set of targets via the
     * standard configuration process.  E.g. if your component is
     * called "recognizer" then Channel(config, "statistics"), will
     * look for a resource matching "recognizer.statistics.channel".
     * The resource's value is interpreted as a comma separated list
     * of target names, specifying were the channel's data is sent.
     *
     * The following channel targets are predefined:
     * -# "stdout" standard output of process
     * -# "stderr" standard error output of process
     * -# "nil"    suppress output
     *
     * All other target names cause the channel manager to create
     * additional targets as needed.  Each target is a Configurable
     * registered under "<application-title>.channels.<target-name>".
     * If the target name contains a dot, only the part after the dot
     * is used as parameter name.  By default a channel target will
     * open a file by its own name and write all output to this
     * file. This can be overridden with the "file" parameter.
     *
     * Channel targets can be configured in several ways:
     * -# The file name can be changed to something different from the
     *    target name using the "file" parameter.
     * -# If the file already exists it is overwritten by default.
     *    By setting "append" to true, the channel manager will append
     *    to the file instead of overwriting.
     * -# File output is buffered by default, which can cause long
     *    delay in the output.  Set "unbuffered" to change to
     *    line-buffering mode.
     * -# Internally channels expect to be provided with UTF-8 encoded
     *    character data.  (Plain ASCII is a subset of UTF-8.)  It is
     *    possible to specify a specific output encoding for each channel
     *    target, by setting the parameter "encoding" (ISO-8859-1 by
     *    default).  The channel will convert the data into this
     *    character set encoding upon output.
     * -# Channels feature automatic word-wrapping, which is disabled
     *    by default.  To enable word-wrapping, set the parameter
     *    "margin" to the number of characters per line.
     * -# XML (and possibly other) text is automatically indented.
     *    The parameter "indentation" controls the depth of indentation.
     *    Naturally, a value of zero, disable auto-indentation.
     * -# zlib compression can be activated using the "compressed" parameter.
     *    The filename will be extended by the suffix ".gz" if not already
     *    present
     *
     * You can check whether a channel's output is actually used by calling
     * isOpen().  Make use of this especially if your output needs additional
     * calculations.
     *
     * For plain text output use Channel.  If you want to produce XML
     * output, use the derived class XmlChannel.
     **/


    /**
     * Configurable text output stream.
     * @see Channels
     */

    class Channel :
	public std::ostream
    {
    public:
	enum TargetType { plainTarget, xmlTarget };
	enum Default { disabled, standard, error };
	static const Core::ParameterString paramTargets;
    private:
	class Target;
	class XmlTarget;
	class Dispatcher;
	bool isOpen_;
    public:
	Channel(const Configuration&,
		const std::string &name,
		Default defaultTarget = disabled,
		TargetType __type = plainTarget);
	virtual ~Channel();

	/** Test whether channel output is actually used. */
	bool isOpen() const { return isOpen_; }

	class Manager;
    };

    /**
     * Configurable XML output stream.
     */
    class XmlChannel :
	public XmlWriter
    {
    private:
	Channel channel_;
    public:
	XmlChannel(const Configuration&,
		   const std::string &name,
		   Channel::Default defaultTarget = Channel::disabled);
	virtual ~XmlChannel();

	bool isOpen() const { return channel_.isOpen(); }
    };

    /**
     * Central management of channel output.
     * The manager is usually owned the Core::Application instance.
     * Singleton: this class must be instantiated only once.
     */

    class Channel::Manager :
	public Configurable,
	private Core::Mutex
    {
    private:
	static const Core::ParameterInt paramBlockedTargetBufferLimit;

	static Manager *singleton_;
	u32 blockedTargetBufferLimit_;
	std::streambuf  *originalStreamBuffers[3];
	typedef StringHashMap<Channel::Target*> TargetMap;
	TargetMap targets_;
	typedef std::list<Channel::Target*> TargetList;
	TargetList ttyTargets_;
	Channel::Target *createTarget(
	    TargetType, const Core::Configuration&, const std::string &defaultFilename);
    public:
	Manager(const Core::Configuration&, bool outputXmlHeader);
	~Manager();

	/** Return the (single) instance. */
	static Manager *us();

	Target *get(TargetType, const std::string&);

	/**
	 * Write any pending output on all channels.
	 **/
	void flushAll();

	/**
	 * Write any pending output on channels connected to the
	 * terminal.
	 */
	void flushTty();

	/**
	 * Temporarily block all output on the terminal.  Also flushes buffers.
	 * This is used to prevent corrupting the progress indicator.
	 * You MUST release the TTY targets with unblockTty().
	 * Between calls to blockTty() and unblockTty(), output to TTY
	 * targets is retained in internal buffers.  (However, the
	 * implementation is not strict: If the buffers grow unduely
	 * large they are flushed anyway.  The limit in configurable.)
	 */
	void blockTty();

	/**
	 * Re-enable TTY targets.
	 */
	void unblockTty();
    };



} // namespace Core

#endif // _CORE_CHANNEL_HH
