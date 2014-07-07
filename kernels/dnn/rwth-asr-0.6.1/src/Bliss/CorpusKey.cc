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
#include "CorpusKey.hh"

using namespace Bliss;


const Core::ParameterString CorpusKey::paramTemplate(
    "template", "template expression e.g. \"<corpus-0>_<speaker>\"");

const std::string CorpusKey::openTag = "<";
const std::string CorpusKey::closeTag = ">";

CorpusKey::CorpusKey(const Core::Configuration &configuration) :
    Component(configuration) {

    if (paramTemplate(configuration).empty())
	criticalError("Template expression is empty or not given.");

    Core::StringExpressionParser parser(*this, openTag, closeTag);
    parser.accept(paramTemplate(configuration));
}


void CorpusKey::resolve(std::string &result) {
    if (!value(result))
	criticalError("Could not resolve corpus key %s.", name().c_str());
}
