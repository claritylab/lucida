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
#include <Core/Application.hh>

#include "Convert.hh"
#include "RescoreInternal.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    namespace {
	const Core::ParameterString paramRescoreMode(
	    "rescore-mode",
	    "rescore mode",
	    "clone");
    } // namespace
    RescoreMode getRescoreMode(const Core::Configuration &config) {
	RescoreMode rescoreMode = getRescoreMode(paramRescoreMode(config));
	switch (rescoreMode) {
	case RescoreModeInPlaceCache:
	    Core::Application::us()->warning(
		"Cached in-place rescoring can have side effects in conjunction with non-linear command networks.");
	    break;
	case RescoreModeInPlace:
	    Core::Application::us()->warning(
		"In-place rescoring can have side effects. "
		"USE WITH CARE: amongst others, side effects will show up for all nodes traversing a lattice and not caching the visited states, e.g. \"info\".");
	    break;
	default:
	    break;
	}
	return rescoreMode;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    RescoreNode::RescoreNode(
	const std::string &name, const Core::Configuration &config) :
	Precursor(name, config) {}

    bool RescoreNode::init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments) {
	bool b = Precursor::init_(crawler, arguments);
	setRescoreMode(getRescoreMode(config));
	// log("Rescore mode is %s.", getRescoreModeName(rescoreMode).c_str());
	return b;
    }

    void RescoreNode::setRescoreMode(RescoreMode _rescoreMode) {
	rescoreMode = _rescoreMode;
    }

    ConstLatticeRef RescoreNode::filter(ConstLatticeRef l) {
	if (!l)
	    return l;
	return rescore(l);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    const Core::ParameterString RescoreSingleDimensionNode::paramKey(
	"key",
	"The semantic of key depends on append: "
	"if append is true, the appended dimension gets that name; "
	"if not, the dimension with that name is extended.",
	"");
    const Core::ParameterFloat RescoreSingleDimensionNode::paramScale(
	"scale",
	"The semantic of scale depends on append: "
	"if append is true, the appended dimension gets that scale; "
	"if not, the score is scaled before extension takes place.",
	Semiring::DefaultScale);
    const Core::ParameterBool RescoreSingleDimensionNode::paramAppend(
	"append",
	"append a new dimension to store rescoring results",
	false);

    RescoreSingleDimensionNode::RescoreSingleDimensionNode(const std::string &name, const Core::Configuration &config) :
	Precursor(name, config) {}

    bool RescoreSingleDimensionNode::init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments) {
	bool b = Precursor::init_(crawler, arguments);
	append_ = paramAppend(config);
	key_ = paramKey(config);
	if (key_.empty())
	    criticalError("RescoreNode: You forgot to specify a key.");
	Score scale = paramScale(config);
	if (append_) {
	    appendScale_ = scale;
	    scoreScale_ = Semiring::DefaultScale;
	} else {
	    appendScale_ = 0.0;
	    scoreScale_ = scale;
	}
	return b;
    }

    ConstLatticeRef RescoreSingleDimensionNode::rescore(ConstLatticeRef l) {
	ScoreId id;
	if (append_) {
	    id = l->semiring()->size();
	    if (!lastSemiring_ || (lastSemiring_.get() != l->semiring().get())) {
		lastSemiring_ = l->semiring();
		lastExtendedSemiring_ = appendSemiring(lastSemiring_, appendScale_, key_);
		if (lastExtendedSemiring_->key(lastExtendedSemiring_->size() - 1) !=  key_)
		    error("Failed to set key \"%s\" for dimension %zu; probably key is already in use.",
			  key_.c_str(), (lastExtendedSemiring_->size() - 1));
	    }
	    l = offsetSemiring(l, lastExtendedSemiring_, 0);
	    verify(id == l->semiring()->size() - 1);
	} else {
	    id = l->semiring()->id(key_);
	    if (!l->semiring()->hasId(id)) {
		if (!Core::strconv(key_, id))
		    id = Semiring::InvalidId;
		if (!l->semiring()->hasId(id)) {
		    criticalError("Could not rescore lattice \"%s\"; dimension \"%s\" not found",
			  l->describe().c_str(), key_.c_str());
		    return l;
		}
	    }
	}
	return rescore(l, id);
    }
    // -------------------------------------------------------------------------

} // namespace Flf
