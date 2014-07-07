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
#ifndef _LATTICE_ARCHIVE_HH
#define _LATTICE_ARCHIVE_HH

#include "Lattice.hh"
#include <Bliss/Lexicon.hh>
#include <Core/Archive.hh>
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/Types.hh>
#include <Core/Version.hh>
#include <Fsa/Archive.hh>
#include <Fsa/Alphabet.hh>
#include <Fsa/Automaton.hh>


namespace Lattice {

    class ArchiveReader;
    class ArchiveWriter;

    class Archive :
	public virtual Core::Component {
    public:
	static const char wordBoundarySuffix[];
	static const char latticeConfigFilename[];
	static const Core::Choice formatChoice;
	static const Core::ParameterChoice paramFormat;
	static const Core::ParameterString paramPath;
	static const Core::Choice alphabetChoice;
	static const Core::ParameterChoice paramAlphabet;

    protected:
	Bliss::LexiconRef lexicon_;
	std::string pathname_;
	std::string suffix_;
	Core::Configuration latticeConfig_;
    public:
	Archive(const Core::Configuration&,
		const std::string &pathname,
		Bliss::LexiconRef);
	virtual ~Archive() {}

	void setSuffix(const std::string &suffix) { suffix_ = suffix; }
	const std::string &suffix() const { return suffix_; }

	static ArchiveReader *openForReading(
	    const Core::Configuration&, Bliss::LexiconRef);

	static ArchiveWriter *openForWriting(
	    const Core::Configuration&, Bliss::LexiconRef);
    };

    class ArchiveReader :
	public Archive {
    private:
	typedef ArchiveReader Self;
	typedef Archive Precursor;

	static const Core::ParameterBool paramCapitalizeTranscriptions;
	static const Core::ParameterFloat paramAmScale;
	static const Core::ParameterFloat paramLmScale;
	static const Core::ParameterFloat paramPenaltyScale;
	static const Core::ParameterFloat paramPronunciationScale;

    public:
	typedef Core::Archive::const_iterator const_iterator;

    protected:
	void loadLatticeConfiguration();

	f64 amScale() {
	    return paramAmScale(config, paramAmScale(latticeConfig_));
	}
	f64 lmScale() {
	    return paramLmScale(config, paramLmScale(latticeConfig_));
	}
	f64 penaltyScale() {
	    return paramPenaltyScale(config, paramPenaltyScale(latticeConfig_));
	}
	f64 pronunciationScale() {
	    return paramPronunciationScale(config, paramPronunciationScale(latticeConfig_));
	}

    public:
	ArchiveReader(
	    const Core::Configuration&,
	    const std::string &pathname,
	    Bliss::LexiconRef);
	virtual ~ArchiveReader() {}

	virtual bool hasFile(const std::string &file) const = 0;
	virtual const_iterator files() const = 0;

	ConstWordLatticeRef get(const std::string &id) { return get(id, WordLattice::acousticFsa); }
	ConstWordLatticeRef get(const std::string &id, const std::string &name) {
	    return get(id, std::vector<std::string>(1, name));
	}
	virtual ConstWordLatticeRef get(
	    const std::string &id, const std::vector<std::string> &names) = 0;
    };

    class ArchiveWriter :
	public Archive {
    private:
	typedef ArchiveWriter Self;
	typedef Archive Precursor;
    public:
	ArchiveWriter(
	    const Core::Configuration&,
	    const std::string &pathname,
	    Bliss::LexiconRef);
	virtual ~ArchiveWriter() {}

	virtual void store(const std::string &id, ConstWordLatticeRef wordLattice) = 0;
    };

} // namespace Lattice

#endif // _LATTICE_ARCHIVE_HH
