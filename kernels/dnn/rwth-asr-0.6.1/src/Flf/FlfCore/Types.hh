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
#ifndef _FLF_CORE_TYPES_HH
#define _FLF_CORE_TYPES_HH

/**
 * Some debugging stuff
 **/
//define MEM_DBG
#define dbg(message) std::cerr << __FILE__ << ":" <<__LINE__ << "\t" << message << std::endl


#include <Core/Types.hh>
#include <Fsa/Types.hh>
#include <Lattice/Types.hh>
#include <Speech/Types.hh>

namespace Flf {

    // -------------------------------------------------------------------------
    typedef Speech::TimeframeIndex Time;
    static const Time InvalidTime = Speech::InvalidTimeframeIndex;

    static const u8 AcrossWordBoundary = 0; // Lattice::AcrossWordBoundary;
    static const u8 WithinWordBoundary = 1; // Lattice::WithinWordBoundary;
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * Extensions for FSA properties.
     **/
    typedef Fsa::Property Property;
    /**
     * Cross Word context is known
     **/
    static const Property PropertyCrossWord = 0x00010000;

    /**
     * Extensions for FSA hints.
     **/
    typedef Fsa::Hint Hint;
    /**
     * Draw scores unscaled;
     * requires Fsa::HintShowDetails, exclusive to Fsa::HintAsProbability
     **/
    static const Hint HintUnscaled = 0x00000008;

    typedef Core::Vector<u8> ByteVector;

    typedef std::vector<Fsa::LabelId> LabelIdList;
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * Semiring type
     *
     * So far, only log- and tropical semiring are supported.
     *
     **/
    typedef Fsa::SemiringType SemiringType;
    SemiringType getSemiringType(const std::string &name);
    std::string getSemiringTypeName(SemiringType type);
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * Rescore mode
     *
     * Clone is always safe.
     * In-place cached is always safe in linear command networks,
     * normally faster than clone, but more memory demanding.
     * In-place is unsafe, but always fastet mode.
     *
     **/
    typedef enum {
	RescoreModeClone        = 0,
	RescoreModeInPlaceCache = 1,
	RescoreModeInPlace      = 2
    } RescoreMode;
    RescoreMode getRescoreMode(const std::string &name);
    const std::string & getRescoreModeName(RescoreMode mode);
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * Info type
     *
     * Collect information and statistics about a lattice.
     * Runtime/memory requirements:
     * cheap:    O(1), lattice is not traversed.
     * normal:   O(N), lattice is traversed exactly once, no state caching.
     * extended: O(N), lattice can be traversed several times.
     * memory:   no runtime/memory requirements.
     *
     **/
    typedef enum {
	InfoTypeCheap    = 0,
	InfoTypeNormal   = 1,
	InfoTypeExtended = 2,
	InfoTypeMemory   = 3
    } InfoType;
    InfoType getInfoType(const std::string &name);
    const std::string & getInfoTypeName(InfoType infoType);
    // -------------------------------------------------------------------------

} // namespace Flf

#endif // _FLF_CORE_TYPES_HH
