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
#ifndef _SPEECH_MODULE_HH
#define _SPEECH_MODULE_HH

#include <Modules.hh>
#include <Speech/Recognizer.hh>
#include <Core/Singleton.hh>

namespace Search {
    class SearchAlgorithm;
}

namespace Speech {

    class AligningFeatureExtractor;
    class MixtureSetTrainer;
    class AlignedFeatureProcessor;
    class AligningFeatureExtractor;
    class DataSource;

#ifdef MODULE_SPEECH_DT
    class DiscriminativeMixtureSetTrainer;
    class SegmentwiseGmmTrainer;
    class LatticeRescorer;
#endif

    class Module_ {
    public:
	Module_();

	/**
	 * Creates and initializes a AligningFeatureExtractor as configured.
	 * @return a newly created instance of AligningFeatureExtractor or 0 if
	 * an error occured.
	 */
	AligningFeatureExtractor *createAligningFeatureExtractor(
	    const Core::Configuration&, AlignedFeatureProcessor&) const;

	MixtureSetTrainer* createMixtureSetTrainer(const Core::Configuration &configuration) const;

	/**
	 * create a Sparse::DataSource object if possible, otherwise create a
	 * Speech::DataSource object.
	 */
	DataSource* createDataSource(const Core::Configuration &configuration, bool loadFromFile=true) const;

#ifdef MODULE_SPEECH_DT
	DiscriminativeMixtureSetTrainer* createDiscriminativeMixtureSetTrainer(const Core::Configuration &configuration) const;
	SegmentwiseGmmTrainer* createSegmentwiseGmmTrainer(const Core::Configuration &configuration) const;
	LatticeRescorer* createDistanceLatticeRescorer(const Core::Configuration &configuration,
						       Bliss::LexiconRef lexicon) const;
#endif
    };

    typedef Core::SingletonHolder<Module_> Module;
}

#endif // _SPEECH_MODULE_HH
