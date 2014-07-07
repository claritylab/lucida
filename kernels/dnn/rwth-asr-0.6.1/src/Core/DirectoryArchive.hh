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
#ifndef _CORE_DIRECTORY_ARCHIVE_HH
#define _CORE_DIRECTORY_ARCHIVE_HH

#include "Archive.hh"
#include "Directory.hh"

namespace Core {

    class DirectoryArchive : public virtual Archive {
    private:
	bool probe(const std::string &file, const struct stat64 &state, Sizes &sizes) const;

	class _const_iterator;
	friend class _const_iterator;
	class _const_iterator : public Archive::_const_iterator {
	private:
	    DirectoryFileIterator iter_;
	    const DirectoryArchive &a_;
	public:
	    _const_iterator(const DirectoryArchive &a) : iter_(a.path(), &DirectoryFileIterator::fileFilter), a_(a) {
		if (iter_)
		    if (a_.probe(iter_.path(), iter_.state(), sizes_))
			name_ = iter_.path();
	    }
	    virtual ~_const_iterator() {}
	    virtual _const_iterator& operator++ () {
		++iter_;
		for (; iter_; ++iter_)
		    if (a_.probe(iter_.path(), iter_.state(), sizes_)) {
			name_ = iter_.path();
			return *this;
		    }
		return *this;
	    }
	    virtual operator bool () const { return iter_; }
	};

	virtual bool clear();
	virtual bool recover();

	friend class Archive;
	static bool test(const std::string &path);


    protected:
	virtual bool discover(const std::string &file, Sizes &sizes) const;
	virtual bool read(const std::string &name, std::string &b) const;
	virtual bool write(const std::string &name, const std::string &b, const Sizes &sizes);
	virtual bool remove(const std::string &name);

    public:
	DirectoryArchive(const Core::Configuration &config, const std::string &path = "",
			 AccessMode access = AccessModeReadWrite);
	virtual ~DirectoryArchive() {}
	virtual const_iterator files() const;
    };

} // namespace Core

#endif // _CORE_DIRECTORY_ARCHIVE_HH
