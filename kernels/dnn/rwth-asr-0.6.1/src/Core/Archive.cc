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
#include <iomanip>
#include <iostream>
#include <zlib.h>

#include "Application.hh"
#include "Archive.hh"
#include "Assertions.hh"
#include "DirectoryArchive.hh"
#include "FileArchive.hh"
#include "BundleArchive.hh"

using namespace Core;

/*****************************************************************************/
Archive::Archive(const Core::Configuration &config, const std::string &path, AccessMode access)
    : Component(config), path_(path), access_(access)
/*****************************************************************************/
{
}

/*****************************************************************************/
bool Archive::hasFile(const std::string &name) const {
    Sizes sizes;
    lock();
    bool result = discover(name, sizes);
    release();
    return result;
}

/*****************************************************************************/
bool Archive::removeFile(const std::string &name) {
    if (!hasFile(name))
	return false;
    lock();
    bool result = remove(name);
    release();
    return result;
}

bool Archive::readFile(const std::string &name, std::string &b)
/*****************************************************************************/
{
    lock();

    Sizes sizes;
    if (!discover(name, sizes)) {
	release();
	return false;
    }

    /* from zlib.h:
     *
     * ZEXTERN int ZEXPORT uncompress OF((Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen));
     *
     * Decompresses the source buffer into the destination buffer. sourceLen
     * is the byte length of the source buffer. Upon entry, destLen is the
     * total size of the destination buffer, which must be large enough to
     * hold the entire uncompressed data. (The size of the uncompressed data
     * must have been saved previously by the compressor and transmitted to
     * the decompressor by some mechanism outside the scope of this
     * compression library.) Upon exit, destLen is the actual size of the
     * compressed buffer.
     * This function can be used to decompress a whole file at once if the
     * input file is mmap'ed.
     *
     * uncompress returns Z_OK if success, Z_MEM_ERROR if there was not
     * enough memory, Z_BUF_ERROR if there was not enough room in the output
     * buffer, or Z_DATA_ERROR if the input data was corrupted.
     */

    bool status = false;
    if (sizes.compressed() > 0) {
	std::string tmp;
	tmp.resize(sizes.compressed());
	if (read(name, tmp)) {
	    // We might want to check the CRC32 here.
	    b.resize(sizes.uncompressed());
	    uLongf tmplen = b.size();
	    uLongf compressedSize = tmp.size() - 10 + 2;

	    // check for extra gzip header data and skip it (zlib does not detect it)
	    u32 base = 10;
	    if (tmp[3] & 0x04) base += (int(tmp[base]) + int(tmp[base + 1])) << 8; // extra field
	    if (tmp[3] & 0x08) {
		for (; (base < compressedSize) && (tmp[base]); base++); // filename
		base++;
	    }
	    if (tmp[3] & 0x10) {
		for (; (base < compressedSize) && (tmp[base]); base++); // comment
		base++;
	    }
	    if (tmp[3] & 0x02) base += 2; // crc16

	    // restore bogus zlib header
	    base -= 2;
	    tmp[base] = 0x78; tmp[base + 1] = 0x9c;
	    switch (uncompress((Bytef*)&b[0], &tmplen, (Bytef*)&tmp[base], compressedSize)) {
	    case Z_MEM_ERROR:
		std::cerr << "no memory to decompress." << std::endl;
		break;
	    case Z_BUF_ERROR:
		std::cerr << "unpack buffer was too small (" << sizes.compressed() << ", "
			  << sizes.uncompressed() << ", " << tmplen << ")." << std::endl;
		break;
	    case Z_DATA_ERROR:
		/* CAUTION! Zlib thinks that data was corrupted because we replaced
		 * the adler32 checksum by a gzip compatible crc32. So we ignore the error.
		 */
		// fall through
	    case Z_OK:
	    default:
		b.resize(tmplen);
		status = true;
		break;
	    }
	}
    } else {
	b.resize(sizes.uncompressed());
	status = read(name, b);
    }

    release();
    return status;
}

/*****************************************************************************/
bool Archive::writeFile(const std::string &name, const std::string &b, bool compress)
/*****************************************************************************/
{
    lock();

    /* from zlib.h:
     *
     * ZEXTERN int ZEXPORT compress OF((Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen));
     *
     * Compresses the source buffer into the destination buffer. sourceLen is
     * the byte length of the source buffer. Upon entry, destLen is the total
     * size of the destination buffer, which must be at least 0.1% larger
     * than sourceLen plus 12 bytes. Upon exit, destLen is the actual size
     * of the compressed buffer.
     * This function can be used to compress a whole file at once if the
     * input file is mmap'ed.
     *
     * compress returns Z_OK if success, Z_MEM_ERROR if there was not
     * enough memory, Z_BUF_ERROR if there was not enough room in the output
     * buffer.
     */

    // compress buffer
    // we could do the compression in a separate thread, but we don't...
    std::string compressed;
    if (compress) {
	size_t header_length = 10;
	compressed.resize((unsigned int)(header_length + (b.size() + 12) * 1.02));
	uLongf compressedSize = compressed.size();

	/*
	 * CAUTION! This is a hack which assumes that the internal
	 * header added by zlib is 2 bytes long which will be
	 * overwritten by a gzip compatible header info later.  We
	 * assume that this zlib header is always 0x78 0x9c.  When
	 * unpacking we drop the gzip header and replace 0x78 0x9c
	 * instead.  Also zlib seems to add an additional 6 bytes of
	 * checksum data at the end which we simply discard after
	 * compression. These assumptions might be wrong, especially
	 * for future versions of zlib.
	 */
	int zstatus = ::compress2
	    ((Bytef*)&compressed[header_length - 2], &compressedSize,
	     (const Bytef*)&(b.c_str()[0]), b.size(), Z_DEFAULT_COMPRESSION);
	hope(compressed[header_length-2] == 0x78 && compressed[header_length-1] == 0x9c);
	compressed.resize(header_length + compressedSize - 6);
	if (zstatus != Z_OK) {
	    std::cerr << "pack buffer was too small (" << b.size() << ", " << compressedSize
		      << ". falling back to no compression." << std::endl;
	    compressed.resize(0);
	} else {
	    // gzip header
	    compressed[0] = 0x1f; // gzip header bytes
	    compressed[1] = 0x8b;
	    compressed[2] = 0x08; // compression format (0x08 = deflate)
	    compressed[3] = 0;    // flags (no flags set)
	    compressed[4] = 0;    // modification time: 4 bytes (0 = no timestamp available)
	    compressed[5] = 0;
	    compressed[6] = 0;
	    compressed[7] = 0;
	    compressed[8] = 0;    // extra flags (0 here, somewhat curious)
	    compressed[9] = 0x03; // operating system (3 = unix)

	    // crc and size
	    u32 crc = crc32(0L, (const Byte*)&(b.c_str()[0]), b.size());
	    compressed.push_back(crc & 0xff);
	    compressed.push_back((crc >> 8) & 0xff);
	    compressed.push_back((crc >> 16) & 0xff);
	    compressed.push_back((crc >> 24) & 0xff);
	    u32 size = b.size();
	    compressed.push_back(size & 0xff);
	    compressed.push_back((size >> 8) & 0xff);
	    compressed.push_back((size >> 16) & 0xff);
	    compressed.push_back((size >> 24) & 0xff);
	}
    }

    // add file data to archive
    bool status = false;
    if (compressed.size()) status = write(name, compressed, Sizes(b.size(), compressed.size()));
    else status = write(name, b, Sizes(b.size(), 0));

    release();
    return status;
}

/*****************************************************************************/
bool Archive::copyFile(const Archive &srcArchive, const std::string &name, const std::string &prefix)
/*****************************************************************************/
{
    Sizes sizes;
    srcArchive.lock();
    if (!srcArchive.discover(name, sizes)) {
	srcArchive.release();
	return false;
    }
    std::string tmp;
    u32 bufferSize = (sizes.compressed() ? sizes.compressed() : sizes.uncompressed());
    std::string buffer;
    buffer.resize(bufferSize);
    bool r = srcArchive.read(name, buffer);
    srcArchive.release();
    if (!r) {
	return false;
    }
    lock();
    r = write(prefix + name, buffer, sizes);
    release();
    return r;
}

/*****************************************************************************/
std::ostream& Core::operator<< (std::ostream &s, const Archive &a)
/*****************************************************************************/
{
    u32 width = 52, total = 0;
    u32 compressed = 0, size = 0;
    std::ios::fmtflags old = s.flags();
    static const int prec = 12;

    //for (Archive::const_iterator i = a.files(); i; ++i)
    //if (i.name().length() > width) width = i.name().length();
    width += 2 * prec + 3;

    s << std::setiosflags(std::ios::left) << std::setw(width - 2 * prec - 3) << "name";
    s.flags(old);
    s << std::setiosflags(std::ios::right) << std::setw(prec + 1) << "compressed";
    s << std::setiosflags(std::ios::right) << std::setw(prec + 1) << "size" << std::endl;
    s << std::setw(width - 1) << std::setfill('-') << "-" << std::setfill(' ') << std::endl;
    for (Archive::const_iterator i = a.files(); i; ++i) {
	s.flags(old);
	s << std::setiosflags(std::ios::left) << std::setw(width - 2 * prec - 3) << i.name().c_str();
	if (i.sizes().compressed()) s << ' ' << std::setiosflags(std::ios::right) << std::setw(prec) << i.sizes().compressed();
	else s << " " << std::setw(prec) << " ";
	s << ' ' << std::setiosflags(std::ios::right) << std::setw(prec) << i.sizes().uncompressed() << std::endl;
	compressed += i.sizes().compressed();
	size += i.sizes().uncompressed();
	total++;
    }
    s << std::setw(width - 1) << std::setfill('-') << "-" << std::setfill(' ') << std::endl;
    s.flags(old);
    s << "total: " << std::setiosflags(std::ios::left) << std::setw(width - 2 * prec - 10) << total;
    if (compressed) s << ' ' << std::setiosflags(std::ios::right) << std::setw(prec) << compressed;
    else s << " " << std::setw(prec) << " ";
    s << ' ' << std::setiosflags(std::ios::right) << std::setw(prec) << size << std::endl;
    s.flags(old);
    return s;
}

/*****************************************************************************/
Archive::Type Archive::test(const std::string &path)
/*****************************************************************************/
{
    if (path == "-") return TypeUnknown;
    if (BundleArchive::test(path)) return TypeBundle;
    if (DirectoryArchive::test(path)) return TypeDirectory;
    if (FileArchive::test(path)) return TypeFile;
    return TypeUnknown;
}

/*****************************************************************************/
Archive* Archive::create(const Configuration &config, const std::string &path, AccessMode access)
/*****************************************************************************/
{
    Archive *result = 0;
    switch (test(path)) {
    case TypeDirectory:
	result = new DirectoryArchive(config, path, access);
	break;
    case TypeFile:
	result = new FileArchive(config, path, access);
	break;
    case TypeBundle:
	result = new BundleArchive(config, path, access);
	break;
    case TypeUnknown:
	Application::us()->error("unknown type of archive (requested path: '%s').", path.c_str());
	break;
    default:
	defect();
    }
    if (result && result->hasFatalErrors()) {
	delete result; result = 0;
    }
    return result;
}

/*****************************************************************************/
ArchiveReader::ArchiveReader(Archive &a, const std::string &name)
/*****************************************************************************/
{
    std::string buffer;
    isOpen_ = a.readFile(name, buffer);
    str(buffer);
}

namespace {
    class ArchiveWriterBuffer : public std::streambuf {
    private:
	std::string &buffer_;
    public:
	ArchiveWriterBuffer(std::string &buffer) : buffer_(buffer) {}
	int overflow(int c) {
	    if (c != EOF) buffer_.push_back(c);
	    return c;
	}
	std::streamsize xsputn(const char *s, std::streamsize num) {
	    buffer_.insert(buffer_.end(), s, s + num);
	    return num;
	}
    };
}

/*****************************************************************************/
ArchiveWriter::ArchiveWriter(Archive &archive, const std::string &path, bool compress)
    : std::ostream(new  ArchiveWriterBuffer(buffer_)), archive_(archive), path_(path), compress_(compress)
/*****************************************************************************/
{
    //std::ostream(new ArchiveWriterBuffer(buffer_));
    isOpen_ = true;
    //    delete rdbuf(new ArchiveWriterBuffer(buffer_));

}

/*****************************************************************************/
ArchiveWriter::~ArchiveWriter()
/*****************************************************************************/
{
    if (isOpen()) archive_.writeFile(path_, buffer_, compress_);
    delete rdbuf(0);
}
