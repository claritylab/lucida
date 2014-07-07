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
#ifndef _FSA_OUTPUT_HH
#define _FSA_OUTPUT_HH

#include "Automaton.hh"
#include "Types.hh"

namespace Fsa {
    /**
     * storage
     **/
    bool write(      ConstAutomatonRef f, const std::string &format, std::ostream &o, StoredComponents what = storeAll, bool progress = false);
    bool write(      ConstAutomatonRef f, const std::string &file, StoredComponents what = storeAll, bool progress = false);

    bool writeAtt(   ConstAutomatonRef f, std::ostream &o, StoredComponents what = storeAll, bool progress = false);
    bool writeAtt(   ConstAutomatonRef f, const std::string &file, StoredComponents what = storeAll, bool progress = false);

    bool writeBinary(ConstAutomatonRef f, std::ostream &o, StoredComponents what = storeAll, bool progress = false);
    bool writeBinary(ConstAutomatonRef f, const std::string &file, StoredComponents what = storeAll, bool progress = false);

    bool writeLinear(ConstAutomatonRef f, std::ostream &o, StoredComponents what = storeAll, bool progress = false, bool printAll=false);
    bool writeLinear(ConstAutomatonRef f, const std::string &file, StoredComponents what = storeAll, bool progress = false);

    bool writeXml(   ConstAutomatonRef f, std::ostream &o, StoredComponents what = storeAll, bool progress = false);
    bool writeXml(   ConstAutomatonRef f, const std::string &file, StoredComponents what = storeAll, bool progress = false);

    bool writeTrXml(  ConstAutomatonRef f, std::ostream &o, StoredComponents what = storeAll, bool progress = false);
    bool writeTrXml(  ConstAutomatonRef f, const std::string &file, StoredComponents what = storeAll, bool progress = false);

    /*
    bool writeMmap(  ConstAutomatonRef f, std::ostream &o, StoredComponents what = storeAll, bool progress = false);
    bool writeMmap(  ConstAutomatonRef f, const std::string &file, StoredComponents what = storeAll, bool progress = false);
    */

    /*
    bool writeHtk(   ConstAutomatonRef f, std::ostream &o, StoredComponents what = storeAll, bool progress = false);
    bool writeHtk(   ConstAutomatonRef f, const std::string &file, StoredComponents what = storeAll, bool progress = false);
    */


    /**
     * visualization
     **/
    bool drawDot(ConstAutomatonRef f, std::ostream &o, Hint hint = HintNone, bool progress = false);
    bool drawDot(ConstAutomatonRef f, const std::string &file, Hint hint = HintNone, bool progress = false);
} // namespace Fsa

#endif // _FSA_OUTPUT_HH
