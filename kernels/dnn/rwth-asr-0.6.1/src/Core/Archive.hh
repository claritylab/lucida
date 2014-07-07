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
#ifndef _CORE_ARCHIVE_HH
#define _CORE_ARCHIVE_HH

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Component.hh"
#include "ReferenceCounting.hh"
#include "Thread.hh"
#include "Types.hh"

namespace Core {

    /**
     * Abstract base class for archives.
     *
     * Used for abstract access to archives.
     **/
    class Archive : public Component {
    public:
	/**
	 * Different types of archives handled by this class.
	 **/
	enum Type { TypeUnknown, TypeDirectory, TypeFile, TypeBundle };

	/**
	 * Different access types to archives.
	 **/
	typedef u32 AccessMode;
	static const AccessMode AccessModeNone      = 0;
	static const AccessMode AccessModeRead      = 1;
	static const AccessMode AccessModeWrite     = 2;
	static const AccessMode AccessModeReadWrite = 3;

	typedef u32 Size;
	class Sizes {
	private:
	    Size uncompressed_;
	    Size compressed_;
	public:
	    Sizes(u32 uncompressed = 0, u32 compressed = 0)
		: uncompressed_(uncompressed), compressed_(compressed) {}
	    void setUncompressed(u32 uncompressed) { uncompressed_ = uncompressed; }
	    Size uncompressed() const { return uncompressed_; }
	    void setCompressed(u32 compressed) { compressed_ = compressed; }
	    Size compressed() const { return compressed_; }
	};

	class _const_iterator : public ReferenceCounted {
	protected:
	    std::string name_;
	    Sizes sizes_;
	public:
	    virtual ~_const_iterator() {}
	    virtual _const_iterator& operator++() = 0;
	    virtual operator bool() const = 0;
	    const std::string& name() { return name_; }
	    const Sizes& sizes() { return sizes_; }
	};
	class const_iterator {
	protected:
	    Ref<_const_iterator> iter_;
	public:
	    const_iterator(_const_iterator *iter) : iter_(iter) {}
	    const_iterator(const const_iterator &i) : iter_(i.iter_) {}
	    const_iterator() : iter_(Ref<_const_iterator>()) {}
	    const_iterator& operator++() { ++(*iter_); return *this; }
	    operator bool() const { return (*iter_); }
	    const std::string& name() { return iter_->name(); }
	    const Sizes& sizes() { return iter_->sizes(); }
	};

    private:
	std::string path_;
	AccessMode access_;
	mutable Mutex mutex_;

    protected:
	// manipulate configuration context
	bool setAccess(AccessMode access) {
	    if (access > access_) return false;
	    access_ = access;
	    return true;
	}
	void lock() const { mutex_.lock(); }
	void release() const { mutex_.release(); }

	virtual bool discover(const std::string &name, Sizes &sizes) const { return false; }
	virtual bool read(const std::string &name, std::string &buffer) const = 0;
	virtual bool write(const std::string &name, const std::string &buffer, const Sizes &sizes) = 0;
	virtual bool remove(const std::string &name) = 0;

	explicit Archive(const Configuration &config, const std::string &path = "",
			 AccessMode access = AccessModeReadWrite);

	friend class BundleArchive;
    public:
	virtual ~Archive() {}

	const std::string& path() const { return path_; }
	bool hasAccess(AccessMode a) const { return (a & access_); }
	friend std::ostream& operator<< (std::ostream &s, const Archive &a);

	virtual const_iterator files() const = 0;
	bool hasFile(const std::string &name) const;
	bool readFile(const std::string &name, std::string &buffer);
	bool writeFile(const std::string &name, const std::string &ufferb,  bool compress = true);
	bool removeFile(const std::string &name);
	bool copyFile(const Archive &src, const std::string &name, const std::string& prefix = std::string());

	/**
	 * Resets the complete archive
	 */
	virtual bool clear() = 0;

	/**
	 * Recover broken archive
	 */
	virtual bool recover() = 0;

	/**
	 * Returns the archive type of a given filesystem object.
	 * @param path valid path pointing to a filesystem object
	 **/
	static Type test(const std::string &path);

	/**
	 * Creates and returns an archive. An existing archive is
	 * auto-detected by test(). For non-existing archives
	 * the type is determined by the path name itself. A trailing
	 * path separator switches between directory or file archives.
	 * @param path destination path to the archive
	 * @return zero on failure, a valid archive otherwise
	 **/
	static Archive* create(const Configuration &config, const std::string &path = "",
			       AccessMode access = AccessModeReadWrite);
    };


    class ArchiveReader : public std::istringstream {
    private:
	bool isOpen_;
    public:
	ArchiveReader(Archive &a, const std::string &name);
	bool isOpen() const { return isOpen_; }
    };

    class ArchiveWriter : public std::ostream {
    private:
	Archive &archive_;
	std::string path_;
	bool compress_;
	bool isOpen_;
	std::string buffer_;
    public:
	ArchiveWriter(Archive &a, const std::string &name, bool compress = false);
	~ArchiveWriter();
	bool isOpen() const { return isOpen_; }
    };

} // namespace Core

#endif // _CORE_ARCHIVE_HH
