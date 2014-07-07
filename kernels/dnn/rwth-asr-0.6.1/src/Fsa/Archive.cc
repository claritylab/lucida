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
#include "Archive.hh"
#include "Input.hh"
#include "Output.hh"
#include "Static.hh"
#include "Types.hh"

using namespace Fsa;


// -----------------------------------------------------------------------------
const Core::ParameterString Archive::paramAlphabetFilename(
    "alphabet-filename",
    "filename of alphabet",
    "alphabets.binfsa.gz");

const Core::ParameterString Archive::paramPath(
    "path",
    "where to store the automaton archive");

Archive::Archive(
    const Core::Configuration &config,
    const std::string &path,
    Core::Archive::AccessMode accessMode) :
    Core::Component(config),
    archive_(0)
{
    std::string pathname = path;
    if (!pathname.size())
	pathname = paramPath(config);

    archive_ = Core::Archive::create(config, pathname, accessMode);
    if (!archive_) {
	error("failed to open fsa archive \"%s\"", pathname.c_str());
	return;
    }
    log("opened fsa archive \"%s\"", pathname.c_str());
}

Archive::~Archive() {
    delete archive_;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
const Core::ParameterInt ArchiveReader::paramReportUnknowns(
    "report-unknowns",
    "maximum number of unknown symbols that should be reported",
    10,
    0);

const Core::ParameterInt ArchiveReader::paramMapUnknownsToIndex(
    "map-unknowns-to-index",
    "unknown indices are mapped to this index",
    InvalidLabelId);

ArchiveReader::ArchiveReader(
    const Core::Configuration &config,
    const std::string &pathname)
    :
    Archive(config, pathname, Core::Archive::AccessModeRead),
    reportUnknowns_(paramReportUnknowns(config)),
    unknownId_(paramMapUnknownsToIndex(config))
{
    if (!archive_) return;
    loadAlphabets();
}

ArchiveReader::ArchiveReader(const Core::Configuration &config) :
    Archive(config, "", Core::Archive::AccessModeRead)
{
    if (!archive_) return;
    loadAlphabets();
}

void ArchiveReader::loadAlphabets() {
    const std::string alphabetFilename = paramAlphabetFilename(config);
    if (!archive_->hasFile(alphabetFilename)) {
	error("cannot find alphabet file \"%s\" in archive",
	      alphabetFilename.c_str());
	return;
    }
    StaticAutomaton staticAutomaton;
    Core::ArchiveReader is(*archive_, alphabetFilename);
    if (!readBinary(&staticAutomaton, is)) {
	error("failed to read alphabets from archive");
	return;
    }
    log("read alphabets from archive");
    inputAlphabet_  = staticAutomaton.getInputAlphabet();
    outputAlphabet_ = staticAutomaton.getOutputAlphabet();
}

void ArchiveReader::setSemiring(ConstSemiringRef targetSemiring) {
    semiring_ = targetSemiring;
}

void ArchiveReader::mapInput(ConstAlphabetRef targetAlphabet) {
    mapAlphabet(
	inputAlphabet_,
	targetAlphabet,
	inputMapping_,
	unknownId_,
	reportUnknowns_);
}

void ArchiveReader::mapOutput(ConstAlphabetRef targetAlphabet) {
    mapAlphabet(
	outputAlphabet_,
	targetAlphabet,
	outputMapping_,
	unknownId_,
	reportUnknowns_);
}

ConstAutomatonRef ArchiveReader::get(const std::string & id)
{
    require(archive_);
    Core::Ref<StorageAutomaton> storage(new StaticAutomaton);

    // these will be overwritten if they are contained in the file
    storage->setType(TypeTransducer);
    storage->setSemiring(semiring_);
    if (inputAlphabet_) {
	storage->setInputAlphabet(inputAlphabet_);
    }
    if (outputAlphabet_) {
	storage->setOutputAlphabet(outputAlphabet_);
    }

    if (!archive_->hasFile(id)) {
	error("Could not find fsa \"%s\"", id.c_str());
	return ConstAutomatonRef();
    }
    log("Reading fsa \"%s\"", id.c_str());
    Core::ArchiveReader is(*archive_, id);
    if (!readBinary(storage.get(), is)) {
	error("Failed to read fsa");
	return ConstAutomatonRef();
    }

    ConstAutomatonRef result = storage;
    if (result->type() == TypeTransducer) {
	result = Fsa::mapInput (result, inputMapping_);
	result = Fsa::mapOutput(result, outputMapping_);
    } else if (result->type() == TypeAcceptor) {
	if (inputMapping_.to()) {
	    result = mapInputOutput(result, inputMapping_);
	}
    }

    return result;
}
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
ArchiveWriter::ArchiveWriter(
    const Core::Configuration & config,
    const std::string & pathname,
    ConstAlphabetRef inputAlphabet,
    ConstAlphabetRef outputAlphabet)
    :
    Archive(config, pathname, Core::Archive::AccessModeWrite)
{
    inputAlphabet_  = inputAlphabet;
    outputAlphabet_ = outputAlphabet;
    storeAlphabets();
}

ArchiveWriter::ArchiveWriter(
    const Core::Configuration & config,
    ConstAlphabetRef inputAlphabet,
    ConstAlphabetRef outputAlphabet)
    :
    Archive(config, "", Core::Archive::AccessModeWrite)
{
    inputAlphabet_  = inputAlphabet;
    outputAlphabet_ = outputAlphabet;
    storeAlphabets();
}

void ArchiveWriter::storeAlphabets() {
    Core::Ref<StorageAutomaton> f(new StaticAutomaton(TypeTransducer));
    f->setInputAlphabet(inputAlphabet_);
    f->setOutputAlphabet(outputAlphabet_);
    const std::string alphabetFilename = paramAlphabetFilename(config);
    Core::ArchiveWriter os(*archive_, alphabetFilename, true);
    writeBinary(f, os, Fsa::storeAlphabets);
    if (!os)
	error("Failed to write alphabets to \"%s\"", alphabetFilename.c_str());
}

void ArchiveWriter::store(const std::string &id, ConstAutomatonRef f) {
    require(archive_);
    StoredComponents what = storeAll;
    if (f->inputAlphabet()  == inputAlphabet_)
	what &= ~storeInputAlphabet;
    if (what & storeInputAlphabet)
	warning("store input alphabet");
    if (f->outputAlphabet() == outputAlphabet_)
	what &= ~storeOutputAlphabet;
    if (what & storeOutputAlphabet && outputAlphabet_)
	warning("store output alphabet");

    info(f, log("Writing fsa \"%s\"", id.c_str()));
    Core::ArchiveWriter os(*archive_, id, true);
    writeBinary(f, os, what);
    if (!os)
	error("Failed to write fsa \"%s\"", id.c_str());
}

// -----------------------------------------------------------------------------
