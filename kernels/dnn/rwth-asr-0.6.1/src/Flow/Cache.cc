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
#include "Registry.hh"
#include "Cache.hh"
#include "Datatype.hh"
#include <Core/Directory.hh>

using namespace Flow;


/*****************************************************************************/
CacheReader::CacheReader(Cache *cache, const std::string &name) :
    Cached(cache, name), reader(*cache->archive_, name)
/*****************************************************************************/
{
    data_.resize(0);
    position_ = 0;
}

/*****************************************************************************/
void CacheReader::readData()
/*****************************************************************************/
{
    Core::BinaryInputStream b(reader);
    std::string datatypeName;
    if (b >> datatypeName) {
	const Datatype *datatype = Flow::Registry::instance().getDatatype(datatypeName);
	if (!datatype || !datatype->readGatheredData(b, data_)) {
	    data_.resize(0);
	}
    } else
	data_.resize(0);
    position_ = 0;
}

/*****************************************************************************/
Data* CacheReader::getData()
/*****************************************************************************/
{
    if (position_ >= data_.size())
	readData();
    if (position_ < data_.size())
	return data_[position_++].get();
    return Data::eos();
}


/*****************************************************************************/
CacheWriter::CacheWriter(Cache *cache, const std::string &name) :
    Cached(cache, name), writer(*cache->archive_, name, cache->compress_),
    datatype_(0)
/*****************************************************************************/
{
}

/*****************************************************************************/
CacheWriter::~CacheWriter()
/*****************************************************************************/
{
    if (attributes_) {
	Core::ArchiveWriter w(*cache_->archive_, name_ + ".attribs", cache_->compress_);
	if (w.isOpen()) {
	    Core::XmlWriter xw(w);
	    xw << *attributes_;
	}
    }

    if (data_.size()) {
	verify(datatype_);
	Core::BinaryOutputStream b(writer);
	b << datatype_->name();
	datatype_->writeGatheredData(b, data_);
	data_.resize(0);
    }
}

/*****************************************************************************/
void CacheWriter::putData(Data *data)
/*****************************************************************************/
{
    if (datatype_ != data->datatype()) {
	if (data_.size() > 0) {
	    verify(datatype_);
	    Core::BinaryOutputStream b(writer);
	    b << datatype_->name();
	    datatype_->writeGatheredData(b, data_);
	    data_.resize(0);
	}
	datatype_ = data->datatype();
    }

    ensure(data->datatype() == datatype_);
    data_.push_back(DataPtr<Data>(data));

    if (data_.size() > cache_->gather_) {
	Core::BinaryOutputStream b(writer);
	b << datatype_->name();
	datatype_->writeGatheredData(b, data_);
	data_.resize(0);
    }
}

// ===========================================================================
// Cache

Core::ParameterString Cache::paramPath
  ("path", "path to cache archive");
Core::ParameterString Cache::paramPrefix
  ("prefix", "prefix for files within the archive");
Core::ParameterInt Cache::paramGather
  ("gather", "number of data packets to gather before writing", Core::Type<u32>::max);
Core::ParameterBool Cache::paramCompress
  ("compress", "compress data written to archive", false);
Core::ParameterString Cache::paramCast
  ("cast", "datatype casted to before writing to archive");

Cache::Cache(const Core::Configuration &c) :
    Core::Component(c),
    archive_(0),
    attributesParser_(select("attributes-parser"))
{
    setPath(paramPath(config));
    setPrefix(paramPrefix(config));
    setGather(paramGather(config));
    setCompress(paramCompress(config));
    setCast(paramCast(config));
}

Cache::~Cache() {
    if (isOpen()) close();
}

/*****************************************************************************/
bool Cache::open(Core::Archive::AccessMode _access)
/*****************************************************************************/
{
    if (isOpen()) close();
    if (path_.empty() ||
	(!Core::isValidPath(path_) &&
	 !(_access & Core::Archive::AccessModeWrite))) return false;
    archive_ = Core::Archive::create(config, path_, _access);
    return isOpen();
}

/*****************************************************************************/
void Cache::close()
/*****************************************************************************/
{
    if (!isOpen()) return;
    delete archive_; archive_ = 0;
}

/*****************************************************************************/
CacheReader* Cache::newReader(const std::string &name)
/*****************************************************************************/
{
    if (isOpen()) {
	CacheReader *reader = new CacheReader(this, name);
	if (reader->isOpen()) return reader;
	delete reader;
    }
    return 0;
}

/*****************************************************************************/
CacheWriter* Cache::newWriter(const std::string &name)
/*****************************************************************************/
{
    if (isOpen()) {
	CacheWriter *writer = new CacheWriter(this, name);
	if (writer->isOpen()) return writer;
	delete writer;
    }
    return 0;
}

// ===========================================================================
// CacheNode


/*****************************************************************************/
CacheNode::CacheNode(const Core::Configuration &c) :
    Core::Component(c),
    Node(c), Cache(c), hasInput_(false), hasOutput_(false),
    isCached_(false), reader_(0), writer_(0), datatype_(0)
/*****************************************************************************/
{}

/*****************************************************************************/
CacheNode::~CacheNode()
/*****************************************************************************/
{
    delete reader_;
    delete writer_;
}

/*****************************************************************************/
bool CacheNode::createContext(const std::string &id)
/*****************************************************************************/
{
    id_ = prefix_ + id;
    delete reader_; reader_ = 0;
    delete writer_; writer_ = 0;

    // Open archive first only for reading. independent if node has an input.
    if (!hasAccess(Core::Archive::AccessModeRead)) {
	if (!open(Core::Archive::AccessModeRead)) {
	    // Report error only of node has no input.
	    if (!hasInput_) {
		Cache::error("Failed to open archive '%s' for reading and "\
			     "node has no input.", path_.c_str());
		return false;
	    }
	}
    }
    isCached_ = false;
    if (hasAccess(Core::Archive::AccessModeRead))
	isCached_ = archive_->hasFile(id_);
    if (isCached_) {
	return true;
    } else if (hasInput_) {
	// Open archive for writing only on demand.
	if (!hasAccess(Core::Archive::AccessModeWrite)) {
	    if (!open(Core::Archive::AccessModeRead | Core::Archive::AccessModeWrite)) {
		Cache::error("Failed to open archive '%s'.", path_.c_str());
		return false;
	    }
	}
	verify(hasAccess(Core::Archive::AccessModeWrite));
	writer_ = newWriter(id_);
	if (!writer_) {
	    Cache::error("Failed to prepare writing to cache for id \"%s\".",
			 id_.c_str());
	}
	return true;
    }
    Cache::error("Cannot provide data for id \"%s\": "
		 "Neither archive entry nor input available.",
		 id_.c_str());
    return false;
}

/*****************************************************************************/
bool CacheNode::setParameter(const std::string &name, const std::string &value)
/*****************************************************************************/
{
    if (name == "id") return createContext(value);
    else if (paramPath.match(name)) setPath(paramPath(value));
    else if (paramPrefix.match(name)) setPrefix(paramPrefix(value));
    else if (paramGather.match(name)) setGather(paramGather(value));
    else if (paramCompress.match(name)) setCompress(paramCompress(value));
    else if (paramCast.match(name)) setCast(paramCast(value));
    else return false;
    return true;
}

/*****************************************************************************/
PortId CacheNode::getInput(const std::string &name)
/*****************************************************************************/
{
    if (!hasInput_) {
	hasInput_ = true;
	addInput(0);
    }
    return 0;
}

/*****************************************************************************/
PortId CacheNode::getOutput(const std::string &name)
/*****************************************************************************/
{
    if (!hasOutput_) {
	hasOutput_ = true;
	addOutput(0);
    }
    return 0;
}

/*****************************************************************************/
bool CacheNode::configure()
/*****************************************************************************/
{
    Core::Ref<const Attributes> attributes;

    if (isCached_) {
	Core::ArchiveReader r(*archive_, id_ + ".attribs");
	if (r.isOpen()) {
	    Core::Ref<Attributes> ca(new Attributes());
	    if (attributesParser_.buildFromStream(*ca, r)) {
		std::string datatype = ca->get("datatype");
		if (!datatype.empty()) {
		    datatype_ = Flow::Registry::instance().getDatatype(datatype);
		    if (datatype_ == 0)
			Cache::error("datatype '%s' from archive is unknown", datatype.c_str());
		}
		attributes = ca;
	    }
	}
    } else if (hasInput_) {
	attributes = getInputAttributes(0);
	std::string datatype = attributes->get("datatype");
	if (!datatype.empty())
	    datatype_ = Flow::Registry::instance().getDatatype(datatype);
    } else
	attributes = Core::ref(new Attributes());

    if (writer_) writer_->putAttributes(attributes);
    if (hasOutput_) return putOutputAttributes(0, attributes);
    return true;
}

/*****************************************************************************/
bool CacheNode::work(PortId output)
/*****************************************************************************/
{
    if (isCached_ && hasOutput_) {
	if (!reader_)
	    reader_ = newReader(id_);
	if (!reader_)
	    return false;
	return putData(0, reader_->getData());
    } else if (hasInput_) {
	DataPtr<Data> in;
	getData(0, in);
	if (writer_ && in) writer_->putData(in.get());
	if (hasOutput_) return putData(0, in.get());
	return true;
    }
    return false;
}
