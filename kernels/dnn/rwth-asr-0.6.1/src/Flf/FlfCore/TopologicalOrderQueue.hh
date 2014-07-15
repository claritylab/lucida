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
#ifndef _FLF_CORE_TOPOLOGICAL_ORDER_QUEUE_HH
#define _FLF_CORE_TOPOLOGICAL_ORDER_QUEUE_HH

#include <Core/PriorityQueue.hh>
#include <Core/ReferenceCounting.hh>

#include "Lattice.hh"

namespace Flf {

    /**
     * Priority queue for states using the topological order for states.
     *
     **/
    struct WeakTopologicalOrder {
		ConstStateMapRef topologicalOrder;
		WeakTopologicalOrder(ConstStateMapRef topologicalOrder) :
			topologicalOrder(topologicalOrder) {}
		bool operator() (Fsa::StateId sid1, Fsa::StateId sid2) const
			{ return (*topologicalOrder)[sid1] < (*topologicalOrder)[sid2]; }
    };
    class TopologicalOrderQueue :
		public Core::PriorityQueue<Fsa::StateId, WeakTopologicalOrder>,
		public Core::ReferenceCounted {
		typedef Core::PriorityQueue<Fsa::StateId, WeakTopologicalOrder> Precursor;
    private:
		ConstStateMapRef topologicalOrder_;
    public:
		TopologicalOrderQueue(const WeakTopologicalOrder &weakOrder) :
			Precursor(weakOrder), topologicalOrder_(weakOrder.topologicalOrder) {}
		ConstStateMapRef getTopologicalOrder() const { return topologicalOrder_; }
    };
    typedef Core::Ref<TopologicalOrderQueue> TopologicalOrderQueueRef;

    TopologicalOrderQueueRef createTopologicalOrderQueue(
		ConstLatticeRef l, ConstStateMapRef topologicalOrder = ConstStateMapRef());


    /**
     * Priority queue for states using the reverse topological state order.
     *
     **/
    struct WeakReverseTopologicalOrder {
		ConstStateMapRef topologicalOrder;
		WeakReverseTopologicalOrder(ConstStateMapRef topologicalOrder) :
			topologicalOrder(topologicalOrder) {}
		bool operator() (Fsa::StateId sid1, Fsa::StateId sid2) const
			{ return (*topologicalOrder)[sid1] > (*topologicalOrder)[sid2]; }
    };
    class ReverseTopologicalOrderQueue :
		public Core::PriorityQueue<Fsa::StateId, WeakReverseTopologicalOrder>,
		public Core::ReferenceCounted {
		typedef Core::PriorityQueue<Fsa::StateId, WeakReverseTopologicalOrder> Precursor;
    private:
		ConstStateMapRef topologicalOrder_;
    public:
		ReverseTopologicalOrderQueue(const WeakReverseTopologicalOrder &weakOrder) :
			Precursor(weakOrder), topologicalOrder_(weakOrder.topologicalOrder) {}
		ConstStateMapRef getTopologicalOrder() const { return topologicalOrder_; }
    };
    typedef Core::Ref<ReverseTopologicalOrderQueue> ReverseTopologicalOrderQueueRef;

    ReverseTopologicalOrderQueueRef createReverseTopologicalOrderQueue(
		ConstLatticeRef l, ConstStateMapRef topologicalOrder = ConstStateMapRef());

} // namespace Flf

#endif // _FLF_CORE_TOPOLOGICAL_ORDER_QUEUE_HH
