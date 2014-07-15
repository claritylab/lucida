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
#include "SegmentwiseFeatureExtractor.hh"

using namespace Speech;

const Core::ParameterBool SegmentwiseFeatureExtractor::paramNoDependencyCheck(
	"no-dependency-check",
	"do not check any dependencies",
	false
	);

SegmentwiseFeatureExtractor::SegmentwiseFeatureExtractor(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    noDependencyCheck_(paramNoDependencyCheck(c))
{}


Flow::PortId SegmentwiseFeatureExtractor::addPort(const std::string &name)
{
    Flow::PortId portId = dataSource()->getOutput(name);
    if (portId != Flow::IllegalPortId) {
	log("Output named \"%s\" added to flow network", name.c_str());
	featureStreams_.insert(std::make_pair(portId, SegmentwiseFeaturesRef(new SegmentwiseFeatures())));
    } else {
	error("Flow network does not have an output named \"%s\"", name.c_str());
    }
    return portId;
}

void SegmentwiseFeatureExtractor::checkCompatibility(
    Flow::PortId port,
    Core::Ref<const Am::AcousticModel> acousticModel) const
{
    if (!noDependencyCheck_){
    if (!features(port)->empty() &&
	!acousticModel->isCompatible(
	    Mm::FeatureDescription(*this, *features(port)->front()))) {
	acousticModel->respondToDelayedErrors();
	}
    }
}

ConstSegmentwiseFeaturesRef SegmentwiseFeatureExtractor::features(
    Flow::PortId port) const
{
    require(featureStreams_.find(port) != featureStreams_.end());
    return featureStreams_.find(port)->second;
}

bool SegmentwiseFeatureExtractor::valid() const
{
    for (FeatureStreams::const_iterator i = featureStreams_.begin(); i != featureStreams_.end(); ++ i) {
	if (!valid(i->first)) {
	    return false;
	}
    }
    return true;
}

void SegmentwiseFeatureExtractor::processSegment(Bliss::Segment* segment)
{
    for (FeatureStreams::iterator i = featureStreams_.begin(); i != featureStreams_.end(); ++ i) {
	if (!dataSource()->getData(i->first, *i->second))
	    warning("Feature stream '%s' is empty.", dataSource()->outputName(i->first).c_str());
    }
}

void SegmentwiseFeatureExtractor::signOn(CorpusVisitor &corpusVisitor)
{
    if (!corpusVisitor.isSignedOn(dataSource())) {
	Precursor::signOn(corpusVisitor);
    }
}
