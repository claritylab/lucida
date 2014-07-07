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
#ifndef _FSA_ARCHIVE_HH
#define _FSA_ARCHIVE_HH

#include <Core/Archive.hh>
#include <Core/Assertions.hh>
#include <Core/Component.hh>

#include "Alphabet.hh"
#include "Automaton.hh"
#include "Basic.hh"
#include "Semiring.hh"
#include "Storage.hh"


namespace Fsa {
    class Archive :
	public Core::Component
    {
    public:
	typedef Core::Archive::const_iterator const_iterator;

	static const Core::ParameterString paramAlphabetFilename;
	static const Core::ParameterString paramPath;

    protected:
	Core::Archive *archive_;
	ConstAlphabetRef inputAlphabet_;
	ConstAlphabetRef outputAlphabet_;

    public:
	Archive(
	    const Core::Configuration &config,
	    const std::string &pathname,
	    Core::Archive::AccessMode accessMode);

	~Archive();

	Core::Archive *archive() { return archive_; }

	bool hasFile(const std::string & file) {
	    require(archive_);
	    return archive_->hasFile(file);
	}
	const_iterator files() {
	    require(archive_);
	    return archive_->files();
	}
    };

    class ArchiveReader : public Archive {
    private:
	static const Core::ParameterInt paramReportUnknowns;
	static const Core::ParameterInt paramMapUnknownsToIndex;
    private:
	ConstSemiringRef semiring_;
	AlphabetMapping  inputMapping_;
	AlphabetMapping  outputMapping_;
	u32 reportUnknowns_;
	LabelId unknownId_;
	void loadAlphabets();
    public:
	ArchiveReader(
	    const Core::Configuration &config,
	    const std::string &pathname);
	ArchiveReader(
	    const Core::Configuration &config);

	void setSemiring(ConstSemiringRef);
	void mapInput(ConstAlphabetRef);
	void mapOutput(ConstAlphabetRef);

	ConstAutomatonRef get(const std::string &id);
    };

    class ArchiveWriter : public Archive {
    private:
	void storeAlphabets();
    public:
	ArchiveWriter(
	    const Core::Configuration &config,
	    const std::string &pathname,
	    ConstAlphabetRef inputAlphabet,
	    ConstAlphabetRef outputAlphabet = ConstAlphabetRef());
	ArchiveWriter(
	    const Core::Configuration &config,
	    ConstAlphabetRef inputAlphabet,
	    ConstAlphabetRef outputAlphabet = ConstAlphabetRef());

	void store(const std::string &id, ConstAutomatonRef lattice);
    };


} // namespace Fsa

#endif // _FSA_ARCHIVE_HH
