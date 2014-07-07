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
#include <sys/stat.h>
#include <unistd.h>
#if !defined(OS_linux) && !defined(truncate64)
#define truncate64 truncate
#endif
#include <Core/ProgressIndicator.hh>
#include "CompressedStream.hh"
#include "Directory.hh"
#include "FileArchive.hh"


using namespace Core;

/*
 * --- file format ------------------------------------------------------------
 *   8   bytes              archive file header: 'SP_ARC_1'
 *   1   byte               flag:  = 0 file info table does not exist,
 *                                != 0 file info table does exist
 *   1.  file data / empty file data
 *   2.  file data / empty file data
 *   ...
 *   n.  file data / empty file data
 *   optional: file info table
 *
 * --- file data --------------------------------------------------------------
 *   4   bytes              0xaa55aa55 recovery tag
 *   ?   bytes              path + filename: 4 byte string size + string without '\0'
 *   4   bytes              size of compressed file
 *   4   bytes              size of uncompressed file (0 = no compression)
 *   4   bytes              checksum (unused)
 *   ...                    data
 *   4   bytes              0x55aa55aa recovery tag
 *
 * --- empty file data --------------------------------------------------------
 *   4   bytes              0xaa55aa55 recovery tag
 *   4   byte               0
 *   4   bytes              size of data
 *   4   bytes              0
 *   4   bytes              0
 *   ...                    data
 *   4   bytes              0x55aa55aa recovery tag
 *
 * --- file info table --------------------------------------------------------
 *   4   bytes              number of files
 *   1.  file info
 *   2.  file info
 *   ...
 *   n.  file info
 *
 *   4   bytes              number of empty files
 *   1.  empty file info
 *   2.  empty file info
 *   ...
 *   n.  empty file info
 *
 *   8   bytes              seek position to first empty file info block
 *   8   bytes              seek position to first file info block
 *
 * --- file info --------------------------------------------------------------
 *   ?   bytes              path + filename: 0-terminated string
 *   8   bytes              seek position within archive
 *   4   bytes              size of compressed file
 *   4   bytes              size of uncompressed file
 *
 * --- empty file info --------------------------------------------------------
 *   8   bytes              seek position within archive
 *   4   bytes              size of data
 */

static const char header[8] = "SP_ARC1";
static const u32  recoveryStartTag = 0xaa55aa55;
static const u32  recoveryEndTag   = 0x55aa55aa;

const ParameterBool FileArchive::paramOverwrite(
    "allow-overwrite", "allow overwriting of existing files", false);

// ===========================================================================
struct FileArchive::FileInfo {
    std::string name;
    u64 position;
    Sizes sizes;

    FileInfo(const std::string &n, u64 p, const Sizes &s) :
	name(n), position(p), sizes(s) {}

    void clear() {
	name.clear(); position = 0;
    }

    bool isEmpty() const {
	return (name.empty() && position == 0);
    }

    struct Empty : public std::unary_function<FileInfo, bool> {
	bool operator()(const FileInfo &f) const {
	    return f.isEmpty();
	}
    };

};

// ===========================================================================
class FileArchive::_const_iterator :
    public Archive::_const_iterator
{
private:
    std::vector<FileInfo>::const_iterator iter_;
    const FileArchive &a_;
public:
    _const_iterator(const FileArchive &a) : iter_(a.files_.begin()), a_(a) {
	if (iter_ != a_.files_.end()) {
	    name_ = iter_->name;
	    sizes_ = iter_->sizes;
	}
    }
    virtual ~_const_iterator() {}
    virtual _const_iterator& operator++ () {
	if (iter_ == a_.files_.end()) return *this;
	++iter_;
	if (iter_ != a_.files_.end()) {
	    name_ = iter_->name;
	    sizes_ = iter_->sizes;
	}
	return *this;
    }
    virtual operator bool () const { return (iter_ != a_.files_.end()); }
};

// ===========================================================================
// class FileArchive

FileArchive::FileInfo* FileArchive::file(const std::string &name) {
    // Check consistency, since '//', '/./' are normalized by directory archive but are not by file archive.
    require(normalizePath(name) == name);
    std::map<std::string, u32>::iterator i = hashedFiles_.find(name);
    return (i == hashedFiles_.end() ? 0 : &files_[i->second]);
}

const FileArchive::FileInfo* FileArchive::file(const std::string &name) const {
    // Check consistency, since '//', '/./' are normalized by directory archive but are not by file archive.
    if (normalizePath(name) != name)
	criticalError("Filename \"%s\" contains special character(e.g. \"//\") sequences that will cause different behaviour of file and directory archives.", name.c_str());
    std::map<std::string, u32>::const_iterator i = hashedFiles_.find(name);
    return (i == hashedFiles_.end() ? 0 : &files_[i->second]);
}


/*****************************************************************************/
FileArchive::FileArchive(const Configuration &c, const std::string &p, AccessMode access)
    : Archive(c, p, access),
      allowOverwrite_(paramOverwrite(c)),
      stream_(0), open_(false), changed_(false)
/*****************************************************************************/
{
    // create file archive if necessary
    open_ = false;
    struct stat64 fs;
    if (stat64(path().c_str(), &fs) != 0) {
	// create new file archive
	if (access & AccessModeWrite) {
	    createDirectory(directoryName(path()));
	    stream_ = new BinaryStream(path(), std::ios::in | std::ios::out | std::ios::trunc);
	    // write header
	    stream_->write(&header[0], 8);
	    *stream_ << u8(0);
	    if (stream_->good()) {
		open_ = true;
		changed_ = true;
		endOfArchive_ = stream_->BinaryInputStream::position();
	    } else
		error("Failed to create archive file \"%s\".", path().c_str());
	} else {
	    error("Archive file \"%s\" does not exist.", path().c_str());
	}
    } else {
	// open existing file archive
	std::ios::openmode mode = std::ios::in;
	if (access & AccessModeWrite) mode |= std::ios::out;
	stream_ = new BinaryStream(path(), mode);
	if (!*stream_) {
	    error("Failed to open archive file \"%s\".", path().c_str());
	    return;
	}

	// read and check header
	u8 _header[8];
	stream_->read(&_header[0], 8);
	if (!*stream_ || memcmp(header, _header, 8)) {
	    error("No file archive header detected in file \"%s\".", path().c_str());
	    return;
	}

	open_ = true;

	readFileInfoTable();
    }
}

/*****************************************************************************/
FileArchive::~FileArchive()
/*****************************************************************************/
{
    if (open_) {
	writeFileInfoTable();
	if (stream_) delete stream_;
    }
}

/*****************************************************************************/
bool FileArchive::clear()
/*****************************************************************************/
{
    if (stream_) delete stream_;
    files_.clear(); hashedFiles_.clear(); emptyFiles_.clear();
    stream_ = new BinaryStream(path(), std::ios::in | std::ios::out | std::ios::trunc);
    stream_->write(&header[0], 8);
    *stream_ << u8(0);
    open_ = true;
    changed_ = true;
    endOfArchive_ = stream_->BinaryInputStream::position();
    return readFileInfoTable();
}

/*****************************************************************************/
bool FileArchive::add(const FileInfo &info)
/*****************************************************************************/
{
    if (hashedFiles_.find(info.name) != hashedFiles_.end())
	return false;
    hashedFiles_[info.name] = files_.size();
    files_.push_back(info);
    return true;
}

/*****************************************************************************/
bool FileArchive::remove(const std::string &name)
/*****************************************************************************/
{
    /*! @todo merge empty segments
     */
    std::map<std::string, u32>::iterator i = hashedFiles_.find(name);
    if (i == hashedFiles_.end())
	return false;
    verify(i->second < files_.size() && i->second >= 0);
    FileInfo info = files_[i->second];

    stream_->synchronizeBuffer();
    stream_->clear();
    std::streamoff begin = info.position - (sizeof(recoveryStartTag) + info.name.size() + sizeof(u32));
    u32 size = (info.sizes.compressed() ? info.sizes.compressed() : info.sizes.uncompressed());
    stream_->BinaryInputStream::seek(begin, std::ios::beg);
    u32 tag = 0;
    *stream_ >> tag;
    if (tag != recoveryStartTag) {
	error("remove failed: recovery start tag not found at position %ld",  (long int)begin);
	return false;
    }
    setChanged();
    if (std::streampos(info.position + 3 * sizeof(u32) + size + sizeof(recoveryEndTag)) == endOfArchive_) {
	// segment is the last segment in archive -> shrink file
	endOfArchive_ = begin;
    } else {
	stream_->BinaryOutputStream::seek(begin + sizeof(recoveryStartTag), std::ios::beg);
	*stream_ << u32(0); // mark as empty file
	std::streampos pos = stream_->BinaryOutputStream::position();

	size += info.name.size();
	*stream_ << size;
	*stream_ << u32(0) << u32(0); // compressed, checksum
	std::streampos curpos = stream_->BinaryOutputStream::position();
	stream_->BinaryInputStream::seek(curpos + std::streampos(size), std::ios::beg);
	*stream_ >> tag;
	verify(tag == recoveryEndTag);
	emptyFiles_.push_back(FileInfo("", pos, Sizes(size, 0)));
    }

    // update file lists
    files_[i->second].clear();
    hashedFiles_.erase(i);
    stream_->synchronizeBuffer();
    return true;
}

/*****************************************************************************/
void FileArchive::setChanged()
/*****************************************************************************/
{
    if (!changed_) {
	stream_->BinaryInputStream::seek(sizeof(header), std::ios::beg);
	*stream_ << u8(0); // no file info table
	changed_ = true;
    }
}

/*****************************************************************************/
bool FileArchive::readFileInfoTable()
/*****************************************************************************/
{
    // look for valid file info table
    stream_->clear();
    stream_->BinaryInputStream::seek(sizeof(header), std::ios::beg);
    u8 ok;
    *stream_ >> ok;
    if (ok) {
	u64 pos;
	stream_->BinaryInputStream::seek(-8, std::ios::end);
	*stream_ >> pos;
	endOfArchive_ = pos;
	stream_->BinaryInputStream::seek(pos);

	// get file info
	u32 count;
	*stream_ >> count;
	for (u32 i = 0; i < count; i++) {
	    std::string name;
	    u32 size, compressed;
	    *stream_ >> name;
	    *stream_ >> pos;
	    *stream_ >> size;
	    *stream_ >> compressed;
	    if (*stream_)
		add(FileInfo(name, pos, Sizes(size, compressed)));
	}
	if (!*stream_) {
	    error("Failed to read file info table from archive \"%s\".",
		  path().c_str());
	}
	*stream_ >> count;
	for (u32 i = 0; i < count; i++) {
	    u32 size;
	    *stream_ >> pos;
	    *stream_ >> size;
	    emptyFiles_.push_back(FileInfo("", pos, Sizes(size, 0)));
	}
	if (!*stream_) {
	    error("Failed to read empty file list from archive \"%s\".",
		  path().c_str());
	}

    } else {
	warning("No file info table available in archive \"%s\". We try a manual scan.",
		path().c_str());
	if (!scanArchive())
	    error("Failed to scan archive.");
	log("Scan complete: detected %zd files.", files_.size());
    }
    return true;
}

/*****************************************************************************/
bool FileArchive::scanArchive()
/*****************************************************************************/
{
    stream_->clear();
    stream_->BinaryInputStream::seek(sizeof(header) + sizeof(u8), std::ios::beg);

    endOfArchive_ = stream_->BinaryInputStream::position();
    Core::ProgressIndicator pi("scanning archive", "entries");
    pi.start();
    while (*stream_) {
	u32 tmp = 0;
	*stream_ >> tmp;
	if (!stream_->good()) break;
	if (tmp != recoveryStartTag) {
	    warning("No start tag found at stream position %lld",
		  (long long int)(stream_->BinaryInputStream::position()) - 4);
	    continue;
	}
	std::string name;
	u32 size, compressed, checksum;
	*stream_ >> name;
	std::streampos pos = stream_->BinaryInputStream::position();
	*stream_ >> size;
	*stream_ >> compressed;
	*stream_ >> checksum;
	if (name.empty()) {
	    // empty file
	    stream_->BinaryInputStream::seek(size, std::ios::cur);
	    *stream_ >> tmp;
	    if (!stream_->good()) break;
	    emptyFiles_.push_back(FileInfo("", pos, Sizes(size, 0)));
	} else {
	    // regular file
	    if (compressed) stream_->BinaryInputStream::seek(compressed, std::ios::cur);
	    else stream_->BinaryInputStream::seek(size, std::ios::cur);
	    tmp = 0;
	    *stream_ >> tmp;
	    if (!stream_->good()) break;
	    add(FileInfo(name, pos, Sizes(size, compressed)));
	}
	if (tmp != recoveryEndTag) {
	    warning("No end tag found at stream position %lld",
		  (long long int)(stream_->BinaryInputStream::position()) - 4);
	} else {
	    endOfArchive_ = stream_->BinaryInputStream::position();
	}
	pi.notify();
    }
    pi.finish();
    return stream_->eof();
}

/*****************************************************************************/
bool FileArchive::writeFileInfoTable()
/*****************************************************************************/
{
    if ((changed_) && hasAccess(AccessModeWrite)) {
	// write file info table
	stream_->clear();
	stream_->BinaryInputStream::seek(endOfArchive_, std::ios::beg);
	std::streampos fileTableStart = stream_->BinaryOutputStream::position();
	u32 tmp = files_.size() - std::count_if (files_.begin(), files_.end(), FileInfo::Empty());
	*stream_ << tmp;
	for (std::vector<FileInfo>::const_iterator i = files_.begin(); i != files_.end(); ++i) {
	    if (i->isEmpty()) continue;
	    *stream_ << i->name;
	    *stream_ << u64(i->position);
	    *stream_ << u32(i->sizes.uncompressed());
	    *stream_ << u32(i->sizes.compressed());
	}

	// write empty file info table
	std::streampos emptyFileTableStart = stream_->BinaryOutputStream::position();
	tmp = emptyFiles_.size();
	*stream_ << tmp;
	for (std::vector<FileInfo>::const_iterator i = emptyFiles_.begin(); i != emptyFiles_.end(); ++i) {
	    *stream_ << u64(i->position);
	    *stream_ << u32(i->sizes.uncompressed());
	}

	// finish archive:
	// 1. write seek position to empty file info table
	// 2. write seek position to file info table
	// 3. set finish flag to 1
	*stream_ << u64(emptyFileTableStart);
	*stream_ << u64(fileTableStart);
	if (!stream_->good())
	    return false;
	std::streampos fileSize = stream_->BinaryOutputStream::position();
	stream_->BinaryInputStream::seek(sizeof(header), std::ios::beg);
	*stream_ << u8(1); // file info table exists

	// truncate file to remove junk at the the of the file
	stream_->close();
	delete stream_;
	if (truncate64(path().c_str(), fileSize) != 0) {
	    warning("cannot truncate file");
	}
	stream_ = new BinaryStream(path(), std::ios::in | std::ios::out);
    }
    return true;
}

/*****************************************************************************/
u32 FileArchive::getChecksum(const std::string &b) const
/*****************************************************************************/
{
    u32 checksum = 0;
    return checksum;
}

/*****************************************************************************/
bool FileArchive::discover(const std::string &name, Sizes &sizes) const
/*****************************************************************************/
{
    const FileInfo *fi = file(name);
    if (!fi) return false;
    sizes = fi->sizes;
    return true;
}

/*****************************************************************************/
bool FileArchive::read(const std::string &name, std::string &b) const
/*****************************************************************************/
{
    require(hasAccess(AccessModeRead));
    if (!open_)
	return false;
    const FileInfo *fi = file(name);
    if (!fi)
	return false;

    stream_->clear();
    stream_->BinaryInputStream::seek(fi->position);
    u32 tmp;
    *stream_ >> tmp;  // read size
    *stream_ >> tmp;  // read compressed
    *stream_ >> tmp;  // read checksum
    bool ok;
    if (fi->sizes.compressed())
	ok = stream_->read(&b[0], fi->sizes.compressed());
    else
	ok = stream_->read(&b[0], fi->sizes.uncompressed());
    // check if checksum is ok
    if (getChecksum(b) != tmp)
	ok = false;

    return ok;
}

/*****************************************************************************/
bool FileArchive::write(const std::string &name, const std::string &b, const Sizes &sizes)
/*****************************************************************************/
{
    require(hasAccess(AccessModeWrite));
    if (!open_)
	return false;
    FileInfo *fi = file(name);
    if (fi) {
	if (!allowOverwrite_) {
	    error("Overwriting is not allowed. Change parameter '%s'.", paramOverwrite.name().c_str());
	    return false;
	} else {
	    remove(fi->name);
	}
    }

    // clear global write tag
    stream_->clear(); // stream state might be corrupted by previous action
    setChanged();

    // look for suitable empty file first
    // if not found => append to archive
    // else => seek to position, write to archive and append remaining empty
    // file to list if not too small
    /*! @todo improve empty block usage */
    FileInfo emptyFile("", 0, Sizes(0, 0));
    u32 neededSize = b.size() + name.size();
    for (std::vector<FileInfo>::iterator e = emptyFiles_.begin();
	 e != emptyFiles_.end(); ++e) {
	if (e->sizes.uncompressed() == neededSize) {
	    emptyFile = *e;
	    emptyFiles_.erase(e);
	    break;
	}
    }

    bool append = emptyFile.isEmpty();
    if (!append) {
	stream_->BinaryInputStream::seek(emptyFile.position - (sizeof(recoveryStartTag) + sizeof(u32)),
					 std::ios::beg);
    } else {
	stream_->BinaryInputStream::seek(endOfArchive_, std::ios::beg);
    }

    u32 tmp = recoveryStartTag;
    *stream_ << tmp;
    *stream_ << name;
    std::streampos pos = stream_->BinaryOutputStream::position();
    *stream_ << u32(sizes.uncompressed());
    *stream_ << u32(sizes.compressed());
    *stream_ << getChecksum(b);
    stream_->write((const char*)&(b.c_str())[0], b.size());
    tmp = recoveryEndTag;
    *stream_ << tmp;

    if (append)
	endOfArchive_ = stream_->BinaryOutputStream::position();
    add(FileInfo(name, pos, sizes));

    return true;
}

/*****************************************************************************/
bool FileArchive::recover()
/*****************************************************************************/
{
    files_.clear(); hashedFiles_.clear(); emptyFiles_.clear();
    if (!scanArchive()) {
	error("Failed to scan archive");
	return false;
    }
    log("Scan complete: detected %zd files.", files_.size());
    setChanged();
    return writeFileInfoTable();
}

/*****************************************************************************/
Archive::const_iterator FileArchive::files() const
/*****************************************************************************/
{
    return const_iterator(new _const_iterator(*this));
}

/*****************************************************************************/
bool FileArchive::test(const std::string &path)
/*****************************************************************************/
{
    struct stat64 fs;
    if (stat64(path.c_str(), &fs) < 0) {
	if (path[path.size() - 1] != '/') return true;
    } else if (S_ISREG(fs.st_mode)) {
	BinaryStream stream(path, std::ios::in);
	u8 _header[8];
	stream.read(&_header[0], 8);
	if ((stream) && (memcmp(header, _header, 8) == 0)) return true;
    }
    return false;
}
