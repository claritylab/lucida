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
#include "Confidences.hh"
#include <Bliss/CorpusDescription.hh>
#include <Flow/DataAdaptor.hh>

using namespace Speech;

/**
 *  Confidences
 */
const Core::ParameterFloat Confidences::paramThreshold(
    "threshold",
    "threshold to cut-off confidences",
    1, 0, 1);

Confidences::Confidences(const Core::Configuration &c) :
    Precursor(c),
    alignment_(0),
    threshold_(paramThreshold(config))
{
    statistics_.nZero = 0;
    statistics_.nOne = 0;
}

Confidences::~Confidences()
{
    dumpStatistics();
    clear();
}

void Confidences::updateStatistics(const Alignment &alignment)
{
    for (u32 t = 0; t < alignment.size(); ++ t) {
	alignment[t].weight < threshold_ ? ++ statistics_.nZero : ++ statistics_.nOne;
    }
}

void Confidences::dumpStatistics()
{
    log(Core::form("statistics: %d with weight '0' and %d with weight '1'",
		   statistics_.nZero, statistics_.nOne).c_str());
}

void Confidences::clear()
{
    delete alignment_;
    alignment_ = 0;
}

void Confidences::setAlignment(const Alignment *alignment)
{
    clear();
    alignment_ = alignment;
    updateStatistics(*alignment_);
}

Mm::Weight Confidences::operator[](TimeframeIndex t) const
{
    if (t < alignment_->size()) {
	require(t == (*alignment_)[t].time);
	return ((*alignment_)[t].weight >= threshold_) ? 1 : 0;
    } else {
	error("confidence (size=") << alignment_->size() << ") shorter than feature stream";
	return 0;
    }
}

bool Confidences::isValid() const
{
    return alignment_;
}

/**
 *  ConfidenceArchive
 */
ConfidenceArchive::ConfidenceArchive(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

ConfidenceArchive::~ConfidenceArchive()
{}

void ConfidenceArchive::get(Confidences &result, const std::string &id)
{
    result.clear();
    if (!hasAccess(Core::Archive::AccessModeRead)) {
	if (!open(Core::Archive::AccessModeRead)) {
	    error("failed to open archive '") << path_.c_str() << "' for reading";
	    return;
	}
    }
    if (!hasAccess(Core::Archive::AccessModeRead)) {
	error("failed to open archive '") << path_.c_str() << "' for reading";
    }
    if (!archive_->hasFile(id)) {
	error("file '") << id << "' not found";
	return;
    }
    Flow::CacheReader *reader = newReader(id);
    if (!reader) {
	error("cannot open cache for '") << id << "'";
	return;
    }
    Flow::Data *data = reader->getData();
    Flow::DataAdaptor<Alignment> *alignment =
	dynamic_cast<Flow::DataAdaptor<Alignment>* >(data);
    require(alignment);
    result.setAlignment(new Alignment(alignment->data()));
    delete reader;
    //    delete alignment;
}

void ConfidenceArchive::get(Confidences &result, Bliss::SpeechSegment *s)
{
    get(result, s->fullName());
}
