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
#include "LatticeHandler.hh"
#include <Lattice/Archive.hh>
#include <Lattice/LatticeAdaptor.hh>

using namespace Search;

const Core::Choice LatticeHandler::choiceLatticeFormat(
	"default", formatDefault,
	"fsa", formatDefault,
	"openfst", formatOpenFst,
	"flf", formatFlf,
	Core::Choice::endMark());

const Core::ParameterChoice LatticeHandler::paramLatticeFormat(
	"format", &choiceLatticeFormat, "lattice format", formatDefault);

LatticeHandler::~LatticeHandler()
{
    delete reader_;
    delete writer_;
}

bool LatticeHandler::createReader()
{
    if (!reader_) {
	verify(lexicon_);
	reader_ = ::Lattice::Archive::openForReading(config, lexicon_);
	if (reader_->hasFatalErrors()) {
	    delete reader_;
	    reader_ = 0;
	}
    }
    return reader_;
}

bool LatticeHandler::createWriter()
{
    if (!writer_) {
	verify(lexicon_);
	writer_ = ::Lattice::Archive::openForWriting(config, lexicon_);
	if (writer_->hasFatalErrors()) {
	    delete writer_;
	    writer_ = 0;
	}
    }
    return writer_;
}

bool LatticeHandler::write(const std::string &id, const WordLatticeAdaptor &l)
{
    if (!createWriter()) return false;
    if (format_ != formatDefault) {
	error("cannot write lattice in format '%s'",
		choiceLatticeFormat[format_].c_str());
	return false;
    }
    writer_->store(id, l.get());
    return true;
}

Core::Ref<LatticeAdaptor> LatticeHandler::read(const std::string &id, const std::string &name)
{
    if (!createReader()) return Core::Ref<LatticeAdaptor>();
    ::Lattice::ConstWordLatticeRef lattice = reader_->get(id, name);
    return Core::ref(new WordLatticeAdaptor(lattice));
}

LatticeHandler::ConstWordLatticeRef LatticeHandler::convert(const WordLatticeAdaptor &l) const {
    return l.get();
}
