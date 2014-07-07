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
#ifndef _MM_MIXTURE_SET_READER_HH
#define _MM_MIXTURE_SET_READER_HH

#include <Core/Component.hh>
#include <Core/Directory.hh>
#include <Core/Factory.hh>
#include "MixtureSet.hh"

namespace Mm
{
    class LogLinearMixtureSet;
    class AbstractMixtureSetEstimator;

    /**
     * Read a mixture set from various file formats
     */
    class MixtureSetReader : public Core::Component
    {
	typedef Core::Ref<AbstractMixtureSet> MixtureSetRef;
    public:
	MixtureSetReader(const Core::Configuration &c);

	/**
	 * read mixture set from file @c filename
	 * and try to cast it into type @c MixtureType
	 */
	template<class MixtureType>
	Core::Ref<MixtureType> read(const std::string &filename) const;

    public:
	/**
	 * Interface for read classes
	 */
	class Reader : public Core::Component
	{
	public:
	    Reader(const Core::Configuration &c)
		: Core::Component(c) {}
	    virtual ~Reader() {}
	    virtual bool read(const std::string &filename, MixtureSetRef &result) const = 0;
	    template<class T>
	    static Reader* create(const Core::Configuration &c) {
		return new T(c);
	    }
	};
	/**
	 * mixture set in legacy format
	 */
	class RefReader : public Reader
	{
	private:
	    static const Core::ParameterBool paramRepeatedCovariance;
	public:
	    RefReader(const Core::Configuration &c)
		: Reader(c) {}
	    virtual ~RefReader() {}
	    virtual bool read(const std::string &filename, MixtureSetRef &result) const;
	};
	/**
	 * mixture set readable using stream operators
	 */
	class FormatReader : public Reader
	{
	public:
	    FormatReader(const Core::Configuration &c)
		: Reader(c) {}
	    virtual ~FormatReader() {}
	    virtual bool read(const std::string &filename, MixtureSetRef &result) const;
	};
	/**
	 * cluster model file reader
	 */
	class CmfReader : public Reader
	{
	public:
	    CmfReader(const Core::Configuration &c)
		: Reader(c) {}
	    virtual ~CmfReader() {}
	    virtual bool read(const std::string &filename, MixtureSetRef &result) const;
	};
	/**
	 * log linear mixture set reader
	 */
	class LambdaReader : public Reader
	{
	private:
	    static const Core::ParameterString paramTopologyFilename;
	public:
	    LambdaReader(const Core::Configuration &c)
		: Reader(c) {}
	    virtual ~LambdaReader() {}
	    virtual bool read(const std::string &filename, MixtureSetRef &result) const;
	    Core::Ref<LogLinearMixtureSet> readLogLinearMixtureSet(const std::string &filename) const;
	};
	/**
	 * load mixture set estimator and estimate mixture set
	 */
	class MixtureSetEstimatorReader : public Reader
	{
	public:
	    MixtureSetEstimatorReader(const Core::Configuration &c)
		: Reader(c) {}
	    virtual ~MixtureSetEstimatorReader() {}
	    virtual bool read(const std::string &filename, MixtureSetRef &result) const;
	    bool readMixtureSetEstimator(const std::string &filename, AbstractMixtureSetEstimator &estimator) const;
	};

    private:
	static Core::ComponentFactory<Reader, const std::string> *reader_;
	/**
	 * register reader class using its name and the associated
	 * filename extension
	 */
	template<class T>
	void registerReader(const std::string &id) {
	    reader_->registerClass(id, Reader::create<T>);
	}
    };

    template<class MixtureType>
    Core::Ref<MixtureType> MixtureSetReader::read(const std::string &filename) const {
	MixtureSetRef result;
	Reader* reader = reader_->getObject(Core::filenameExtension(filename), config);
	if (!reader) {
	    // create default reader
	    reader = new MixtureSetEstimatorReader(config);
	}
	if (!reader->read(filename, result))
	    error("Cannot read mixture set from file '%s'", filename.c_str());
	delete reader;
	return Core::Ref<MixtureType>( dynamic_cast<MixtureType*>(result.get()) );
    }

    template<>
    Core::Ref<LogLinearMixtureSet> MixtureSetReader::read(const std::string &filename) const;
} // namespace Mm

#endif // _MM_MIXTURE_SET_READER_HH
