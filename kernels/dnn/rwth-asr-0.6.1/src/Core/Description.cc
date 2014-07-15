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
#include "Description.hh"

using namespace Core;

//=============================================================================================
void Description::Stream::getDependencies(
    const std::string &prefix, std::string &value) const
{
    StringHashMap<std::string>::const_iterator a;
    for(a = attributes_.begin(); a != attributes_.end(); ++ a)
	value += "'" + prefix + a->first + "=" + a->second + "'";
}

//=============================================================================================
std::string Description::Stream::getValue(const std::string &name, bool critical) const
{
    std::string result;
    if (!getValue(name, result)) {
	Application::us()->error("%s: could not find attribute '%s'.", name_.c_str(), name.c_str());
	if (critical) Application::us()->respondToDelayedErrors();
    }
    return result;
}

bool Description::Stream::getValue(const std::string &name, std::string &value) const
{
    StringHashMap<std::string>::const_iterator a = attributes_.find(name);
    if (a == attributes_.end()) return false;
    value = a->second;
    return true;
}

//=============================================================================================

Description::Stream &Description::getStream(size_t index)
{
    while (index >= streams_.size())
	streams_.push_back(Stream(form("%s.%zd", name_.c_str(), streams_.size())));
    return streams_[index];
}

bool Description::verifyNumberOfStreams(size_t expectedNumber, bool critical) const
{
    if (expectedNumber != nStreams()) {
	Application::us()->error(
	    "%s: number of streams does not match: %zu expected and %zu exist.",
	    name_.c_str(), expectedNumber, nStreams());
	if (critical) Application::us()->respondToDelayedErrors();
	return false;
    }
    return true;
}

void Description::getDependencies(DependencySet &dependency) const
{
    std::string value;
    for(size_t s = 0; s < streams_.size(); ++ s) {
	streams_[s].getDependencies(form("stream-%zu:", s), value);
    }
    dependency.add("stream-extraction", value);
}
