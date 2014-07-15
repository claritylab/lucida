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
#ifndef _CORE_DIRECTORY_HH
#define _CORE_DIRECTORY_HH

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <stack>

namespace Core {

    /** Return the directory name of a pathname.
     * @param path a pathname
     * @return directory name of pathname @c path.  If there is no
     * slash in @c path, the result will be empty.  The result will
     * have no trailing slashes unless it is the root */
    std::string directoryName(const std::string &path);

    /** Strip directory from filename.
     * @param path a pathname
     * @return pathname @c with any leading directory components
     * removed.  If @c path ends with a slash, the result will be
     * empty. */
    std::string baseName(const std::string &path);

    /** Join two path components intelligently.
     * @param path1 first pathname component
     * @param path2 second pathname component
     * @return the concatenation of @c path1 and @c path2, with
     * exactly one slash ('/') inserted in between, unless one of them
     * is empty.  If @c path2 is an absolute path @c path1 is thrown
     * away. */
    std::string joinPaths(const std::string &path1, const std::string &path2);

    /** Normalize a pathname.
     * This collapses redundant separators and up-level references,
     * e.g. A//B, A/./B and A/foo/../B all become A/B.
     * It should be understood that this may change the meaning of the
     * path if it contains symbolic links. */
    std::string normalizePath(const std::string &path);

    /** Get filename extension.
     * If @c path is of the form "name.ext" return ".ext".  Returns
     * the empty string if @path does not contain ".".
     */
    std::string filenameExtension(const std::string &path);

    /** Strips the extension of the @param path.
     *  If @param path does not have an extenstion @return is @param path without changes.
     */
    std::string stripExtension(const std::string &path);

    /** Create directory.
     * Create given directory @c path.  If @path consists of several
     * components parent directories are created as needed. (similar
     * to "mkdir -p")
     * @return true if @c path was created successfully or already existed. */
    bool createDirectory(const std::string &path);

    /** Remove all contents of a directory recursively. */
    bool clearDirectory(const std::string &path);

    /** Remove a directory.
     * This recursively removes all contents from the given directory,
     * then removes the directory itself (similar to "rm -r"). */
    bool removeDirectory(const std::string &path);

    /** Test for pathname.
     * @return true if @c path exists. */
    bool isValidPath(const std::string &path);

    /** Test for directory.
     * @return true if @c path exists and is a directory. */
    bool isDirectory(const std::string &path);

    /** Test for regular file.
     * @return true if @c path exists and is a regular file. */
    bool isRegularFile(const std::string &path);

    /** Test for symbolic link.
     * @return true if @c path exists and is a symbolic link */
    bool isSymbolicLink(const std::string &path);

    /** Test if file / directory is readable.
     * @return true if @c path exists and is readable */
    bool isReadable(const std::string &path);

    /** Test if file / directory is writable.
     * @return true if @c path exists and is writable */
    bool isWritable(const std::string &path);

    /** Test if file / directory is executable.
     * @return true if @c path exists and is executable */
    bool isExecutable(const std::string &path);



    /** DirectoryFileIterator iterates recursively thought the directory
     *
     * Filter is a function object implementing the
     * bool operator()(const std::string &path, const struct stat64& state,
     *                 uid_t uid, gid_t gid) function.
     * Return true, if the iteration should yield the given
     * element. Return false, it it should ignore it.
     */

    class DirectoryFileIterator {
    public:
	/** Abstract base class for directory iterator filters. */
	struct Filter {
	    virtual bool operator()(const std::string &path,
				    const struct stat64& state,
				    uid_t uid, gid_t gid) const = 0;
	    virtual ~Filter() {}
	};

	/** Return all files and directories*/
	struct AllFilter : Filter {
	    AllFilter() {} // make it non-POD
	    virtual bool operator()(const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const;
	};
	static const AllFilter allFilter;

	/** Return only the directories */
	struct DirectoryFilter : Filter {
	    DirectoryFilter() {} // non-POD
	    virtual bool operator()(const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const;
	};
	static const DirectoryFilter directoryFilter;

	/** Return only regular files */
	struct FileFilter : Filter {
	    FileFilter() {} // non-POD
	    virtual bool operator()(const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const;
	};
	static const FileFilter fileFilter;

	/** Return only the files whose name contains the given string*/
	class FileNameFilter : public Filter {
	protected:

	    std::string name_;
	    bool exactMatch_;

	public:
	    FileNameFilter(const std::string &name, bool exactMatch = false) :
		name_(name), exactMatch_(exactMatch) {}

	    virtual bool operator()(const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const;
	};

	/** Return only the files whose name end on the given string */

	struct FileExtensionFilter : public FileNameFilter {
	    FileExtensionFilter(const std::string &name) : FileNameFilter(name) {}
	    virtual bool operator()(const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const;
	};


    private:
	bool end_;
	uid_t uid_;
	gid_t gid_;
	std::string base_;
	std::string path_;
	struct stat64 state_;
	const Filter *filter_;

	typedef std::pair<std::string, DIR*> DirectoryId;
	std::stack<DirectoryId> directoryStack_;

	bool pushPath();
	void popPath();
	bool canEnterPath();
	struct dirent *nextEntry();

    public:
	DirectoryFileIterator(const std::string &path, const Filter *filter = &allFilter);
	DirectoryFileIterator& operator++();

	uid_t uid() const { return uid_; }
	gid_t gid() const { return gid_; }
	const std::string& base() const { return base_; }
	const std::string& path() const { return path_; }
	const struct stat64& state() const { return state_; }
	operator bool() const { return !end_; }
    };

} // namespace Core

#endif // _CORE_DIRECTORY_HH
