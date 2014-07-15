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
#ifndef _CORE_BUNDLE_ARCHIVE_HH
#define _CORE_BUNDLE_ARCHIVE_HH

#include <iostream>
#include <queue>
#include "Archive.hh"
#include "Hash.hh"

namespace Core {

    class BundleArchive : public virtual Archive {
    private:
	class FileInfo;

	std::vector<std::string> archiveFiles_;
	typedef Archive* ArchiveRef;
	typedef std::map<u32, ArchiveRef> ArchiveCache;
	mutable ArchiveCache archiveCache_;
	StringHashMap<u32> fileMap_;
	static const std::string suffix_;
	// Note that caching open files violates the const correctness,
	// when calling const methods, but this shouldn't cause any
	// problems here, since the object remains logically const.
	mutable std::queue<u32> lruQueue_;
	const u32 maxOpenFiles_;

	class _const_iterator;
	friend class _const_iterator;
    private:
	bool readBundleContent(const std::string &filename);
	bool readIndex(const std::string &filename);
	bool writeIndex(const std::string &filename);
	bool createIndex();
	static std::string indexFile(const std::string &archiveFile);
	ArchiveRef getArchive(const std::string &file) const;

	friend class Archive;
	static bool test(const std::string &path);
    protected:

	virtual bool discover(const std::string &name, Sizes &sizes) const;
	virtual bool read(const std::string &name, std::string &b) const;
	virtual bool write(const std::string &name, const std::string &b, const Sizes &sizes);
	virtual bool remove(const std::string &name);

    public:
	BundleArchive(const Configuration &config, const std::string &path = "", AccessMode access = AccessModeReadWrite);
	virtual ~BundleArchive();
	virtual const_iterator files() const;
	virtual bool clear();
	virtual bool recover();

	static const Core::ParameterInt paramMaxOpenFiles;
    };

} // namespace Core

#endif // _CORE_BUNDLE_ARCHIVE_HH
