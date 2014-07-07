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
#include <Core/VectorParser.hh>
#include "BayesClassification.hh"

using namespace Signal;


// BayesClassification
//////////////////////


BayesClassification::BayesClassification(const Core::Configuration &c) :
    Predecessor(c),
    aprioriProbability_(0),
    likelihoodFunction_(0),
    nFeatures_(0),
    delay_(0),
    nUsedFeatures_(0),
    useSlidingWindow_(false),
    firstStartTime_(0),
    lastEndTime_(0),
    newData_(false),
    needInit_(true),
    statisticsChannel_(c, "statistics")
{}

BayesClassification::~BayesClassification() {
    delete aprioriProbability_;
    delete likelihoodFunction_;
}

void BayesClassification::init(size_t inputSize)
{
   verify(aprioriProbability_ != 0 && likelihoodFunction_ != 0);

   if (classLabels_.empty())
       criticalError("Class labels not defined.");

   if (!aprioriProbability_->setClasses(classLabels_))
       aprioriProbability_->respondToDelayedErrors();

   if (!likelihoodFunction_->setClasses(classLabels_) || !likelihoodFunction_->setDimension(inputSize))
       likelihoodFunction_->respondToDelayedErrors();

   nFeatures_ = 0;
   newData_ = false;
   needInit_ = false;
}

void BayesClassification::feed(const Flow::Vector<Data> &featureVector, Weight featureScoreWeight)
{
    if (needInit_)
	init(featureVector.size());
    ScoreVector newScores;
    likelihoodFunction_->feed(featureVector, featureScoreWeight, &newScores);
    if (useSlidingWindow_)
	scoreWindow_.add(newScores);
    updateTimes(featureVector);
    ++ nFeatures_;
    ++ nFeaturesBuffered_;
    newData_ = true;
}

void BayesClassification::updateTimes(const Flow::Timestamp &timestamp)
{
    if (nFeatures_ == 0)
	firstStartTime_ = timestamp.startTime();
    lastEndTime_ = timestamp.endTime();
    if (useSlidingWindow_)
	timeWindow_.add(std::make_pair(timestamp.startTime(), timestamp.endTime()));
}

bool BayesClassification::classify(Flow::String& classLabel)
{
    if (needInit_)
	init(0);

    if (!newData_)
	return false;

    classLabel() = argMin();

    if (useSlidingWindow_) {
	classLabel.setStartTime(timeWindow_.back().first);
	classLabel.setEndTime(timeWindow_.front().second);
    } else {
	classLabel.setStartTime(firstStartTime_);
	classLabel.setEndTime(lastEndTime_);
    }

    nFeaturesBuffered_ = 0;
    newData_ = false;
    return true;
}

bool BayesClassification::classify(
    Flow::String &classLabel, const Flow::Vector<Data> &featureVector, Weight featureScoreWeight)
{
    feed(featureVector, featureScoreWeight);
    bool output = false;
    if (!needMoreFeatureVectors())
	output = true;
    if (!useSlidingWindow_ && nFeatures_ > delay_)
	output = true;
    if (useSlidingWindow_ && scoreWindow_.full()) {
	if (delay_ == u32(Core::Type<s32>::max) || nFeaturesBuffered_ >= delay_)
	    output = true;
    }

    if (output) {
	return classify(classLabel);
    }
    return false;
}


bool BayesClassification::getScores(Flow::Vector<Score> &scores)
{
    if(needInit_)
	init(0);
    if(!newData_)
	return false;
    scores.resize(nClasses());
    scores.setStartTime(firstStartTime_);
    scores.setEndTime(lastEndTime_);
    newData_ = false;
    for(u32 classIndex = 0; classIndex < nClasses(); ++classIndex)
	scores[classIndex] = (*aprioriProbability_)[classIndex] + (*likelihoodFunction_)[classIndex];
    return true;
}

const std::string& BayesClassification::argMin() const
{
    verify(nClasses() > 0);

    Score minScore = Core::Type<Score>::max;
    u32 minClassIndex = Core::Type<u32>::max;
    Core::Statistics<Score> statistics("score-statistics");

    for(u32 classIndex = 0; classIndex < nClasses(); ++ classIndex) {
	Score score = (*aprioriProbability_)[classIndex];
	if (useSlidingWindow_) {
	    for (SlidingWindow<ScoreVector>::ConstantIterator i = scoreWindow_.begin();
		 i != scoreWindow_.end(); ++i)
		score += (*i)[classIndex];
	} else {
	    score += (*likelihoodFunction_)[classIndex];
	}
	if (statisticsChannel_.isOpen())
	    statistics += score;
	if (score < minScore) {
	    minScore = score;
	    minClassIndex = classIndex;
	}
    }
    if (statisticsChannel_.isOpen())
	writeStatistics(statisticsChannel_, minClassIndex, statistics);
    return classLabels_[minClassIndex];
}

void BayesClassification::writeStatistics(
    Core::XmlWriter &os, u32 minClassIndex, const Core::Statistics<Score> &scoreStatistics) const
{
    os << Core::XmlOpen(name() + "-statistics")
       << Core::XmlOpen("minimum")
       << Core::XmlOpen("class") << classLabels_[minClassIndex]
       << Core::XmlClose("class")
       << Core::XmlOpen("apriory-score") << (*aprioriProbability_)[minClassIndex]
       << Core::XmlClose("apriory-score")
       << Core::XmlOpen("likelihood-score") << (*likelihoodFunction_)[minClassIndex]
       << Core::XmlClose("likelihood-score")
       << Core::XmlOpen("average-likelihood-score")
       << ((*likelihoodFunction_)[minClassIndex] / likelihoodFunction_->sumOfWeights())
       << Core::XmlClose("average-likelihood-score")
       << Core::XmlClose("minimum")
       << scoreStatistics
       << Core::XmlClose(name() + "-statistics");
}

bool BayesClassification::getScores(
    Flow::Vector<Score> &scores, const Flow::Vector<Data> &featureVector, Weight featureScoreWeight)
{
    feed(featureVector, featureScoreWeight);
    if(nFeatures_ > delay_)
	return getScores(scores);
    return false;
}

void BayesClassification::reset()
{
    if (likelihoodFunction_ != 0)
	likelihoodFunction_->reset();
    if (useSlidingWindow_) {
	scoreWindow_.clear();
	timeWindow_.clear();
    }

    nFeatures_ = 0;
    nFeaturesBuffered_ = 0;
    newData_ = false;
}

void BayesClassification::setClassLabels(const std::string &fileName)
{
    if (fileName != "") {
	Core::XmlVectorDocument<std::string> parser(getConfiguration(), classLabels_);
	parser.parseFile(fileName.c_str());

	if (classLabels_.empty())
	    criticalError("The file \"%s\" did not contain any class labels.", fileName.c_str());

	needInit_ = true;
    }
}

void BayesClassification::setClassLabels(u32 nClasses)
{
    classLabels_.resize(nClasses);
    needInit_ = true;
}

void BayesClassification::setAprioriProbability(AprioriProbabilityType type)
{
    if (aprioriProbability_)
	delete aprioriProbability_;

    switch(type) {
      case Uniform:
	  aprioriProbability_ = new UniformAprioriProbability(select("a-priory-probability"));
	  break;
    default:
	defect();
    }
    needInit_ = true;
}

void BayesClassification::setLikelihoodFunction(LikelihoodFunctionType type)
{
    if (likelihoodFunction_)
	delete likelihoodFunction_;

    switch(type) {
      case IndependentSequence:
	  likelihoodFunction_ = new IndependentSequenceLikelihood(select("likelihood-function"));
	  break;
    default:
	defect();
    }
    needInit_ = true;
}

void BayesClassification::setDelay(u32 delay)
{
    if (delay_ != delay) {
	delay_ = delay;
	reset();
    }
}

void BayesClassification::setNumUsedFeatures(u32 nFeatures)
{
    if (nUsedFeatures_ != nFeatures) {
	nUsedFeatures_ = nFeatures;
	reset();
    }
}

bool BayesClassification::needMoreFeatureVectors() const
{
    return (nFeatures_ < nUsedFeatures_);
}

void BayesClassification::setUseSlidingWindow(bool useWindow, int windowLength, int windowRight)
{
    // note: assignment in if clause
    if ((useSlidingWindow_ = useWindow)) {
	scoreWindow_.init(windowLength, windowRight);
	timeWindow_.init(windowLength, windowRight);
    }
}



// BayesClassificationNode
//////////////////////////

const Core::ParameterString BayesClassificationNode::paramClassLabelsFileName(
    "class-label-file", "file containing list of class labels");

const Core::Choice BayesClassificationNode::choiceAprioriProbabilityType(
    "uniform", Uniform,
    Core::Choice::endMark());
const Core::ParameterChoice BayesClassificationNode::paramAprioriProbabilityType(
    "a-priory-probability-type", &choiceAprioriProbabilityType, "type of apriori probability", Uniform);

const Core::Choice BayesClassificationNode::choiceLikelihoodFunctionType(
    "independent-sequence", IndependentSequence,
    Core::Choice::endMark());
const Core::ParameterChoice BayesClassificationNode::paramLikelihoodFunctionType(
    "likelihood-function-type", &choiceLikelihoodFunctionType,
    "type of likelihood function", IndependentSequence);

const Core::ParameterInt BayesClassificationNode::paramDelay(
    "delay", "continuous classification starts after delay number of features", Core::Type<s32>::max, 0);

const Core::ParameterInt BayesClassificationNode::paramNumUsedFeatures(
    "number-of-features", "number of feature vectors used for classification", Core::Type<s32>::max, 1);

const Core::ParameterInt BayesClassificationNode::paramWindowLength(
    "window-length", "width of the sliding window, -1 to disable usage of sliding window", -1);
const Core::ParameterInt BayesClassificationNode::paramWindowRight(
    "window-right", "output point of the sliding window", 0);


BayesClassificationNode::BayesClassificationNode(const Core::Configuration &c) :
    Component(c),
    SleeveNode(c),
    BayesClassification(c)
{
    if(!paramClassLabelsFileName(c).empty())
	setClassLabels(paramClassLabelsFileName(c));
    setAprioriProbability((AprioriProbabilityType)paramAprioriProbabilityType(c));
    setLikelihoodFunction((LikelihoodFunctionType)paramLikelihoodFunctionType(c));
    setDelay(paramDelay(c));
    setNumUsedFeatures(paramNumUsedFeatures(c));
    setUseSlidingWindow(paramWindowLength(c) > 0, paramWindowLength(c), paramWindowRight(c));
}

bool BayesClassificationNode::configure()
{
    reset();

    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);
    if (!configureDatatype(attributes, Flow::Vector<f32>::type()))
	return false;

    if (nInputs() >= 2) {
	Core::Ref<const Flow::Attributes> weightAttributes = getInputAttributes(1);
	if (!configureDatatype(weightAttributes, Flow::Float32::type()))
	    return false;
	attributes->merge(*weightAttributes);
    }
    attributes->set("datatype", outputTypeName());
    attributes->remove("sample-rate");

    return putOutputAttributes(0, attributes);
}

Flow::PortId BayesClassificationNode::getInput(const std::string &name)
{
    if (name == "feature-score-weight") {
	addInput(1);
	return 1;
    }

    return 0;
}

bool BayesClassificationNode::setParameter(const std::string &name, const std::string &value)
{
     if (paramClassLabelsFileName.match(name))
	 setClassLabels(paramClassLabelsFileName(value));
    else if (paramAprioriProbabilityType.match(name))
	setAprioriProbability((AprioriProbabilityType)paramAprioriProbabilityType(value));
    else if (paramLikelihoodFunctionType.match(name))
	setLikelihoodFunction((LikelihoodFunctionType)paramLikelihoodFunctionType(value));
    else if (paramDelay.match(name))
	setDelay(paramDelay(value));
    else if (paramNumUsedFeatures.match(name))
	setNumUsedFeatures(paramNumUsedFeatures(value));
    else
	return false;

    return true;
}

BayesClassification::Weight BayesClassificationNode::featureScoreWeight(
    const Flow::Timestamp &featureTimestamp)
{
    if (nInputs() >= 2) {
	Flow::DataPtr<Flow::DataAdaptor<Weight> > featureScoreWeight;
	if (getData(1, featureScoreWeight)) {
	    if (featureTimestamp.contains(*featureScoreWeight)) {
		if ((*featureScoreWeight)() >= 0)
		    return (*featureScoreWeight)();
		else
		    criticalError("Weight (%f) smaller then zero.", (*featureScoreWeight)());
	    }

	    criticalError("Timestamp of feature (%f-%f) does not contain the one of score-weight (%f-%f)",
			  featureTimestamp.startTime(), featureTimestamp.endTime(),
			  featureScoreWeight->startTime(), featureScoreWeight->endTime());
	}
	criticalError("Score-weight stream is shorter than the feature weight");
    }
    return 1;
}

bool BayesClassificationNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::Vector<f32> > featureVector;
    Flow::DataPtr<Flow::String> classLabel(new Flow::String);
    do {
	if (!getData(0, featureVector)) {
	    // end of stream reached
	    if (classify(*classLabel))
		// send the recognized class labe
		putData(0, classLabel.get());
	    // pass the EOS
	    return putData(0, featureVector.get());
	}
    } while(!classify(*classLabel, *featureVector, featureScoreWeight(*featureVector)));

    if (!needMoreFeatureVectors()) {
	// features for classification have allready been read
	// read from stream until end to get the correct end time
	Time endTime = classLabel->endTime();
	while (getData(0, featureVector))
	    endTime = featureVector->endTime();
	classLabel->setEndTime(endTime);
	return putData(0, classLabel.get()) && putData(0, featureVector.get());
    } else {
	// continuous classification, delay <= nFeatures_
	return putData(0, classLabel.get());
    }
}


// ==========================================================

const Core::ParameterInt BayesClassificationScoreNode::paramNumberOfClasses(
    "number-of-classes", "number of classes", 0);

const Core::ParameterBool BayesClassificationScoreNode::paramSingleFrameClassification(
    "single-frame-classification", "classify single frames", false);

BayesClassificationScoreNode::BayesClassificationScoreNode(const Core::Configuration &c)
    : Component(c), Precursor(c),
      singleFrames_(paramSingleFrameClassification(c))
{
    if(nClasses() == 0)
	setClassLabels(paramNumberOfClasses(c));
}

bool BayesClassificationScoreNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::Vector<f32> > featureVector;
    Flow::DataPtr<Flow::Vector<Score> > scores(new Flow::Vector<Score>);
    bool classified = false;
    do {
	if (!getData(0, featureVector)) {
	    if (getScores(*scores))
		putData(0, scores.get());
	    return putData(0, featureVector.get());
	}
	classified = getScores(*scores, *featureVector, featureScoreWeight(*featureVector));
	if(classified && singleFrames_)
	    reset();
    } while(!classified);
    return putData(0, scores.get());
}
