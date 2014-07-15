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
// $Id: Configurable.hh 5439 2005-11-09 11:05:06Z bisani $

#ifndef _CORE_CONFIGURABLE_HH
#define _CORE_CONFIGURABLE_HH

#include "Configuration.hh"

namespace Core {

    /**
     * Abstract base class for run-time configurable classes.
     *
     * Each run-time configurable class must inherit Configurable to
     * ensure automatic creation of the selection hierarchy and passing of
     * configuration.
     */

    class Configurable {
    protected:
	Configuration config;
    public:
	Configurable(const Configuration &c) : config(c) {}

	Configuration& getConfiguration() { return config; }
	const Configuration& getConfiguration() const { return config; }

	const std::string& name() const { return config.getName(); }

	/**
	 * Run-time name of the component. This is just the
	 * configuration path.
	 */
	const std::string& fullName() const { return config.getSelection(); }

    protected:
	Configuration select(const std::string &selection) const {
	    return Configuration(config, selection);
	}
    };

}

#endif // _CORE_CONFIGURABLE_HH
