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
#ifndef _BLISS_CORPUS_KEY_HH
#define _BLISS_CORPUS_KEY_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/StringExpression.hh>

namespace Bliss {

    /** CorpusKey: template for combining copus section names. E.g "$(coprus)/$(speaker).xxx"
     *  They can be used as identifier for the currently processed objects
     *  (e.g. speaker dependent estimation).
     *  CorpusVisitor resolves the copus section names in the template.
     */
    class CorpusKey :
	public Core::ReferenceCounted,
	public Core::StringExpression,
	public virtual Core::Component {
    public:

	static const Core::ParameterString paramTemplate;

	static const std::string openTag;
	static const std::string closeTag;

    public:
	CorpusKey(const Core::Configuration &configuration);

	void resolve(std::string &result);
    };

} // namespace Speech

#endif // _BLISS_CORPUS_KEY_HH
