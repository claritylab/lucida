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
#ifndef _FLF_ARCHIVE_HH
#define _FLF_ARCHIVE_HH

#include <Bliss/Lexicon.hh>
#include <Core/Archive.hh>
#include <Core/Component.hh>
#include <Core/Parameter.hh>

#include "Io.hh"

namespace Flf {

    /**
     * Archives
     **/
    class Archive : public Core::Component, public IoFormat {
    public:
	static const Core::ParameterString paramPath;
	static const Core::ParameterString paramFile;
	static const Core::ParameterString paramSuffix;
	typedef Core::Archive::const_iterator const_iterator;
    protected:
	Core::Archive *archive;
	bool isClosed_;
    private:
	std::string pathname_;
	mutable std::string suffix_;
    protected:
	virtual std::string defaultSuffix() const = 0;
	virtual void finalize() {}
    public:
	Archive(const Core::Configuration&,
		const std::string &pathname);
	virtual ~Archive();
	void close();
	bool isClosed() const { return isClosed_; }
	const std::string & path() const;
	const std::string & suffix() const;
	virtual bool hasFile(const std::string &file) const;
	virtual const_iterator files() const;
    };



    /**
     *
     * Lattice Archives
     *
     **/
    class LatticeArchiveReader;
    class LatticeArchiveWriter;
    class LatticeArchive : public Archive {
    public:
	LatticeArchive(const Core::Configuration &config,
		       const std::string &pathname) :
	    Archive(config, pathname) {}
    public:
	static LatticeArchiveReader *getReader(
	    const Core::Configuration&);
	static LatticeArchiveWriter *getWriter(
	    const Core::Configuration&);
    };

    class LatticeArchiveReader : public LatticeArchive {
	typedef LatticeArchive Precursor;
    public:
	static const Core::ParameterString paramSemiring;
	static const Core::ParameterString paramKeys;
	static const Core::ParameterFloat paramScale;
    private:
	ConstSemiringRef semiring_;
	Core::Configuration latticeConfig_;
    protected:
	const Core::Configuration & latticeConfig() const
	    { return latticeConfig_; }
	ConstSemiringRef semiring() const
	    { return semiring_; }
    public:
	LatticeArchiveReader(
	    const Core::Configuration&,
	    const std::string &pathname);
	virtual ~LatticeArchiveReader();
	virtual ConstLatticeRef get(const std::string &id) = 0;
    };

    class LatticeArchiveWriter : public LatticeArchive {
	typedef LatticeArchive Precursor;
    public:
	LatticeArchiveWriter(
	    const Core::Configuration&,
	    const std::string &pathname);
	virtual ~LatticeArchiveWriter();
	virtual void store(const std::string &id, ConstLatticeRef) = 0;
    };

    NodeRef createLatticeArchiveReaderNode(const std::string &name, const Core::Configuration &config);
    NodeRef createLatticeArchiveWriterNode(const std::string &name, const Core::Configuration &config);


    /**
     * deprecated
     **/
    class LatticeProcessorArchiveWriter : public LatticeArchiveWriter {
	typedef LatticeArchiveWriter Precursor;
    public:
	static const Core::ParameterFloat paramPronunciationScale;
    private:
	Score pronunciationScale_;
    protected:
	virtual std::string defaultSuffix() const { return ""; }
    public:
	LatticeProcessorArchiveWriter(
	    const Core::Configuration &config,
	    const std::string &pathname);
	~LatticeProcessorArchiveWriter();
	virtual void store(const std::string &id, ConstLatticeRef f);
    };


    /**
     *
     * Confusion Network Archives
     *
     **/
    class ConfusionNetworkArchiveReader;
    class ConfusionNetworkArchiveWriter;
    class ConfusionNetworkArchive : public Archive {
	typedef Archive Precursor;
    protected:
	CnFormat format;
	std::string encoding;
    public:
	ConfusionNetworkArchive(const Core::Configuration &config,
				const std::string &pathname,
				CnFormat format);
	virtual std::string defaultSuffix() const;
    public:
	static ConfusionNetworkArchiveReader * getReader(const Core::Configuration&, CnFormat = CnFormatXml);
	static ConfusionNetworkArchiveWriter * getWriter(const Core::Configuration&, CnFormat = CnFormatXml);
    };

    class ConfusionNetworkArchiveReader : public ConfusionNetworkArchive {
	typedef ConfusionNetworkArchive Precursor;
    private:
	bool getHeader_;
	ConstConfusionNetworkRef header_;
    private:
	ConstConfusionNetworkRef getHeader();
    public:
	ConfusionNetworkArchiveReader(
	    const Core::Configuration&,
	    const std::string &pathname,
	    CnFormat format);
	virtual ~ConfusionNetworkArchiveReader();
	virtual ConstConfusionNetworkRef get(const std::string &id);
    };

    class ConfusionNetworkArchiveWriter : public ConfusionNetworkArchive {
	typedef ConfusionNetworkArchive Precursor;
    protected:
	s32 margin;
	s32 indentation;
    private:
	bool storeHeader_;
    private:
	void storeHeader(ConstConfusionNetworkRef cn);
    public:
	ConfusionNetworkArchiveWriter(
	    const Core::Configuration&,
	    const std::string &pathname,
	    CnFormat format);
	virtual ~ConfusionNetworkArchiveWriter();
	virtual void store(const std::string &id, ConstConfusionNetworkRef cn, ConstSegmentRef segment);
    };

    NodeRef createConfusionNetworkArchiveReaderNode(const std::string &name, const Core::Configuration &config);
    NodeRef createConfusionNetworkArchiveWriterNode(const std::string &name, const Core::Configuration &config);



    /**
     *
     * Posterior CN Archives
     *
     **/
    class PosteriorCnArchiveReader;
    class PosteriorCnArchiveWriter;
    class PosteriorCnArchive : public Archive {
	typedef Archive Precursor;
    protected:
	PosteriorCnFormat format;
	std::string encoding;
    public:
	PosteriorCnArchive(const Core::Configuration &config,
			   const std::string &pathname,
			   PosteriorCnFormat format);
	virtual std::string defaultSuffix() const;
    public:
	static PosteriorCnArchiveReader * getReader(const Core::Configuration&, PosteriorCnFormat = PosteriorCnFormatXml);
	static PosteriorCnArchiveWriter * getWriter(const Core::Configuration&, PosteriorCnFormat = PosteriorCnFormatXml);
    };

    class PosteriorCnArchiveReader : public PosteriorCnArchive {
	typedef PosteriorCnArchive Precursor;
    private:
	bool getHeader_;
	ConstPosteriorCnRef header_;
    private:
	ConstPosteriorCnRef getHeader();
    public:
	PosteriorCnArchiveReader(
	    const Core::Configuration&,
	    const std::string &pathname,
	    PosteriorCnFormat format);
	virtual ~PosteriorCnArchiveReader();
	virtual ConstPosteriorCnRef get(const std::string &id);
    };

    class PosteriorCnArchiveWriter : public PosteriorCnArchive {
	typedef PosteriorCnArchive Precursor;
    protected:
	s32 margin;
	s32 indentation;
    private:
	bool storeHeader_;
    private:
	void storeHeader(ConstPosteriorCnRef cn);
    public:
	PosteriorCnArchiveWriter(
	    const Core::Configuration&,
	    const std::string &pathname,
	    PosteriorCnFormat format);
	virtual ~PosteriorCnArchiveWriter();
	virtual void store(const std::string &id, ConstPosteriorCnRef cn, ConstSegmentRef segment);
    };

    NodeRef createPosteriorCnArchiveReaderNode(const std::string &name, const Core::Configuration &config);
    NodeRef createPosteriorCnArchiveWriterNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_ARCHIVE_HH
