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
#ifndef _SPEECH_MODEL_COMBINATION_HH
#define _SPEECH_MODEL_COMBINATION_HH

#include <Mc/Component.hh>
#include <Bliss/Lexicon.hh>
#include <Lm/ScaledLanguageModel.hh>
#include <Am/AcousticModel.hh>
#include <Am/AcousticModelAdaptor.hh>
#include <Core/ReferenceCounting.hh>

namespace Speech {


    /** Combination of a lexicon, an acoustic model, and a language model.
     *  It supports creation and initialization of these three mutually dependent objects.
     *
     *  Usage:
     *    1) create ModelCombination object locally to create the three parts:
     *       lexicon, acoustic model, and language model.Store references of those parts which you
     *    2) call function load, to load the scaling values
     *    3) Store the references to those parts you will use later.
     *    4) When the local ModelCombination object get destructed, the unreferenced parts gets freed as well.
     */
    class ModelCombination : public Mc::Component, public Core::ReferenceCounted {
    public:
	typedef u32 Mode;
	static const Mode complete;
	static const Mode useLexicon;
	static const Mode useAcousticModel;
	static const Mode useLanguageModel;

	static const Core::ParameterFloat paramPronunciationScale;
    protected:
	Bliss::LexiconRef lexicon_;
	Mm::Score pronunciationScale_;
	Core::Ref<Am::AcousticModel> acousticModel_;
	Core::Ref<Lm::ScaledLanguageModel> languageModel_;
    private:
	void setPronunciationScale(Mm::Score scale) { pronunciationScale_ = scale; }
    protected:
	virtual void distributeScaleUpdate(const Mc::ScaleUpdate &scaleUpdate);
    public:
	ModelCombination(const Core::Configuration &,
			 Mode = complete,
			 Am::AcousticModel::Mode = Am::AcousticModel::complete);
	ModelCombination(const Core::Configuration &,
			 Bliss::LexiconRef, Core::Ref<Am::AcousticModel>, Core::Ref<Lm::ScaledLanguageModel>);
	virtual ~ModelCombination();

	void getDependencies(Core::DependencySet &) const;

	Bliss::LexiconRef lexicon() const { return lexicon_; }
	void setLexicon(Bliss::LexiconRef);
	Mm::Score pronunciationScale() const { return pronunciationScale_ * scale(); }
	Core::Ref<Am::AcousticModel> acousticModel() const { return acousticModel_; }
	void setAcousticModel(Core::Ref<Am::AcousticModel>);
	Core::Ref<Lm::ScaledLanguageModel> languageModel() const { return languageModel_; }
	void setLanguageModel(Core::Ref<Lm::ScaledLanguageModel>);
    };

    typedef Core::Ref<ModelCombination> ModelCombinationRef;

} // namespace Speech

namespace Core {

    template <>
    class NameHelper<Speech::ModelCombinationRef> : public std::string {
    public:
	NameHelper() : std::string("flow-model-combination-ref") {}
    };

} // namespace Core

#endif // _SPEECH_MODEL_COMBINATION_HH
