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
#include "MappedArchive.hh"

#include <map>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <Core/Debug.hh>

/*
 * This code has to be very careful when changing the mapped files, because there
 * may be other processes which try to write at the same time, or which are
 * using some part of the data through memory-mapping.
 *
 * Therefore we use a copy-on-write mechanism: Whenever we do a change to the file,
 * we create a new modified version of the file, and then move it
 * over the old version. This way each new version gets a new identity (eg. inode number), and thus
 * processed which are reading the old version are not affected.
 *
 * The temporary files are called $filename.temp.$hostname.$pid in the original location. Modern copy-on-write
 * file systems like ZFS will be able to handle the copying and moving very efficiently.
 *
 * The changed files are moved back when the process finishes. The downside of this
 * approach is that crashed processes may leave behind dangling temporary files.
 * */

std::string hostAndProcessId() {
    enum { Len = 1000 };
    char buf[Len];
    gethostname(buf, Len);
    u64 pid = getpid();
    std::ostringstream os;
    os << buf << "." << pid;
    return os.str();
}

namespace Core
{
enum {
    Version = 0x17231
};

MMappedFile::MMappedFile() :
    data_(0),
    desc_(-1),
    size_(0)
{
}

bool MMappedFile::load(std::string file) {
    unload();

    desc_ = open(file.c_str(), O_RDONLY);
    if(desc_ == -1)
	return false;
    struct stat buf;
    fstat(desc_, &buf);
    size_ = buf.st_size;
    data_ = reinterpret_cast<char*>(mmap(0, size_, PROT_READ, MAP_SHARED, desc_, 0));
    if(data_ == MAP_FAILED)
    {
	data_ = 0;
	size_ = 0;
    }
    return (bool)data_;
}

MMappedFile::~MMappedFile() {
    unload();
}

void MMappedFile::unload() {
    if(data_)
	munmap(data_, size_);
    if(desc_ != -1)
	close(desc_);

    data_ = 0;
    desc_ = -1;
    size_ = 0;
}

size_t MMappedFile::size() const {
    return size_;
}

MappedArchive::MappedArchive(std::string archivePath, bool readOnly) :
    mapped_(0),
    archivePath_(archivePath),
    readOffset_(0),
    currentWriter_(0),
    readOnly_(readOnly) {
    loadData();
}

MappedArchive::~MappedArchive() {
    finalize();
    delete mapped_;
}

void MappedArchive::initializeTempFile()
{
    if(readOnly_ || tempFile_.size())
	return;
    tempFile_ =  archivePath_ + ".temp." + hostAndProcessId();
    remove(tempFile_.c_str());
    // Write header
    std::ofstream out(tempFile_.c_str());
    u32 ver = Version;
    out.write((const char*)&ver, sizeof(u32));
    if(out.good())
	Core::Application::us()->log() << tempFile_ << ": Initialized empty archive";
    else {
	Core::Application::us()->log() << tempFile_ << ": Initializing archive failed";
	tempFile_.clear();
    }
}

MappedArchiveWriter MappedArchive::getWriter(std::string name)
{
    initializeTempFile();

    if(tempFile_.empty())
	return MappedArchiveWriter();

    verify(!currentWriter_);
    currentWriter_ = new Writer(name, new std::ofstream(tempFile_.c_str(), std::ofstream::app), this);

    // Skip the header, which we will write later
    currentWriter_->itemOffset_ = currentWriter_->out_->tellp();
    if(!currentWriter_->out_->good() || currentWriter_->itemOffset_ == -1)
    {
	delete currentWriter_;
	currentWriter_ = 0;
	Core::Application::us()->log() << tempFile_ << ": creating writer failed " << name;
	return MappedArchiveWriter();
    }else{
	Core::Application::us()->log() << tempFile_ << ": created writer " << name;
    }

    verify(currentWriter_->itemOffset_ != 0); //  Must be at least behind the header

    // Add some zero-filled header space, which will be filled in releaseWriter
    std::vector<char> dummy(sizeof(u32) + sizeof(u64) + name.length(), 0);
    currentWriter_->out_->write(dummy.data(), dummy.size());

    currentWriter_->dataOffset_ = currentWriter_->out_->tellp();

    wroteTempFileItems_.insert(name);

    return MappedArchiveWriter(Core::Ref<Writer>(currentWriter_));
}

void MappedArchive::releaseWriter(MappedArchive::Writer* writer) {
    verify(currentWriter_ == writer);

    bool failed = !writer->out_->good();

    // Write the ItemInfo header
    size_t writePos = writer->out_->tellp();

    delete writer->out_;
    currentWriter_ = 0;

    if(failed)
    {
	Core::Application::us()->log() << tempFile_ << ": Writing temp-file failed, making archive read-only";
	readOnly_ = true;
	tempFile_.clear();
	wroteTempFileItems_.clear();
	return;
    }

    // We have to use fopen instead of ofstream to update the header, because
    // only its "r+" mode allows selectively overwriting some bytes of a file.
    FILE* f = fopen(tempFile_.c_str(), "r+");
    fseek(f, writer->itemOffset_, SEEK_SET);
    verify(writer->dataOffset_ == writer->itemOffset_ + sizeof(u32) + sizeof(u64) + writer->name_.length());

    u32 nameLength = writer->name_.length();
    u64 dataSize = writePos - writer->dataOffset_;

    fwrite((char*)&nameLength, sizeof(u32), 1, f);
    fwrite((char*)&dataSize, sizeof(u64), 1, f);

    verify(ftell(f) == writer->itemOffset_ + sizeof(u32) + sizeof(u64));

    fwrite(writer->name_.data(), sizeof(char), writer->name_.length(), f);

    verify(ftell(f) == writer->itemOffset_ + sizeof(u32) + sizeof(u64) + nameLength);
    verify(writer->dataOffset_ == writer->itemOffset_ + sizeof(u32) + sizeof(u64) + nameLength);

    verify((size_t)ftell(f) == writer->dataOffset_);
    fclose(f);

    Core::Application::us()->log() << tempFile_ << ": wrote " << writer->name_ << ", data size: " << dataSize;

/* This code tries to directly map the written data, to eventually save some memory. However this doesn't work.
 * if(mapped_->update(writer->dataOffset_ + info.dataSize))
    {
	MappedItem item;
	item.name = writer->name_;
	item.size = info.dataSize;
	item.data = mapped_->data<char>(writer->dataOffset_);
	items_.push_back(item);
    }else{
	std::cout << "extending the memory-map failed" << std::endl;
    }*/
}

MappedArchiveReader MappedArchive::getReader(std::string name) {
    MappedItem item = getItem(name);
    if(!item.data)
    {
	Core::Application::us()->log() << archivePath_ << ": found no item named " << name;
	return MappedArchiveReader();
    }
    return MappedArchiveReader(Core::Ref<Reader>(new Reader(name, item.data, 0, item.size)));
}

void MappedArchive::finalize() {
    if(wroteTempFileItems_.empty())
	return;

    // Unfortunately we cannot use Core::Application::us()->log() here, because the system may already be shutting down

    std::map<std::string, MappedItem> addItems;
    bool failed = false;
    for(std::vector<MappedItem>::reverse_iterator it = items_.rbegin(); it != items_.rend(); ++it)
    {
	if(wroteTempFileItems_.count(it->name) == 0)
	{
	    wroteTempFileItems_.insert(it->name);
	    std::cout << tempFile_ << ": Keeping " << it->name << " of size " << it->size << std::endl;
	    std::ofstream of(tempFile_.c_str(), std::ofstream::app);
	    u32 nameLength = it->name.length();
	    u64 dataSize = it->size;
	    of.write((char*)&nameLength, sizeof(u32));
	    of.write((char*)&dataSize, sizeof(u64));
	    of.write(it->name.data(), it->name.length());
	    of.write(it->data, it->size);
	    if(!of.good())
	    {
		failed = true;
		std::cout << tempFile_.data() << ": Writing failed, discarding written data" << std::endl;
		break;
	    }
	}
    }

    if(!failed && rename(tempFile_.c_str(), archivePath_.c_str()) == 0) {
	std::cout << tempFile_.data() << ": Renamed to " << archivePath_.data() << std::endl;
    } else {
	if(!failed)
	    std::cout << tempFile_.data() << ": Failed renaming to " << archivePath_.data() << std::endl;;
	remove(tempFile_.c_str());
    }
}

bool MappedArchive::loadData() {
    items_.clear();
    bool loaded = loadFile();
    if(loaded && get<u32>() == Version)
    {
	Core::Application::us()->log() << archivePath_ << ": Loading";

	while(readOffset_ < mapped_->size())
	{
	    if(!available(sizeof(u32) + sizeof(u64)))
		return false;

	    u32 nameLength = get<u32>();
	    u64 dataSize = get<u64>();

	    if(nameLength == 0 || !available(nameLength + dataSize))
		return false;

	    std::string name;
	    for(u32 i = 0; i < nameLength; ++i)
		name.push_back(get<char>());

	    MappedItem i;
	    i.name = name;
	    i.data = mapped_->data<char>(readOffset_);
	    i.size = dataSize;

	    skip(dataSize);

	    Core::Application::us()->log() << archivePath_ << ": Item " << i.name << " size " << i.size;

	    items_.push_back(i);
	}

	return true;
    }

    return false;
}

bool MappedArchive::loadFile() {
    readOffset_ = 0;
    delete mapped_;
    mapped_ = new MMappedFile;
    return mapped_->load(archivePath_);
}

MappedArchive::MappedItem MappedArchive::getItem(std::string name) const {
    for(std::vector<MappedItem>::const_reverse_iterator it = items_.rbegin(); it != items_.rend(); ++it)
	if(it->name == name)
	    return *it;
    return MappedItem();
}

MappedArchive::Reader::Reader(std::string name, const char* data, size_t offset, size_t size) :
    name_(name),
    data_(data),
    initialOffset_(offset),
    offset_(offset),
    size_(size)
{
}

MappedArchive::Writer::~Writer() {
    archive_->releaseWriter(this);
}

MappedArchive::Writer::Writer(std::string name, std::ofstream* out, MappedArchive* archive) :
    name_(name),
    out_(out),
    archive_(archive)
{
}
}
