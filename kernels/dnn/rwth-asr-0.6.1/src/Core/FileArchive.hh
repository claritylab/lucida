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
#ifndef _CORE_FILE_ARCHIVE_HH
#define _CORE_FILE_ARCHIVE_HH

#include <iostream>
#include "Archive.hh"
#include "BinaryStream.hh"


namespace Core {

    class FileArchive : public virtual Archive {
    private:
	static const ParameterBool paramOverwrite;
	bool allowOverwrite_;
    private:
	struct FileInfo;

	BinaryStream *stream_;
	bool open_;
	bool changed_;
	std::streampos endOfArchive_;
	std::map<std::string, u32> hashedFiles_;
	std::vector<FileInfo> files_;
	std::vector<FileInfo> emptyFiles_;

	class _const_iterator;
	friend class _const_iterator;
    private:
	bool add(const FileInfo&);
	bool readFileInfoTable();
	bool writeFileInfoTable();
	u32 getChecksum(const std::string &b) const;
	FileInfo* file(const std::string &name);
	const FileInfo* file(const std::string &name) const;
	void setChanged();
	bool scanArchive();

	friend class Archive;
	static bool test(const std::string &path);

    protected:
	virtual bool discover(const std::string &name, Sizes &sizes) const;
	virtual bool read(const std::string &name, std::string &b) const;
	virtual bool write(const std::string &name, const std::string &b, const Sizes &sizes);
	virtual bool remove(const std::string &name);

    public:
	FileArchive(const Configuration &config, const std::string &path = "", AccessMode access = AccessModeReadWrite);
	virtual ~FileArchive();
	virtual const_iterator files() const;
	virtual bool clear();
	virtual bool recover();
    };

} // namespace Core

#endif // _CORE_FILE_ARCHIVE_HH
