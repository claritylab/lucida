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
#ifndef _MM_MIXTURE_SET_LOADER_HH
#define _MM_MIXTURE_SET_LOADER_HH

#include <Mm/MixtureSet.hh>

namespace Mm {

struct AbstractMixtureSetLoader
{
    virtual Core::Ref<AbstractMixtureSet> load(const std::string &filename,
					       const Core::Configuration &c) const;
    virtual ~AbstractMixtureSetLoader() {}
};

struct EmptyMixtureSetLoader : public AbstractMixtureSetLoader
{
    virtual Core::Ref<AbstractMixtureSet> load(const std::string &filename,
					       const Core::Configuration &c) const;
    virtual ~EmptyMixtureSetLoader() {}
};

} // namespace Mm

#endif  // _MM_MIXTURE_SET_LOADER_HH
