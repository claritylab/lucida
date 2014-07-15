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
#include "AlignmentWithLinearSegmentation.hh"
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include "ModelCombination.hh"
#include "AlignerModelAcceptor.hh"
#include "AllophoneStateGraphBuilder.hh"
#include <Flow/DataAdaptor.hh>
#include "FsaCache.hh"
#include <Fsa/Basic.hh>

using namespace Speech;

/** Delimiter
 */
class LinearSegmenter::Delimiter: public Core::Component
{
public:
    typedef std::pair<TimeframeIndex, TimeframeIndex> Delimitation;
    static const Core::ParameterInt paramNumberOfIterations;
    static const Core::ParameterFloat paramPenalty;
    static const Core::ParameterFloat paramMinimumSpeechProportion;
protected:
    u32 numberOfIterations_;
    f64 f_; // penalty
    f32 minimumSpeechProportion_;
    std::vector<f64> Q_;
    std::vector<f64> M_;

    mutable Core::XmlChannel statisticsChannel_;
protected:
    f64 logLikelihood(u32 ib, u32 ie) const;
public:
    Delimiter(const Core::Configuration&);
    virtual ~Delimiter() {
    }

    void reset() {
	Q_.clear();
	Q_.push_back(0);
	M_.clear();
	M_.push_back(0);
    }
    void feed(f64 xx) {
	M_.push_back(M_.back() + xx);
	if (f_ > 0) {
	    Q_.push_back(Q_.back() + (xx * xx + f_ / 2.0) / f_);
	} else {
	    Q_.push_back(Q_.back() + xx * xx);
	}
	if (Q_.back() > Core::Type<f64>::max)
	    error("delimit overflow => segment to long?");
    }
    virtual Delimitation getDelimitation() const;
    u32 nFeatures() const {
	verify(Q_.size() == M_.size());
	return Q_.size() - 1;
    }
};

const Core::ParameterInt LinearSegmenter::Delimiter::paramNumberOfIterations(
	"number-of-iterations",
	"number of iterations to calculate approximate silence/speech/silence segmentation",
	3, 0);

const Core::ParameterFloat LinearSegmenter::Delimiter::paramPenalty(
	"penalty",
	"penalty for second order statistic, zero for no penalty",
	100.0, 0);

const Core::ParameterFloat LinearSegmenter::Delimiter::paramMinimumSpeechProportion(
	"minimum-speech-proportion",
	"minimum proportion of speech in segment",
	0.7, 0.0, 1.0);

LinearSegmenter::Delimiter::Delimiter(const Core::Configuration& c) :
	    Core::Component(c),
	    numberOfIterations_(paramNumberOfIterations(config)),
	    f_(paramPenalty(config)),
	    minimumSpeechProportion_(paramMinimumSpeechProportion(config)),
	    statisticsChannel_(config, "statistics")
{}

f64 LinearSegmenter::Delimiter::logLikelihood(u32 ib, u32 ie) const
{
    const f64 e = 0.5;
    f64 yy = Core::Type<f64>::max;
    f64 s1 = ib - 1 + nFeatures() - ie;
    f64 m1 = M_[ib - 1] + M_[nFeatures()] - M_[ie];
    f64 qq = Q_[ib - 1] + Q_[nFeatures()] - Q_[ie];
    f64 xx = qq / s1 - ((m1 / s1) * (m1 / s1)) / (f_ > 0 ? f_ : 1);
    f64 xsil = s1 * ::log(std::max(xx, e));
    f64 s2 = ie - ib + 1;
    f64 m2 = M_[ie] - M_[ib - 1];
    qq = Q_[ie] - Q_[ib - 1];
    xx = qq / s2 - ((m2 / s2) * (m2 / s2)) / (f_ > 0 ? f_ : 1);
    f64 xspe = s2 * ::log(std::max(xx, e));
    if (((m1 / s1) < (m2 / s2)) &&
	    ((ie - ib) >= (minimumSpeechProportion_ * nFeatures()))) yy = xsil + xspe;
    return yy;
}

/*****************************************************************************/
/*
 * taken from old standard system (aka delimit(*)):
 *
 *       START/STOP-DETECTION USING TWO GAUSSIAN MODELS,
 *       ONE FOR SPEECH AND ONE FOR SILENCE;
 *       BRIDLE,SEDGWICK, PP.656-659, ICASSP'77.
 *
 *       ITERATIVE APPROXIMATION BY VARYING IB AND IE SEPARATELY
 *
 *                                          N
 *       F =  N*log(var) =  N * log( 1/N * SUM [x(n)-xx]**2 )
 *                                         n=1
 *
 *       SUM [x(n)-xx]**2 =  SUM [x(n)**2 - 2*xx*x(n)     + xx**2]
 *                        =  SUM x(n)**2  - 2*xx*SUM x(n) + SUM xx**2
 *                        =  SUM x(n)**2  - 2*xx*N*xx     +  N *xx**2
 *                        =  SUM x(n)**2  - N*(xx**2)
 *
 *        silence       speech      silence
 *       [1,...,IB-1] [IB,...,IE] [IE+1,...,NI]
 */
/*****************************************************************************/
LinearSegmenter::Delimiter::Delimitation
LinearSegmenter::Delimiter::getDelimitation() const
{
    if (minimumSpeechProportion_ == 1.0)
	return std::make_pair(0, nFeatures() - 1);

    if (nFeatures() == 1)
	return std::make_pair(0, 0);

    f64 x;
    f64 x_min = Core::Type<f64>::max;
    u32 ie, ib, iEnd = nFeatures() - 1, iBeg = 1;
    for (u32 i = 0; i < numberOfIterations_; ++i) {
	for (ib = 2; ib < iEnd - 1; ++ib) {
	    x = logLikelihood(ib, iEnd);
	    if (x < x_min) {
		x_min = x;
		iBeg = ib;
	    }
	}
	for (ie = iBeg + 1; ie < nFeatures() - 1; ie++) {
	    x = logLikelihood(iBeg, ie);
	    if (x < x_min) {
		x_min = x;
		iEnd = ie;
	    }
	}
    }

    if (statisticsChannel_.isOpen()) {
	statisticsChannel_ << Core::XmlOpen("delimiter-statistics")
	<< Core::XmlFull("frames", nFeatures())
	<< Core::XmlFull("speech-begin", iBeg)
	<< Core::XmlFull("speech-end", iEnd)
	<< Core::XmlFull("score", x_min)
	<< Core::XmlClose("delimiter-statistics");
    }

    return std::make_pair(iBeg - 1, iEnd - 1);
}

/** Sietill Delimiter (silence  proportion may be large)
 */
class SietillDelimiter : public LinearSegmenter::Delimiter
{
    typedef LinearSegmenter::Delimiter Precursor;
public:
    SietillDelimiter(const Core::Configuration &c) : Precursor(c) {}
    virtual ~SietillDelimiter() {}

    virtual Delimitation getDelimitation() const;
};

/**
 *  from /u/loof/teaching/speech-image-WS0405/3.1.solution/Training.c
 */
LinearSegmenter::Delimiter::Delimitation
SietillDelimiter::getDelimitation() const
{
    if (minimumSpeechProportion_ == 1.0)
	return std::make_pair(0, nFeatures() - 1);

    f64 x_min = Core::Type<f64>::max;
    u32 iEnd = nFeatures(), iBeg = 1;
    for (u32 i = 0; i < numberOfIterations_; ++i) {
	x_min = Core::Type<f64>::max;
	for (u32 ib = 2; ib < nFeatures(); ++ib) {
	    if (ib != iEnd) {
		f64 x = logLikelihood(std::min(iEnd, ib), std::max(ib, iEnd));
		if (x < x_min) {
		    x_min = x;
		    iBeg = ib;
		}
	    }
	}
	for (u32 ie = 2; ie < nFeatures(); ++ ie) {
	    if (ie != iBeg) {
		f64 x = logLikelihood(std::min(iBeg, ie), std::max(ie, iBeg));
		if(x < x_min) {
		    x_min = x;
		    iEnd = ie;
		}
	    }
	}

	if (iEnd < iBeg) {
	    u32 tmp = iBeg;
	    iBeg = iEnd;
	    iEnd = tmp;
	}
    }

    if (statisticsChannel_.isOpen()) {
	statisticsChannel_ << Core::XmlOpen("delimiter-statistics")
	<< Core::XmlFull("frames", nFeatures())
	<< Core::XmlFull("speech-begin", iBeg)
	<< Core::XmlFull("speech-end", iEnd)
	<< Core::XmlFull("score", x_min)
	<< Core::XmlClose("delimiter-statistics");
    }

    return std::make_pair(iBeg - 1, iEnd - 1);
}

/** LinearSegmenter
 */
const Core::ParameterInt LinearSegmenter::paramMaximumSegmentLength(
	"maximum-segment-length",
	"maximum number of timeframes a segment may have to enter the linear segmentation",
	Core::Type<s32>::max, 0);

const Core::ParameterInt LinearSegmenter::paramMinimumSegmentLength(
	"minimum-segment-length",
	"minimum number of timeframes a segment may have to enter the linear segmentation",
	1, 0);

const Core::ParameterBool LinearSegmenter::paramShouldUseSietill(
	"should-use-sietill",
	"delimitation for Sietill is special",
	false);

LinearSegmenter::LinearSegmenter(const Core::Configuration &c) :
	    Core::Component(c),
	    delimiter_(0),
	    silence_(Fsa::InvalidLabelId),
	    minimumSegmentLength_(paramMinimumSegmentLength(config)),
	    maximumSegmentLength_(paramMaximumSegmentLength(config)),
	    maximumJump_(2)
{
    if (!paramShouldUseSietill(config)) {
	delimiter_ = new Delimiter(select("delimiter"));
    } else {
	delimiter_ = new SietillDelimiter(select("delimiter"));
    }
}

LinearSegmenter::~LinearSegmenter()
{
    delete delimiter_;
}

u32 LinearSegmenter::nFeatures() const
{
    return delimiter_->nFeatures();
}

void LinearSegmenter::setSilence(Am::AllophoneStateIndex silence)
{
    if (silence == Fsa::InvalidLabelId)
	error("Silence allophone state index is not valid.");
    silence_ = silence;
}

void LinearSegmenter::setModel(Fsa::ConstAutomatonRef flatModel)
{
    delimiter_->reset();
    states_.clear();
    if (!flatModel || flatModel->initialStateId() == Fsa::InvalidStateId) return;

    Fsa::ConstStateRef sp = flatModel->getState(flatModel->initialStateId());
    while (!sp->isFinal()) {
	verify(sp->begin() != sp->end());
	states_.push_back(sp->begin()->input());
	sp = flatModel->getState(sp->begin()->target());
    }
}

void LinearSegmenter::feed(f32 f)
{
    delimiter_->feed(f);
}

bool LinearSegmenter::isAlignmentValid() const
{
    return (states_.size() > 0) && (nFeatures() > 0);
}

bool LinearSegmenter::getAlignment(Alignment &alignment) const
{
    if (nFeatures() < minimumSegmentLength_) return false;
    if (nFeatures() > maximumSegmentLength_) return false;
    TimeframeIndex time;
    Delimiter::Delimitation delimitation = delimiter_->getDelimitation();

    if (silence_ == Fsa::InvalidLabelId) respondToDelayedErrors();
    for (time = 0; time < delimitation.first; ++time)
	alignment.push_back(AlignmentItem(time, silence_));

    f32 slope = states_.size() - 1;
    slope /= std::max((delimitation.second - delimitation.first), 1u);
    u32 previousIndex = 0;
    for ( ; time <= delimitation.second; ++time) {
	u32 index = static_cast<u32>(round(slope * (time - delimitation.first)));
	if (index - previousIndex > maximumJump_) {
	    alignment.clear();
	    return false;
	}
	alignment.push_back(AlignmentItem(time, states_[index]));
	previousIndex = index;
    }
    if(previousIndex != (states_.size() - 1))
    {
	warning("Alignment problem, index mismatch: ", previousIndex, " vs ", states_.size()-1);
	alignment.clear();
	return false;
    }

    for ( ; time < nFeatures(); ++time)
	alignment.push_back(AlignmentItem(time, silence_));

    return true;
}

/** AlignmentWithLinearSegmentationNode
 */
AlignmentWithLinearSegmentationNode::AlignmentWithLinearSegmentationNode(const Core::Configuration &c) :
	    Core::Component(c),
	    Precursor(c),
	    segmenter_(select("linear-segmenter"))
{}

bool AlignmentWithLinearSegmentationNode::configure()
{
    if (!Precursor::configure(Flow::Float32::type())) return false;
    if (needInit_) initialize();
    createModel();
    return true;
}

void AlignmentWithLinearSegmentationNode::createModel()
{
    verify(modelCache_);
    verify(allophoneStateGraphBuilder_);

    Fsa::ConstAutomatonRef model = modelCache_->find(segmentId_);
    if (!model) {
	model = modelCache_->get(allophoneStateGraphBuilder_->createFunctor(segmentId_, orthography_));
    }
    segmenter_.setModel(model);
}

void AlignmentWithLinearSegmentationNode::initialize()
{
    ModelCombination modelCombination(select("model-combination"), ModelCombination::useAcousticModel,
	    Am::AcousticModel::noEmissions);
    modelCombination.load();

    verify(!allophoneStateGraphBuilder_);
    allophoneStateGraphBuilder_ = new AllophoneStateGraphBuilder(
	    select("allophone-state-graph-builder"),
	    modelCombination.lexicon(),
	    modelCombination.acousticModel(),
	    true);

    Core::DependencySet dependencies;
    modelCombination.getDependencies(dependencies);

    verify(!modelCache_);
    modelCache_ = new FsaCache(select("model-cache"), Fsa::storeStates);
    modelCache_->setDependencies(dependencies);

    segmenter_.setSilence(modelCombination.acousticModel()->silenceAllophoneStateIndex());

    needInit_ = false;
}

bool AlignmentWithLinearSegmentationNode::work(Flow::PortId p)
{
    Flow::DataAdaptor<Alignment> *alignment = new Flow::DataAdaptor<Alignment>();
    alignment->invalidateTimestamp();

    Flow::DataPtr<Flow::Float32> in;
    while (getData(0, in)) {
	segmenter_.feed(in->data());
	alignment->expandTimestamp(*in);
    }

    if (!segmenter_.isAlignmentValid()) {
	warning("Failed to generate the alignment for segment %s (%d).",
		segmentId_.c_str(), segmenter_.nFeatures());
	return false;
    } else if (!segmenter_.getAlignment(alignment->data()))
	warning("The segment %s (%d) has been discarded.",
		segmentId_.c_str(), segmenter_.nFeatures());

    return putData(0, alignment) && putData(0, in.get());
}
