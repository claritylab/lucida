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
#include "tAutomaton.hh"
#include <cstdlib>

namespace Ftl
{
template<class _Arc, template<class T> class _ArcContainer>
_Arc* State<_Arc, _ArcContainer>::newArc()
{
	return arcs_.addArc();
}

template<class _Arc, template<class T> class _ArcContainer>
_Arc* State<_Arc, _ArcContainer>::newArc(Fsa::StateId target,
		typename _Arc::Weight weight, Fsa::LabelId input)
{
	return arcs_.addArc(target, weight, input);
}

template<class _Arc, template<class T> class _ArcContainer>
_Arc* State<_Arc, _ArcContainer>::newArc(Fsa::StateId target,
		typename _Arc::Weight weight, Fsa::LabelId input, Fsa::LabelId output)
{
	return arcs_.addArc(target, weight, input, output);
}

// =========================================================0

template<class _Arc>
inline void ArrayArcContainer<_Arc>::destroy(iterator first, iterator last)
{
	typedef typename _Arc::Weight Weight;
	for(; first != last; ++first)
		first->weight_.Weight::~Weight();
}

template<class _Arc>
inline void ArrayArcContainer<_Arc>::reallocate(size_t i)
{
	if(i < nArcs_) destroy(arcs_ + i, end());
	nArcs_ = i;
	arcs_ = (_Arc*) ::realloc(arcs_, sizeof(_Arc) * nArcs_);
	/*! @todo: check bad alloc */
}

template<class _Arc>
inline void ArrayArcContainer<_Arc>::reallocateOneMore()
{
	++nArcs_;
	arcs_ = (_Arc*) ::realloc(arcs_, sizeof(_Arc) * nArcs_);
	/*! @todo: check bad alloc */
}

template<class _Arc>
ArrayArcContainer<_Arc>::ArrayArcContainer(const ArrayArcContainer<_Arc> &other)
{
	reallocate(other.nArcs_);
	::memcpy(arcs_, other.arcs_, sizeof(_Arc) * nArcs_);
}

template<class _Arc>
ArrayArcContainer<_Arc>& ArrayArcContainer<_Arc>::operator=(const Self &other)
{
	if(arcs_) clear();
	reallocate(other.nArcs_);
	::memcpy(arcs_, other.arcs_, sizeof(_Arc) * nArcs_);
	return *this;
}

template<class _Arc>
void ArrayArcContainer<_Arc>::clear()
{
	typedef typename _Arc::Weight Weight;
	if(arcs_) {
		destroy(begin(), end());
		::free(arcs_);
		arcs_ = 0;
	}
	nArcs_ = 0;
}

template<class _Arc>
_Arc* ArrayArcContainer<_Arc>::addArc()
{
	reallocateOneMore();
	return arcs_ + nArcs_ - 1;
}

template<class _Arc>
_Arc* ArrayArcContainer<_Arc>::addArc(Fsa::StateId target, typename _Arc::Weight weight,
					       Fsa::LabelId input)
{
	reallocateOneMore();
	_Arc *arc = arcs_ + nArcs_ - 1;
	arc->target_ = target;
	arc->weight_ = weight;
	arc->input_ = arc->output_ = input;
	return arc;
}

template<class _Arc>
_Arc* ArrayArcContainer<_Arc>::addArc(Fsa::StateId target, typename _Arc::Weight weight,
						   Fsa::LabelId input, Fsa::LabelId output)
{
	reallocateOneMore();
	_Arc *arc = arcs_ + nArcs_ - 1;
	arc->target_ = target;
	arc->weight_ = weight;
	arc->input_ = input;
	arc->output_ = output;
	return arc;
}

template<class _Arc>
void ArrayArcContainer<_Arc>::truncate(iterator i)
{
	reallocate(i - begin());
}

template<class _Arc>
void ArrayArcContainer<_Arc>::resize(size_t s)
{
	reallocate(s);
}

template<class _Arc>
void ArrayArcContainer<_Arc>::minimize()
{
	reallocate(nArcs_);
}

template<class _Arc>
typename ArrayArcContainer<_Arc>::iterator ArrayArcContainer<_Arc>::insert(iterator pos, const _Arc &arc)
{
	size_t p = pos - begin();
	reallocateOneMore();
	::memmove(arcs_ + p + 1, arcs_ + p, sizeof(_Arc) * (nArcs_ - p - 1));
	*(arcs_ + p) = arc;
	return arcs_ + p;
}


} // namespace Ftl
