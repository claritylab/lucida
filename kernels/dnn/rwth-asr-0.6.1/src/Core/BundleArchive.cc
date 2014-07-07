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
#include "BundleArchive.hh"
#include "Directory.hh"
#include "CompressedStream.hh"
#include <iterator>
#include <algorithm>

using namespace Core;

const std::string BundleArchive::suffix_ = ".bundle";

class BundleArchive::_const_iterator :
    public Archive::_const_iterator
{
private:
    std::vector<std::string>::const_iterator iterArchive_;
    Archive::const_iterator iterFile_;
    ArchiveRef curArchive_;
    const BundleArchive &a_;
public:
    _const_iterator(const BundleArchive &a) : a_(a)
    {
	iterArchive_ = a_.archiveFiles_.begin();
	if (iterArchive_ != a_.archiveFiles_.end()) {
	    curArchive_ = Archive::create(a_.config, *iterArchive_, Archive::AccessModeRead);
	    verify(curArchive_);
	    iterFile_ = curArchive_->files();
	    name_ = iterFile_.name();
	    sizes_ = iterFile_.sizes();
	}
    }
    virtual ~_const_iterator() {
	delete curArchive_;
    }
    virtual _const_iterator& operator++ () {
	if (iterArchive_ == a_.archiveFiles_.end() || !iterFile_)
	    return *this;
	++iterFile_;
	if (!iterFile_) {
	    ++iterArchive_;
	    if (iterArchive_ != a_.archiveFiles_.end()) {
		delete curArchive_;
		curArchive_ = Archive::create(a_.config, *iterArchive_, Archive::AccessModeRead);
		iterFile_ = curArchive_->files();
	    } else {
		return *this;
	    }
	}
	name_ = iterFile_.name();
	sizes_ = iterFile_.sizes();
	return *this;
    }
    virtual operator bool () const { return (iterArchive_ != a_.archiveFiles_.end()); }
};

const Core::ParameterInt Core::BundleArchive::paramMaxOpenFiles(
    "max-open-files",
    "maximum number of files to keep open (to respect ulimit -n)",
    1024);

Core::BundleArchive::BundleArchive(const Configuration &config, const std::string &path, AccessMode access)
    : Archive(config, path, access),
      maxOpenFiles_(paramMaxOpenFiles(config))
{
    if (!isReadable(path) || !readBundleContent(path)) {
	error("cannot read bundle archive '%s'", path.c_str());
    }
    std::string index = indexFile(path);

    if (!readIndex(index)) {
	if (!createIndex()) {
	    error("cannot create index. check the archives.");
	} else {
	    if (!writeIndex(index)) {
		warning("cannot write index to '%s'", index.c_str());
	    } else {
		log("bundle archive index in '%s'", index.c_str());
	    }
	}
    }
}

Core::BundleArchive::~BundleArchive()
{
    for (ArchiveCache::iterator i = archiveCache_.begin(); i != archiveCache_.end(); ++i) {
	delete i->second;
    }
}



std::string Core::BundleArchive::indexFile(const std::string &archiveFile)
{
    return archiveFile + ".idx.gz";
}

bool Core::BundleArchive::readBundleContent(const std::string &filename)
{
    std::ifstream fin(filename.c_str());
    if (!fin)
	return false;

    while (!fin.eof()) {
	std::string archiveFile;
	fin >> archiveFile;
	if (archiveFile.empty()) continue;
	archiveFiles_.push_back(archiveFile);
    }
    return true;
}

bool Core::BundleArchive::createIndex()
{
    for (u32 i = 0; i < archiveFiles_.size(); ++i) {
	Archive *archive = Archive::create(config, archiveFiles_[i], AccessModeRead);
	if (!archive)
	    return false;
	for (Archive::const_iterator file = archive->files(); file; ++file) {
	    fileMap_[file.name()] = i;
	}
	delete archive;
    }
    return true;
}

bool Core::BundleArchive::writeIndex(const std::string &filename)
{
    CompressedOutputStream fout(filename);
    if (!fout)
	return false;
    fout << archiveFiles_.size() << "\n";
    std::copy(archiveFiles_.begin(), archiveFiles_.end(), std::ostream_iterator<std::string>(fout, "\n"));
    fout << fileMap_.size() << "\n";
    for (StringHashMap<u32>::const_iterator i = fileMap_.begin(); i != fileMap_.end(); ++i) {
	fout << i->first << " " << i->second << "\n";
    }
    return true;
}

bool Core::BundleArchive::readIndex(const std::string &filename)
{
    CompressedInputStream fin(filename);
    if (!fin)
	return false;
    u32 tmp;
    fin >> tmp;
    if (archiveFiles_.size() != tmp)
	return false;
    for (std::vector<std::string>::const_iterator f = archiveFiles_.begin(); f != archiveFiles_.end(); ++f) {
	std::string buffer;
	fin >> buffer;
	if (*f != buffer)
	    return false;
    }
    fin >> tmp;
    for (u32 i = 0; i < tmp; ++i) {
	std::string buffer;
	u32 n;
	fin >> buffer >> n;
	fileMap_[buffer] = n;
    }
    return true;
}

BundleArchive::ArchiveRef Core::BundleArchive::getArchive(const std::string &file) const
{
    StringHashMap<u32>::const_iterator idx = fileMap_.find(file);
    if (idx == fileMap_.end()) {
	return ArchiveRef();
    }
    ArchiveCache::const_iterator archive = archiveCache_.find(idx->second);
    if (archive == archiveCache_.end()) {
	ArchiveRef archive = Archive::create(config, archiveFiles_[idx->second], AccessModeRead);
	verify(archive);
	archiveCache_[idx->second] = ArchiveRef(archive);

	// Keep track of open archives and destroy the least recently used
	// if maximal allowed number is exceeded.
	lruQueue_.push(idx->second);
	if (lruQueue_.size() > maxOpenFiles_) {
	    delete archiveCache_[lruQueue_.front()];
	    archiveCache_.erase(lruQueue_.front());
	    lruQueue_.pop();
	}
    }
    return archiveCache_[idx->second];
}

bool Core::BundleArchive::discover(const std::string &name, Sizes &sizes) const
{
    ArchiveRef archive = getArchive(name);
    if (!archive) {
	return false;
    }
    return archive->discover(name, sizes);
}


bool Core::BundleArchive::read(const std::string &name, std::string &b) const
{
    ArchiveRef archive = getArchive(name);
    if (!archive)
	return false;
    archive->read(name, b);
    return true;
}

bool Core::BundleArchive::write(const std::string &name, const std::string &b, const Sizes &sizes)
{
    warning("bundle archives do not support write operations");
    return false;
}

bool Core::BundleArchive::remove(const std::string &name)
{
    warning("bundle archives do not support read operations");
    return false;
}

bool Core::BundleArchive::test(const std::string &path)
{
    if (path.rfind(suffix_) == path.length() - suffix_.length() && path.length() >= suffix_.length())
	return true;
    return false;
}

Archive::const_iterator Core::BundleArchive::files() const
{
    return const_iterator(new _const_iterator(*this));
}

bool Core::BundleArchive::clear()
{
    return false;
}

bool Core::BundleArchive::recover()
{
    return false;
}
