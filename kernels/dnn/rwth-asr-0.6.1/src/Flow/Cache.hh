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
#ifndef _FLOW_CACHE_HH
#define _FLOW_CACHE_HH

/*
 * flow network cache:
 *
 * we derive from node directly because we must determine which
 * connections have been made:
 *
 * no input, output  => read only
 * input, output     => read/write, pass-through
 *
 */

#include <string>

#include <Core/Archive.hh>
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/Types.hh>

#include "Attributes.hh"
#include "Data.hh"
#include "Node.hh"

namespace Flow {

    class Cache;
    class Cached {
    protected:
	std::string name_;

	Cache *cache_;
	Core::Archive::AccessMode access_;

	// gathered data
	std::vector<DataPtr<Data> > data_;

    public:
	Cached(Cache *cache, const std::string &name) :
	    name_(name), cache_(cache) {}
    };

    class CacheReader : public Cached {
    private:
	size_t position_;
	Core::ArchiveReader reader;

    public:
	CacheReader(Cache *cache, const std::string &name);
	void readData();
	Data* getData();
	bool isOpen() const { return reader.isOpen(); }
    };

    class CacheWriter : public Cached {
    private:
	Core::ArchiveWriter writer;
	const Datatype *datatype_;
	Core::Ref<Attributes> attributes_;

    public:
	CacheWriter(Cache *cache, const std::string &name);
	~CacheWriter();
	void putData(Data *data);
	void putAttributes(Core::Ref<const Attributes> a) {
	    if (!attributes_) attributes_ = Core::ref(new Attributes());
	    if (a) attributes_->merge(*a);
	}
	Core::Ref<Attributes> getAttributes() {
	    Core::Ref<Attributes> a = attributes_;
	    attributes_.reset();
	    return a;
	}
	bool isOpen() const { return writer.isOpen(); }
	void flush();
    };



    class Cache :
	public virtual Core::Component
    {
    private:
	friend class Cached;
	friend class CacheReader;
	friend class CacheWriter;
    protected:
	static Core::ParameterString paramPath;
	static Core::ParameterString paramPrefix;
	static Core::ParameterInt paramGather;
	static Core::ParameterBool paramCompress;
	static Core::ParameterString paramCast;

	Core::Archive *archive_;
	Attributes::Parser attributesParser_;
	std::string path_;
	std::string prefix_;
	u32 gather_;
	bool compress_;
	std::string cast_;
    public:
	Cache(const Core::Configuration&);
	virtual ~Cache();

	const std::string& path() const { return path_; }
	void setPath(const std::string &path) { path_ = path; }
	void setPrefix(const std::string &prefix) { prefix_ = prefix; }
	void setGather(u32 gather) { gather_ = gather; }
	void setCompress(bool compress) { compress_ = compress; }
	void setCast(const std::string &cast) { cast_ = cast; }

	bool hasAccess(Core::Archive::AccessMode a) const {
	    return (archive_) ? archive_->hasAccess(a) : false;
	}
	bool open(Core::Archive::AccessMode access);
	void close();
	bool isOpen() const { return (archive_ != 0); }

	CacheReader* newReader(const std::string &name);
	CacheWriter* newWriter(const std::string &name);
    };



    class CacheNode : public Node, public Cache {
    private:
	bool hasInput_;
	bool hasOutput_;
	std::string id_;
	bool isCached_;
	CacheReader *reader_;
	CacheWriter *writer_;
	const Datatype *datatype_;

    private:
	bool createContext(const std::string &id);

    public:
	static std::string filterName() { return "generic-cache"; }

	CacheNode(const Core::Configuration &c);
	virtual ~CacheNode();

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual PortId getInput(const std::string &name);
	virtual PortId getOutput(const std::string &name);
	virtual bool configure();
	virtual bool work(PortId output);
    };

} // namespace Flow

#endif // _FLOW_CACHE_HH
