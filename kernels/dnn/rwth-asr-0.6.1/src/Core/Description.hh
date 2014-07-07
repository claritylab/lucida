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
#ifndef _CORE_DESCRIPTION_HH
#define _CORE_DESCRIPTION_HH

#include "Application.hh"
#include "Dependency.hh"

namespace Core {

    /** Vector of stream descriptions
     * Stream descriptions if a set of attributes describing a (feature) stream.
     * For example: type, dimension, etc.
     */
    class Description {
    public:
	class Stream  {
	private:
	    std::string name_;
	    StringHashMap<std::string> attributes_;
	public:
	    Stream(const std::string &name) : name_(name) {}

	    /** Set the value of attribute @param name. */
	    template<class ValueType>
	    void setValue(const std::string &name, const ValueType &value) {
		std::ostringstream s; s << value; ensure(s);
		attributes_[name] = s.str();
	    }

	    /** Retrieves the value of attribute @param name.
	     *  If attribute @param name is not found, result is false.
	     */
	    bool getValue(const std::string &name, std::string &value) const;
	    /** Retrieves the value of attribute @param name.
	     *  If attribute @param name is not found, error is raised.
	     *  In case of an error, if @c critical is true, program terminates.
	     */
	    std::string getValue(const std::string &name, bool critical = true) const;
	    /** Retrieves the value of attribute @param name.
	     *  If attribute @param name is not found, result is false.
	     */
	    template<class ValueType>
	    bool getValue(const std::string &name, ValueType &value) const;

	    /** Compares the value of the attribute @param name with the @param expected.
	     *  If @param critical is true and comparison fails, execution is aborted
	     */
	    template<class ValueType>
	    bool verifyValue(const std::string &name,
			     const ValueType &expected,
			     bool critical = false) const;
	    void getDependencies(const std::string &prefix, std::string &value) const;

	    bool operator==(const Stream &s) const { return attributes_ == s.attributes_; }
	};
    protected:
	std::string name_;
	std::vector<Stream> streams_;
    protected:
	Stream &getStream(size_t index);
    public:
	static std::string prepareName(const std::string &parent, const std::string &name) {
	    return Configuration::prepareResourceName(parent, name);
	}
    public:
	/**
	 *  Constructor.
	 *  Since this object might be created frequently,
	 *  it does not derive from Component.
	 *  Error messages are passed to the Application object.
	 *  @c name is used to identify this object.
	 */
	Description(const std::string &name) : name_(name) {}

	Stream &operator[](size_t index) { return getStream(index); }
	const Stream &operator[](size_t index) const {
	    require(index < streams_.size()); return streams_[index];
	}

	Core::Description::Stream &mainStream() { return getStream(0); }
	const Core::Description::Stream &mainStream() const {
	    require(!streams_.empty()); return streams_.front();
	}

	void clear() { streams_.clear(); }

	size_t nStreams() const { return streams_.size(); }
	/** Compares the number of streams with @param expectedNumber.
	 *  If @param critical is true and comparison fails, execution is aborted
	 */
	bool verifyNumberOfStreams(size_t expectedNumber, bool critical = false) const;

	void getDependencies(DependencySet &) const;

	bool operator==(const Description &d) const { return streams_ == d.streams_; }
	bool operator!=(const Description &d) const { return !operator==(d); }
    };

    template<class ValueType>
    bool Description::Stream::getValue(const std::string &name, ValueType &value) const {
	std::string v;
	if (getValue(name, v)) {
	    std::istringstream s(v);
	    s >> value;
	    ensure(s);
	    return true;
	}
	return false;
    }

    template<class ValueType>
    bool Description::Stream::verifyValue(
	const std::string &name, const ValueType &expected, bool critical) const {
	ValueType value; getValue(name, value);
	if (value != expected) {
	    Application::us()->error() << name_ << ":mismatch in value '" << name
				       << "': expected '" << expected
				       << "' retrieved '" << value << "'.";
	    if (critical) Application::us()->respondToDelayedErrors();
	    return false;
	}
	return true;
    }

} //namespace Core

#endif // _CORE_DESCRIPTION_HH
