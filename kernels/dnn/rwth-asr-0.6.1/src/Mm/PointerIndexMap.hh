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
#ifndef _MM_POINTER_INDEX_MAP_HH
#define _MM_POINTER_INDEX_MAP_HH

#include <Core/Hash.hh>
#include <Core/ReferenceCounting.hh>
#include "Utilities.hh"

namespace Mm {

    template<class Pointer, class Hash, class Equality>
    class PointerIndexMap {
    public:
	typedef Core::hash_map<Pointer, size_t, Hash, Equality> PointerToIndexMap;
	typedef std::vector<Pointer> IndexToPointerMap;
    private:
	PointerToIndexMap pointerToIndex_;
	IndexToPointerMap indexToPointer_;
    public:
	void add(Pointer pointer) {
	    if (pointerToIndex_.find(pointer) == pointerToIndex_.end()) {
		pointerToIndex_.insert(std::make_pair(pointer, indexToPointer_.size()));
		indexToPointer_.push_back(pointer);
	    }
	}

	size_t size() const { return indexToPointer_.size(); }

	const Pointer operator[](size_t index) const { return indexToPointer_[index]; }
	const std::vector<Pointer>& indexToPointerMap() const { return indexToPointer_; }

	size_t operator[](Pointer pointer) const {
	    typename PointerToIndexMap::const_iterator result = pointerToIndex_.find(pointer);
	    ensure(result != pointerToIndex_.end());
	    return result->second;
	}
    };

    template<class Referenced>
    class ReferenceIndexMap :
	public PointerIndexMap<Core::Ref<Referenced>,
			       hashReference<Referenced>,
			       std::equal_to<Core::Ref<Referenced> > >
    {};

} // namespace Mm

#endif // _MM_POINTER_INDEX_MAP_HH
