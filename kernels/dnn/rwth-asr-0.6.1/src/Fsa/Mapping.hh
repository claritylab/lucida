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
#ifndef _FSA_MAPPING_HH
#define _FSA_MAPPING_HH

#include <Core/ReferenceCounting.hh>
#include "Types.hh"

namespace Fsa {
    /*
      Mappings must not have references to automatons;
      only weak references.
    */

    /**
     * Mapping between state ids of two automatons;
     * see compose for examples
     **/
    class Mapping : public Core::ReferenceCounted {
    public:
	virtual ~Mapping() {}
	virtual StateId map(StateId target) const = 0;
    };
    typedef Core::Ref<const Mapping> ConstMappingRef;

} // namespace Fsa

#endif // _FSA_MAPPING_HH
