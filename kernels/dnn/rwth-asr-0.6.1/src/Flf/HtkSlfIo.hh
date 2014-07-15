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
#ifndef _FLF_HTK_SLF_IO_HH
#define _FLF_HTK_SLF_IO_HH

#include <Core/Choice.hh>
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/Vector.hh>

#include "FlfCore/Lattice.hh"
#include "Archive.hh"
#include "Io.hh"

/**
 * Implementation of HTK's standard lattice format
 **/
namespace Flf {
    /*
      Differences to the "official" format specification
      - no support for sub-lattices
      - no support for the div field
      - no support for the ngram field
      - no support for acoustic field at nodes
      - time field is compulsory
    */

    typedef enum { HtkSlfForward, HtkSlfBackward } HtkSlfType;

    class HtkSlfContext;
    typedef Core::Ref<HtkSlfContext> HtkSlfContextRef;

    class HtkSlfContext : public Core::ReferenceCounted {
    private:
	Fsa::LabelId unkId_, silId_;
	HtkSlfType type_;
	f32 fps_;
	bool isCapitalize_;
	bool mergePenalty_;

	f32 base_;
	std::string epsSymbol_;
	ConstSemiringRef semiring_;
	ScoreId amId_, lmId_, penaltyId_;
	Score wrdPenalty_, silPenalty_;
	std::string lmName_;

    public:
	HtkSlfContext();
	~HtkSlfContext();

	Fsa::LabelId unknown() const { return unkId_; }
	Fsa::LabelId silence() const { return silId_; }

	void setType(HtkSlfType type);
	HtkSlfType type() const { return type_; }

	void setFps(f32 fps);
	f32 fps() const { return fps_; }

	void setCapitalize(bool isCapitalize);
	bool capitalize() const { return isCapitalize_; }

	void setMergePenalties(bool mergePenalty);
	bool mergePenalty() const { return mergePenalty_; }

	void setBase(f32 base);
	f32 base() const { return base_; }

	void setEpsSymbol(const std::string&);
	const std::string & epsSymbol() const { return epsSymbol_; }

	void setSemiring(ConstSemiringRef semiring);
	ConstSemiringRef semiring() const { return semiring_; }
	ScoreId amId() const { return amId_; }
	ScoreId lmId() const { return lmId_; }
	ScoreId penaltyId() const { return penaltyId_; }

	// if silPenalty is not given or invalid, silPenalty = wrdPenalty
	void setPenalties(Score wrdPenalty, Score silPenalty = Semiring::Invalid);
	bool hasPenalties() const { return wrdPenalty_ != Semiring::Invalid; }
	Score wordPenalty() const { return wrdPenalty_; }
	Score silPenalty() const { return silPenalty_; }

	void setLmName(const std::string &lmName);
	const std::string & lmName() const { return lmName_; }
	bool cmpLm(const std::string &lmName) const { return lmName_ == lmName; }

	std::string info() const;
	void clear();

	static HtkSlfContextRef create(const Core::Configuration &config);
    };


    struct HtkSlfHeader {
	std::string version;
	std::string utterance;
	f32 base;
	std::string lmName;
	Score lmScale;
	Score wrdPenalty;
	Score silPenalty;
	u32 nNodes;
	u32 nLinks;
	HtkSlfHeader() { reset(); }
	void reset() {
	    version.clear();
	    utterance.clear();
	    base = -1.0;
	    lmName.clear();
	    lmScale = Semiring::UndefinedScale;
	    wrdPenalty = Semiring::One;
	    silPenalty = Semiring::Invalid;
	    nNodes = nLinks = 0;
	}
    };


    /**
     * reads lattices stored in HTK's standard lattice format
     **/
    class HtkSlfBuilder;
    class HtkSlfReader : public LatticeReader {
	friend class HtkSlfBuilder;
	typedef LatticeReader Precursor;
    public:
	static const u32 TrustContext;
	static const u32 AdaptContext;
	static const u32 CheckContext;
	static const u32 UpdateContext;
	static const Core::ParameterString paramContextMode;
	static const Core::ParameterBool paramLogComments;
	static const Core::ParameterString paramEncoding;

    protected:
	static const u32 ResetContext;

    private:
	std::string encoding_;
	HtkSlfBuilder *builder_;

    public:
	HtkSlfReader(const Core::Configuration&);
	HtkSlfReader(const Core::Configuration&, HtkSlfContextRef, u32 contextHandling = CheckContext);
	virtual ~HtkSlfReader();

	HtkSlfContextRef context() const;
	void setContext(HtkSlfContextRef context);

	void setContextHandling(u32);
	u32 contextHandling() const;

	const std::string & encoding() const { return encoding_; }
	void setEncoding(const std::string &encoding) { encoding_ = encoding; }

	HtkSlfHeader* readHeader(const std::string &filename, Core::Archive *archive = 0);
	ConstLatticeRef read(const std::string &filename, Core::Archive *archive);
	virtual ConstLatticeRef read(const std::string &filename) {
	    return read(filename, 0);
	}
    };



    /**
     * writes lattices using HTK's standard lattice format
     **/
    class HtkSlfWriter : public LatticeWriter {
	typedef LatticeWriter Precursor;
    public:
	typedef Core::Vector<Fsa::StateId> StateIdList;
	typedef std::vector<ConstStateRef> ConstStateRefList;

	struct StateIdMapping {
	    StateIdList htk2fsa;
	    StateIdList fsa2htk;
	    ConstStateRefList finals;
	    void clear() { htk2fsa.clear(); fsa2htk.clear(); finals.clear(); }
	};

	class ToWordVariant;

    private:
	mutable ToWordVariant *label2wv_;
	ToWordVariant *lp2wv_, *l2wv_;
	std::string encoding_;
	f32 fps_;

    protected:
	void writeHeader(HtkSlfHeader &header, std::ostream &os) const;
	void writeNodes(ConstLatticeRef f, HtkSlfHeader &header, StateIdMapping &mapping, std::ostream &os) const;
	void writeLinks(ConstLatticeRef f, HtkSlfHeader &header, StateIdMapping &mapping, std::ostream &os) const;

    public:
	HtkSlfWriter(const Core::Configuration &config);
	virtual ~HtkSlfWriter();

	const std::string& encoding() const { return encoding_; }
	void setEncoding(const std::string encoding);

	f32 fps() const { return fps_; }
	void setFps(f32 fps);

	bool buildHeaderAndMapping(
	    ConstLatticeRef f,
	    HtkSlfHeader &header,
	    StateIdMapping &mapping,
	    bool topological = true) const;

	bool write(
	    ConstLatticeRef f,
	    HtkSlfHeader &header,
	    StateIdMapping &mapping,
	    const std::string &filename,
	    Core::Archive *archive = 0) const;

	bool write(
	    ConstLatticeRef f,
	    const std::string &filename,
	    Core::Archive *archive) const;

	virtual bool write(ConstLatticeRef f, const std::string &filename) {
	    return write(f, filename, 0);
	}
    };



    /**
     * reads lattices from an archive,
     * the lattices must be stored in HTK's standard lattice format
     **/
    class HtkSlfArchiveReader : public LatticeArchiveReader {
	typedef LatticeArchiveReader Precursor;
    private:
	// Core::Archive *archive_;
	HtkSlfReader *reader_;

    protected:
	virtual std::string defaultSuffix() const { return ".lat.gz"; }

    public:
	HtkSlfArchiveReader(
	    const Core::Configuration &config,
	    const std::string &pathname);
	virtual ~HtkSlfArchiveReader();

	/*
	bool hasFile(const std::string &file) const {
	    return archive_->hasFile(file);
	}

	const_iterator files() const {
	    return archive_->files();
	}
	*/

	virtual ConstLatticeRef get(const std::string &id);
    };



    /**
     * writes lattices to an archive,
     * the lattices are stored in HTK's standard lattice format
     **/
    class HtkSlfArchiveWriter : public LatticeArchiveWriter {
	typedef LatticeArchiveWriter Precursor;
    private:
	// Core::Archive *archive_;
	HtkSlfWriter *writer_;

    protected:
	virtual std::string defaultSuffix() const { return ".lat.gz"; }

    public:
	HtkSlfArchiveWriter(
	    const Core::Configuration &config,
	    const std::string &pathname);
	~HtkSlfArchiveWriter();

	virtual void store(const std::string &id, ConstLatticeRef f);
    };

} // namespace Flf

#endif // _FLF_HTK_SLF_IO_HH
