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
#include <Core/Application.hh>
#include <Core/Choice.hh>

#include "Types.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    namespace {
	Core::Choice SemiringTypeChoice(
	    "log",      Fsa::SemiringTypeLog,
	    "tropical", Fsa::SemiringTypeTropical,
	    Core::Choice::endMark());
    } // namespace
    SemiringType getSemiringType(const std::string &s) {
	Core::Choice::Value type = SemiringTypeChoice[s];
	if (type == Core::Choice::IllegalValue)
	    return Fsa::SemiringTypeUnknown;
	else
	    return SemiringType(type);
    }
    std::string getSemiringTypeName(SemiringType type) {
	return SemiringTypeChoice[type];
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	const Core::Choice RescoreModeNameChoice(
	    "clone",           RescoreModeClone,
	    "in-place-cached", RescoreModeInPlaceCache,
	    "in-place",        RescoreModeInPlace,
	    Core::Choice::endMark());
    } // namespace
    RescoreMode getRescoreMode(const std::string &s) {
	Core::Choice::Value mode = RescoreModeNameChoice[s];
	if (mode == Core::Choice::IllegalValue)
	    Core::Application::us()->criticalError("Unknown rescore mode \"%s\".", s.c_str());
	return RescoreMode(mode);
    }
    const std::string & getRescoreModeName(RescoreMode mode) {
	return RescoreModeNameChoice[mode];
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	const Core::Choice InfoTypeNameChoice(
	    "cheap",    InfoTypeCheap,
	    "normal",   InfoTypeNormal,
	    "extended", InfoTypeExtended,
	    "memory",   InfoTypeMemory,
	    Core::Choice::endMark());
    } // namespace
    InfoType getInfoType(const std::string &s) {
	Core::Choice::Value type = InfoTypeNameChoice[s];
	if (type == Core::Choice::IllegalValue)
	    Core::Application::us()->criticalError("Unknown info type \"%s\".", s.c_str());
	return InfoType(type);
    }
    const std::string & getInfoTypeName(InfoType type) {
	return InfoTypeNameChoice[type];
    }
    // -------------------------------------------------------------------------

} // namespace Flf
