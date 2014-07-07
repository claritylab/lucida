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
#ifndef _FLF_MODULE_HH
#define _FLF_MODULE_HH

#include <Core/Singleton.hh>

#include "Lexicon.hh"
#include "Processor.hh"



namespace Flf {

    class LatticeHandler;

    class Module_ {
    private:
	Lexicon *lexicon_;
    protected:
	Network *network_;
	Processor *processor_;
    public:
	Module_();
	~Module_();
	/*! @todo network, lexicon, processor should not be part of Flf::Module (?) */
	void init();
	const Lexicon *lexicon();
	void setLexicon(Lexicon *l);
	Network *network();
	void setNetwork(Network *n);
	Processor *processor();
	void setProcessor(Processor *p);

	Flf::LatticeHandler* createLatticeHandler(const Core::Configuration &c) const;
    };

    typedef Core::SingletonHolder<Module_> Module;
} // namespace Flf

#endif // _FLF_MODULE_HH
