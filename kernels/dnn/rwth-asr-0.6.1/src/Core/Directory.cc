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
#include "Directory.hh"
#include "Tokenizer.hh"
#include <vector>
#include <errno.h>
#include <unistd.h>
#include <cstring>

#include "Assertions.hh"

using namespace Core;

/*****************************************************************************/
std::string Core::directoryName(const std::string &path)
/*****************************************************************************/
{
    std::string result(path);
    std::string::size_type i = result.find_last_of("/"), j;
    if (i == std::string::npos) {
	j = 0; // no slash at all -> no dir
    } else {
	j = result.find_last_not_of("/", i);
	if (j == std::string::npos) {
	    j = 1; // only leading slashed -> remove everything except for a signle leading slash
	} else {
	    j += 1; // remove all trailing slashes
	}
    }
    result.erase(j);
    return result;
}

/*****************************************************************************/
std::string Core::baseName(const std::string &path)
/*****************************************************************************/
{
    std::string::size_type i = path.find_last_of("/");
    if (i == std::string::npos) {
	return path; // no slash at all -> no dir to strip
    } else {
	return path.substr(i+1);
    }
}

/*****************************************************************************/
std::string Core::joinPaths(const std::string &a, const std::string &b)
/*****************************************************************************/
{
    if (b.size() == 0) return a;
    if (b[0] == '/') return b;
    std::string result(a);
    if (result.size() && result.end()[-1] != '/')
	result += '/';
    result += b;
    return result;
}

/*****************************************************************************/
std::string Core::normalizePath(const std::string &path)
/*****************************************************************************/
{
    if (path == "") return ".";

    size_t nInitialSlashes = 0;
    while ((path.size() > nInitialSlashes) && (path[nInitialSlashes] == '/'))
	++nInitialSlashes;
    if (nInitialSlashes > 2)
	nInitialSlashes = 1;

    StringTokenizer components(path, "/");
    std::vector<std::string> newComponents;
    for (StringTokenizer::Iterator comp = components.begin(); comp != components.end(); ++comp) {
	std::string component(*comp);
	if (component.size() == 0 || component == ".")
	    continue;
	else if ((component == "..") && !(newComponents.size() == 0 || newComponents.back() == ".."))
	    newComponents.pop_back();
	else
	    newComponents.push_back(component);
    }

    std::string result(nInitialSlashes, '/');
    for (std::vector<std::string>::const_iterator c = newComponents.begin(); c != newComponents.end(); ++c) {
	if (c != newComponents.begin()) result += "/";
	result += *c;
    }

    return result;
}

/*****************************************************************************/
std::string Core::filenameExtension(const std::string &path)
/*****************************************************************************/
{
    std::string::size_type i = path.find_last_of("./");
    if (i != std::string::npos && path[i] == '.')
	return path.substr(i);
    return std::string();
}

/*****************************************************************************/
std::string Core::stripExtension(const std::string &path)
/*****************************************************************************/
{
    std::string::size_type i = path.find_last_of("./");
    if (i != std::string::npos && path[i] == '.')
	return path.substr(0, i);
    return path;
}

/*****************************************************************************/
bool Core::createDirectory(const std::string &path)
/*****************************************************************************/
{
    for (std::string::size_type i = 1; i <= path.size(); i++)
	if ((path[i] == '/') || (i == path.size())) {
	    if (mkdir(path.substr(0, i).c_str(), 0777) != 0) {
		if (errno == EEXIST) errno = 0;
		else return false;
	    }
	}
    return true;
}

/*****************************************************************************/
bool Core::clearDirectory(const std::string &path)
/*****************************************************************************/
{
    for (DirectoryFileIterator file(path, &DirectoryFileIterator::fileFilter); file; ++file) {
	if (unlink(joinPaths(file.base(), file.path()).c_str()) != 0) return false;
    }
    for (DirectoryFileIterator directory(path, &DirectoryFileIterator::directoryFilter); directory; ++directory) {
	if (!removeDirectory(joinPaths(directory.base(), directory.path()))) return false;
    }
    return true;
}

/*****************************************************************************/
bool Core::removeDirectory(const std::string &path)
/*****************************************************************************/
{
    return (clearDirectory(path) && (rmdir(path.c_str()) == 0));
}

/*****************************************************************************/
bool Core::isValidPath(const std::string &path)
/*****************************************************************************/
{
    struct stat64 state;
    return (stat64(path.c_str(), &state) == 0);
}

/*****************************************************************************/
bool Core::isDirectory(const std::string &path)
/*****************************************************************************/
{
    struct stat64 state;
    if (stat64(path.c_str(), &state) != 0)
	return false;

    return S_ISDIR(state.st_mode);
}

/*****************************************************************************/
bool Core::isRegularFile(const std::string &path)
/*****************************************************************************/
{
    struct stat64 state;
    if (stat64(path.c_str(), &state) != 0)
	return false;

    return S_ISREG(state.st_mode);
}

/*****************************************************************************/
bool Core::isSymbolicLink(const std::string &path)
/*****************************************************************************/
{
    struct stat64 state;
    if (lstat64(path.c_str(), &state) != 0)
	return false;
    return S_ISLNK(state.st_mode);
}

/*****************************************************************************/
bool Core::isReadable(const std::string &path)
/*****************************************************************************/
{
    return (access(path.c_str(), R_OK) == 0);
}

/*****************************************************************************/
bool Core::isWritable(const std::string &path)
/*****************************************************************************/
{
    return (access(path.c_str(), W_OK) == 0);
}

/*****************************************************************************/
bool Core::isExecutable(const std::string &path)
/*****************************************************************************/
{
    return (access(path.c_str(), X_OK) == 0);
}


// ===========================================================================
bool DirectoryFileIterator::AllFilter::operator()(const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const {
    return true;
}

const DirectoryFileIterator::AllFilter DirectoryFileIterator::allFilter;

bool DirectoryFileIterator::DirectoryFilter::operator()(const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const {
    return S_ISDIR(state.st_mode);
}

const DirectoryFileIterator::DirectoryFilter DirectoryFileIterator::directoryFilter;

bool DirectoryFileIterator::FileFilter::operator()(
    const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const {
    return S_ISREG(state.st_mode);
}

const DirectoryFileIterator::FileFilter DirectoryFileIterator::fileFilter;

bool DirectoryFileIterator::FileNameFilter::operator()(
    const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const {

    return  S_ISREG(state.st_mode) &&
	(exactMatch_ ? (baseName(path) == name_) : (baseName(path).find(name_) != std::string::npos));
}

bool DirectoryFileIterator::FileExtensionFilter::operator()(
    const std::string &path, const struct stat64& state, uid_t uid, gid_t gid) const {

    return S_ISREG(state.st_mode) && filenameExtension(path) == name_;
}

bool DirectoryFileIterator::pushPath() {
    std::string path = joinPaths(base_, path_);
    DIR* dir = opendir(path.c_str());
    if (dir != 0) directoryStack_.push(DirectoryId(path_, dir));
    return dir != 0;
}

void DirectoryFileIterator::popPath() {
    verify(!directoryStack_.empty());
    closedir(directoryStack_.top().second);
    directoryStack_.pop();
}

bool DirectoryFileIterator::canEnterPath() {
    return S_ISDIR(state_.st_mode) &&
	((state_.st_uid == uid_ && (state_.st_mode & (S_IXUSR))) ||
	 (state_.st_gid == gid_ && (state_.st_mode & (S_IXGRP))) ||
	 (state_.st_mode & (S_IXOTH)));
}

struct dirent *DirectoryFileIterator::nextEntry() {
    struct dirent *result;
    do {
	result = readdir(directoryStack_.top().second);
	// check wether d_name is one of the default directory names which
	// we should avoid to trace due to recursion
    } while (result != 0 &&
	     (strcmp(result->d_name, ".") == 0 ||
	      strcmp(result->d_name, "..") == 0));
    if (result) {
	path_ = joinPaths(directoryStack_.top().first, result->d_name);
	if (stat64(joinPaths(base_, path_).c_str(), &state_) != 0)
	    perror(joinPaths(base_, path_).c_str());
    }
    return result;
}

DirectoryFileIterator::DirectoryFileIterator(const std::string &path, const Filter *filter) :
    end_(false), filter_(filter) {
    uid_ = getuid();
    gid_ = getgid();
    base_ = path;
    path_ = "";
    end_ = !pushPath();
    operator++();
}

DirectoryFileIterator& DirectoryFileIterator::operator++() {
    if (end_)
	return *this;

    struct dirent *entry;
    while ((entry = nextEntry())) {
	if (canEnterPath()) {
	    if (!pushPath())
		defect();
	}
	if ((*filter_)(joinPaths(base_, path_), state_, uid_, gid_))
	    return *this;
    }
    popPath();
    return (end_ = directoryStack_.empty()) ? *this : operator++();
}
