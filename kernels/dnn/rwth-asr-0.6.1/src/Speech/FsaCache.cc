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
#include "FsaCache.hh"
#include <Core/Archive.hh>
#include <Core/Dependency.hh>
#include <Fsa/Static.hh>
#include <Fsa/Input.hh>
#include <Fsa/Output.hh>

using namespace Speech;


const Core::ParameterString FsaCache::paramPath(
    "path", "path of model acceptor cache");

const Core::ParameterBool FsaCache::paramReadOnly(
    "read-only", "if true cache is only read", false);

FsaCache::FsaCache(const Core::Configuration &c, Fsa::StoredComponents whatToStore) :
    Core::Component(c),
    archive_(0),
    whatToStore_(whatToStore),
    readOnly_(false)
{
    readOnly_ = paramReadOnly(config);
    std::string path(paramPath(c));
    if (!path.empty()) {
	Core::Archive::AccessMode accessMode = Core::Archive::AccessModeReadWrite;
	if (readOnly_) accessMode = Core::Archive::AccessModeRead;
	archive_ = Core::Archive::create(c, path, accessMode);
	if (!archive_) error("Failed to create archive '%s'.", path.c_str());
    }
}

FsaCache::~FsaCache()
{
    delete archive_;
}

bool FsaCache::setDependencies(const Core::DependencySet &dependencies)
{
    if (!archive_) return true;

    bool isClean = false;
    Core::ArchiveReader ar(*archive_, "DEPENDENCIES");
    if (ar.isOpen()) {
	Core::DependencySet cacheDependencies;
	if (cacheDependencies.read(config, ar)) {
	    if (dependencies.satisfies(cacheDependencies))
		isClean = true;
	} else
	    error("Failed to read dependencies from model acceptor cache.");
    }

    /*! \warning Apparently dependencies on Alpha and Intel differ. */

    if (isClean) {
	log("Using clean model acceptor cache.");
    } else {
	if (readOnly_)
	    criticalError("Read-only flag set but the cache is not clean.");

	if (ar.isOpen()) warning("Cache is dirty.");

	Core::XmlOutputStream xw(new Core::ArchiveWriter(*archive_, "DEPENDENCIES"));
	dependencies.write(xw);
    }
    return isClean;
}

Fsa::ConstAutomatonRef FsaCache::read(const std::string &id)
{
    verify(archive_);

    Core::Ref<Fsa::StorageAutomaton> result;
    Core::ArchiveReader reader(*archive_, id);
    if (reader.isOpen()) {
	result = Core::ref(new Fsa::StaticAutomaton());
	if (!Fsa::readBinary(result.get(), reader)) {
	    result.reset();
	    error("Failed to read the object \"%s\" from cache \"%s\"",
		  id.c_str(), archive_->path().c_str());
	} else {
	    if (inputAlphabet_) result->setInputAlphabet(inputAlphabet_);
	    if (outputAlphabet_) result->setOutputAlphabet(outputAlphabet_);
	}
    }
    return result;
}

void FsaCache::write(const std::string &id, Fsa::ConstAutomatonRef fsa)
{
    verify(archive_);

    Core::ArchiveWriter writer(*archive_, id, true);
    if (!Fsa::writeBinary(fsa, writer, whatToStore_)) {
	error("Failed to write the object \"%s\" to cache \"%s\"",
	      id.c_str(), archive_->path().c_str());
    }
}
