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
#include "PiecewiseLinearFunction.hh"

using namespace Math;


PiecewiseLinearFunction::PiecewiseLinearFunction() :
    a_(new PiecewiseConstantFunction),
    b_(new PiecewiseConstantFunction)
{}


void PiecewiseLinearFunction::add(Argument limit, Argument a)
{
    if (empty())
	append(limit, a, 0);
    else
	append(limit, a, value(lastLimit()) - a * lastLimit());
}

void PiecewiseLinearFunction::normalize(Argument limit)
{
    require(limit > lastLimit());
    if (empty())
	add(Core::Type<Argument>::max, 1);
    else
	add(Core::Type<Argument>::max, (limit - value(lastLimit())) / (limit - lastLimit()));
}

void PiecewiseLinearFunction::append(Argument limit, Argument a, Argument b)
{
    require(empty() || limit > lastLimit());
    a_->insert(std::make_pair(limit, a));
    b_->insert(std::make_pair(limit, b));
}

UnaryAnalyticFunctionRef PiecewiseLinearFunction::invert() const
{
    PiecewiseLinearFunction *result = new PiecewiseLinearFunction;
    PiecewiseConstantFunction::const_iterator a = a_->begin();
    PiecewiseConstantFunction::const_iterator b = b_->begin();
    for(; a != a_->end(); ++ a, ++ b)
	result->append(value(a->first), 1.0 / a->second, -b->second / a->second);
    return UnaryAnalyticFunctionRef(result);
}
