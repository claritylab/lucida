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
#include <Modules.hh>
#include <Search/LatticeHandler.hh>
#include <Search/Module.hh>
#include <Search/WordConditionedTreeSearch.hh>

using namespace Search;

Module_::Module_()
{
}


SearchAlgorithm* Module_::createRecognizer(SearchType type, const Core::Configuration &config) const
{
    SearchAlgorithm *recognizer = 0;
    switch (type) {
    case WordConditionedTreeSearchType:
	recognizer = new Search::WordConditionedTreeSearch(config);
	break;

    case AdvancedTreeSearch:
#if 1
	Core::Application::us()->criticalError("Module MODULE_ADVANCED_TREE_SEARCH not available!");
#endif
	break;
    case FsaSearchType:
#if 1
	Core::Application::us()->criticalError("Module MODULE_SEARCH_WFST_DEPRECATED not available!");
#endif
	break;
    case ExpandingFsaSearchType:
#if 1
	Core::Application::us()->criticalError("Module MODULE_SEARCH_WFST not available!");
#endif
	break;
    case LinearSearchType:
#if 1
	Core::Application::us()->criticalError("Module MODULE_SEARCH_LINEAR not available!");
#endif
	break;
    case TeachingWordConditionedTreeSearchType:
#if 1
	Core::Application::us()->criticalError("Module MODULE_TEACHING not available!");
#endif
	break;
    case TeachingLinearSearchType:
#if 1
	Core::Application::us()->criticalError("Module MODULE_TEACHING not available!");
#endif
    case LinearSignLanguageSearchType:
#if 1
	Core::Application::us()->criticalError("Module MODULE_SEARCH_SIGNLANGUAGE not available!");
	break;
#endif
    default:
	Core::Application::us()->criticalError("unknown recognizer type: %d", type);
	break;
    }
    return recognizer;
}

LatticeHandler* Module_::createLatticeHandler(const Core::Configuration &c) const
{
    LatticeHandler *handler = new LatticeHandler(c);
    /**
     * @todo: add Flf::LatticeHandler?
     * This would add a dependency to module Flf without actual benefit.
     */
    return handler;
}
