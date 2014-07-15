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
#ifndef _TEST_LEXICON_HH
#define _TEST_LEXICON_HH

#include <Bliss/Lexicon.hh>

namespace Test {

/**
 * Quick and easy to set up lexicon for unit tests.
 *
 * See Test_Lexicon.cc for an example.
 */
class Lexicon : public Bliss::Lexicon
{
public:
    Lexicon();

    bool addPhoneme(const std::string &name, bool contextDependent = true);
    void addLemma(const std::string &orth, const std::string &pron,
		  const std::string &special = "");

protected:
    Core::Ref<Bliss::PhonemeInventory> phonemes_;
};

}  // namespace Test

#endif // _TEST_LEXICON_HH
