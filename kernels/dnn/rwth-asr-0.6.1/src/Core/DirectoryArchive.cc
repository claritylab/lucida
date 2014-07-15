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
#include "DirectoryArchive.hh"

#include <fstream>
#include <unistd.h>

using namespace Core;

/*****************************************************************************/
DirectoryArchive::DirectoryArchive(const Core::Configuration &config, const std::string &path, AccessMode access)
    : Archive(config, path, access)
/*****************************************************************************/
{
    if (access & AccessModeWrite) {
	if (!createDirectory(path))
	    error("Failed to create directory \"%s\"", path.c_str());
    } else if (access & AccessModeRead) {
	if (!isDirectory(path))
	    error("Directory \"%s\" does not exist", path.c_str());
    }
    // verify permissions on directory
    if ((!(isReadable(path) && isExecutable(path))) ||
	((access & AccessModeWrite) && !isWritable(path))) {
	error("Cannot access directory \"%s\": permission denied", path.c_str());
    }

}

/* Scanning of the archive directory is disabled.
 *
 * On large archives a complete scan takes very long and causes a LOT
 * of disk trashing.  (Example: WSJ training corpus feature cache:
 * 4.3G in 37372 files; scanning takes about 40 minutes.)
 *
 * If this beahviour is needed, we should add an index file to the
 * archive.  For the time being we rely on the "dynamic dicovery"
 * mechanism.
 */

/*****************************************************************************/
bool DirectoryArchive::probe(const std::string &file, const struct stat64 &state, Sizes &sizes) const
/*****************************************************************************/
{
    FILE *fp = fopen(joinPaths(path(), file).c_str(), "r");
    if (!fp) return false;

    u8 tmp[4];
    sizes.setCompressed(0);
    sizes.setUncompressed(0);
    if (fread(tmp, 1, 2, fp) == 2) {
	if ((tmp[0] == 0x1f) && (tmp[1] == 0x8b)) {
	    // assume gzip compressed file
	    fseek(fp, -4, SEEK_END);
	    if (fread(tmp, 1, 4, fp) == 4) {
		sizes.setCompressed(state.st_size);
		sizes.setUncompressed(tmp[0] | (u32(tmp[1]) << 8) | (u32(tmp[2]) << 16) | (u32(tmp[3]) << 24));
	    }
	} else sizes.setUncompressed(state.st_size);
    }
    fclose(fp);
    return true;
}

/*****************************************************************************/
bool DirectoryArchive::discover(const std::string &file, Sizes &sizes) const
/*****************************************************************************/
{
    struct stat64 state;
    if (stat64(joinPaths(path(), file).c_str(), &state) != 0) return false;
    if (!S_ISREG(state.st_mode)) return false;
    return probe(file, state, sizes);
}

/*****************************************************************************/
Archive::const_iterator DirectoryArchive::files() const
/*****************************************************************************/
{
    return const_iterator(new _const_iterator(*this));
}

/*****************************************************************************/
bool DirectoryArchive::read(const std::string &name, std::string &buffer) const
/*****************************************************************************/
{
    require(hasAccess(AccessModeRead));
    if (name.empty()) return false;
    Sizes sizes;
    if (!discover(name, sizes)) return false;
    std::ifstream s(joinPaths(path(), name).c_str());
    if (sizes.compressed())
	return s.read(&buffer[0], sizes.compressed());
    else
	return s.read(&buffer[0], sizes.uncompressed());
}

/*****************************************************************************/
bool DirectoryArchive::write(const std::string &name, const std::string &buffer, const Sizes &sizes)
/*****************************************************************************/
{
    require(hasAccess(AccessModeWrite));
    if (name.empty()) return false;
    std::string file = joinPaths(path(), name);
    std::string dir = directoryName(file);
    if (!isDirectory(dir) && !createDirectory(dir))
	error("Failed to create directory \"%s\".", dir.c_str());
    std::ofstream s(file.c_str());
    if (!s)
	error("Failed to open file \"%s\" for writing.", file.c_str());
    return s.write((const char*)&(buffer.c_str())[0], buffer.size());
}

/*****************************************************************************/
bool DirectoryArchive::test(const std::string &path)
/*****************************************************************************/
{
    struct stat64 fs;
    if (stat64(path.c_str(), &fs) < 0) {
	if (path[path.size() - 1] == '/') return true;
    } else if (S_ISDIR(fs.st_mode)) return true;
    return false;
}

/*****************************************************************************/
bool Core::DirectoryArchive::recover()
/*****************************************************************************/
{
    log("recovery is not implemented for directory archives");
    return true;
}

/*****************************************************************************/
bool Core::DirectoryArchive::clear()
/*****************************************************************************/
{
    return clearDirectory(path());
}

/*****************************************************************************/
bool Core::DirectoryArchive::remove(const std::string &name)
/*****************************************************************************/
{
    require(hasAccess(AccessModeWrite));
    if (name.empty()) return false;
    std::string file = joinPaths(path(), name);
    return (unlink(file.c_str()) == 0);
}
