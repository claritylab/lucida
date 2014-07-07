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
#ifndef _SIGNAL_BAYES_CLASSIFICATION_HH
#define _SIGNAL_BAYES_CLASSIFICATION_HH

#include "AprioriProbability.hh"
#include "LikelihoodFunction.hh"
#include "SlidingWindow.hh"
#include <Core/Statistics.hh>
#include <Flow/DataAdaptor.hh>
#include <Flow/Vector.hh>

namespace Signal {


    /** Implementation of bayes decision rule for "simple" distributions.
     *
     *  Provides the class k with maximum a-posterioiry probability
     *    i.e minimum with -log(a-posterioiry probability):
     *
     *    argmin_{k} -log( p(k) * p(x|k) )
     *    where p(k) is the apriori probablity of the classes and
     *    p(x|k) is the class dependent likelihood function.
     */
    class BayesClassification : public virtual Core::Component {
	typedef Component Predecessor;
    public:
	typedef LikelihoodFunction::Data Data;
	typedef LikelihoodFunction::Score Score;
	typedef LikelihoodFunction::ScoreVector ScoreVector;
	typedef LikelihoodFunction::Weight Weight;
	typedef Flow::Time Time;
	typedef std::pair<Time, Time> StartEndTime;

	enum AprioriProbabilityType { Uniform };
	enum LikelihoodFunctionType { IndependentSequence };
    private:
	std::vector<std::string> classLabels_;

	AprioriProbability *aprioriProbability_;
	LikelihoodFunction *likelihoodFunction_;

	u32 nFeatures_, nFeaturesBuffered_;
	u32 delay_;
	u32 nUsedFeatures_;
	bool useSlidingWindow_;

	/** Scores for each feature vector in the sliding window
	 */
	SlidingWindow<ScoreVector> scoreWindow_;
	/** Start and end time of the features in the sliding window
	 */
	SlidingWindow<StartEndTime> timeWindow_;

	/** Start-time of the first feature vector since the last successful classification.
	 */
	Time firstStartTime_;
	/** End-time of the last feature vector.
	 */
	Time lastEndTime_;

	/** function classify returns false if there came no new data since the last call to it.
	 */
	bool newData_;
	bool needInit_;

	mutable Core::XmlChannel statisticsChannel_;
    private:
	void init(size_t inputSize);

	/** Bayes decision rule.
	 *  @return is class label with minimal score.
	 */
	const std::string& argMin() const;
	void writeStatistics(Core::XmlWriter&, u32 minClassIndex, const Core::Statistics<Score>&) const;

	/** Updated firstStartTime_ and lastEndTime_ by using the member newData_.
	 */
	void updateTimes(const Flow::Timestamp &timestamp);
    public:
	BayesClassification(const Core::Configuration &c);
	~BayesClassification();

	/** Accumulates the scores for @param featureVector. */
	void feed(const Flow::Vector<Data> &featureVector, Weight featureScoreWeight = 1);
	/** Carries out the classification based on accumulated scores.
	 *  Result of classification is stored in @param classLabel.
	 *
	 *  @return is false if no features has been seen or
	 *    if there came no new data since the last call.
	 */
	bool classify(Flow::String& classLabel);
	/** After updating scores by @param featureVector, classification is carried out.
	 *  Result of classification is stored in @param classLabel.
	 *
	 *  @return is false if decision could not be made yet (@see setDelay) or
	 *    if there came no new data since the last call.
	 */
	bool classify(Flow::String& classLabel,
		      const Flow::Vector<Data> &featureVector, Weight featureScoreWeight = 1);

	bool getScores(Flow::Vector<Score> &scores);
	bool getScores(Flow::Vector<Score> &scores,
		       const Flow::Vector<Data> &featureVector, Weight featureScoreWeight);
	/** Clears accumulated results.
	 */
	void reset();

	/** Have been sufficient features vectors processed?
	 */
	bool needMoreFeatureVectors() const;

	void setAprioriProbability(AprioriProbabilityType type);
	void setLikelihoodFunction(LikelihoodFunctionType type);

	void setClassLabels(const std::string &fileName);
	void setClassLabels(u32 nClasses);
	void setNumUsedFeatures(u32 nFeatures);
	void setUseSlidingWindow(bool useWindow, int windowLength, int windowRight);
	u32 nClasses() const { return classLabels_.size(); }

	/** Decision is made only after @param delay number of features.
	 */
	void setDelay(u32 delay);
    };


    /** BayesClassificationNode
     */
    class BayesClassificationNode : public Flow::SleeveNode, public BayesClassification {
    public:
	static const Core::ParameterString paramClassLabelsFileName;

	static const Core::Choice choiceAprioriProbabilityType;
	static const Core::ParameterChoice paramAprioriProbabilityType;

	static const Core::Choice choiceLikelihoodFunctionType;
	static const Core::ParameterChoice paramLikelihoodFunctionType;

	static const Core::ParameterInt paramDelay;
	static const Core::ParameterInt paramNumUsedFeatures;
	static const Core::ParameterInt paramWindowLength;
	static const Core::ParameterInt paramWindowRight;
    protected:
	Weight featureScoreWeight(const Flow::Timestamp &featureTimestamp);
    public:
	static std::string filterName() { return "signal-bayes-classification"; }

	BayesClassificationNode(const Core::Configuration &c);
	virtual ~BayesClassificationNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual Flow::PortId getInput(const std::string &name);
	virtual bool work(Flow::PortId p);
    protected:
	virtual std::string outputTypeName() { return Flow::String::type()->name(); }
    };

    class BayesClassificationScoreNode : public BayesClassificationNode {
	typedef BayesClassificationNode Precursor;
	typedef BayesClassification::Score Score;
    public:
	static const Core::ParameterInt paramNumberOfClasses;
	static const Core::ParameterBool paramSingleFrameClassification;
    public:
	static std::string filterName() { return "signal-bayes-classification-score"; }
	BayesClassificationScoreNode(const Core::Configuration &c);
	virtual ~BayesClassificationScoreNode() {}
	virtual bool work(Flow::PortId p);
    protected:
	virtual std::string outputTypeName() { return Flow::Vector<Score>::type()->name(); }
	bool singleFrames_;
    };


} // namespace Signal

#endif // _SIGNAL_BAYES_CLASSIFICATION_HH
