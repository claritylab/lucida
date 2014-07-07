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
#ifndef _SEARCH_MODULE_HH
#define _SEARCH_MODULE_HH

#include <Core/Configuration.hh>
#include <Core/Singleton.hh>

namespace Search {

    class SearchAlgorithm;
    class LatticeHandler;

    enum SearchType {
	WordConditionedTreeSearchType,
	AdvancedTreeSearch,
	TeachingWordConditionedTreeSearchType,
	FsaSearchType,
	LinearSearchType,
	ExpandingFsaSearchType,
	TeachingLinearSearchType,
	LinearSignLanguageSearchType
    };

    class Module_ {
    public:
	Module_();

	SearchAlgorithm* createRecognizer(SearchType type, const Core::Configuration &config) const;
	LatticeHandler* createLatticeHandler(const Core::Configuration &c) const;
    };

    typedef Core::SingletonHolder<Module_> Module;
}

#endif // _SEARCH_MODULE_HH
