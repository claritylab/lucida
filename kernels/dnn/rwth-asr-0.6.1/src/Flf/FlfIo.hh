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
#ifndef _FLF_FLF_IO_HH
#define _FLF_FLF_IO_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Fsa/AlphabetUtility.hh>
#include <Fsa/Static.hh>
#include <Fsa/Utility.hh>

#include "FlfCore/Lattice.hh"
#include "Archive.hh"
#include "Io.hh"
#include "Lexicon.hh"

/**
 * Implementation of RWTH's fsa based lattice format
 **/
namespace Flf {
    /*
      flf format example:

      <?xml version="1.0" encoding="UTF8"?>
      <lattice [semiring="tropical"] [type="lattice"]>
      [<input-alphabet name="lemma-pronunciation" format="bin" file="input-alphabet.binfsa.gz"/>]
      [<output-alphabet name="evaluation" format="bin" file="output-alphabet.binfsa.gz"/>]
      [<boundaries format="bin" file="test.binwb.gz"/>]
      <structure format="bin" file="test-am.binfsa.gz"/>
      <scores n="3">
	  [<dim id="0">
	      [<name> am </name>]
	      [<fsa format="bin" file="am.binfsa.gz"/>]
	  </dim>]
	  [<dim id="1">
	      [<name> lm </name>]
	      [<scale>20.0</scale>]
	      [<fsa format="bin" file="lm.binfsa.gz"/>]
	  </dim>]
	  [<dim id="3">
	      [<name> penalties </name>]
	  </dim>]
      </scores>
      </lattice>
    */

    struct FsaDescriptor {
	static const Core::ParameterString paramFormat;
	static const Core::ParameterString paramFile;

	std::string format;
	std::string file;

	FsaDescriptor() {}
	FsaDescriptor(const std::string &format, const std::string &file) :
	    format(format), file(file) {}
	FsaDescriptor(const std::string &qualifiedFile);
	FsaDescriptor(const Core::Configuration &config);
	FsaDescriptor & operator= (const FsaDescriptor &desc)
	    { format = desc.format; file = desc.file; return *this; }

	std::string operator() (const std::string &root = "") const
	    { return qualifiedPath(root); }
	bool operator== (const FsaDescriptor &desc) const
	    { return (file == desc.file) && (format == desc.format); }
	operator bool() const
	    { return !file.empty(); }

	/*
	const std::string & format() const
	    { return format; }
	const std::string & file() const
	    { return file; }
	*/

	std::string path(const std::string &root = "") const;
	std::string qualifiedPath(const std::string &root = "") const;

	void clear();

	static FsaDescriptor joinPaths(const FsaDescriptor &desc, const std::string &path);
    };
    typedef std::vector<FsaDescriptor> FsaDescriptorList;


    struct BoundariesDescriptor {
	std::string file;
	BoundariesDescriptor() {}
	BoundariesDescriptor(const std::string &file) :
	    file(file) {}
	std::string operator() (const std::string &root = "") const;
	operator bool() const { return !file.empty(); }

	void clear() { file.clear(); }
    };


    struct FlfDescriptor : public Core::ReferenceCounted {
	std::string id;
	std::string latticeRoot;
	SemiringType semiringType;
	bool hasInputAlphabet;
	std::string inputAlphabetName;
	FsaDescriptor inputAlphabetFile;
	bool hasOutputAlphabet;
	std::string outputAlphabetName;
	FsaDescriptor outputAlphabetFile;
	bool hasBoundaries;
	BoundariesDescriptor boundariesFile;
	FsaDescriptor structureFile;
	KeyList keys;
	ScoreList scales;
	FsaDescriptorList scoreFiles;
	FlfDescriptor() {
	    semiringType = Fsa::SemiringTypeUnknown;
	    hasInputAlphabet = false;
	    hasOutputAlphabet = false;
	    hasBoundaries = false;
	}
	std::string info() const;
    };
    typedef Core::Ref<FlfDescriptor> FlfDescriptorRef;


    class FlfContext : public Core::ReferenceCounted {
    private:
	ConstSemiringRef semiring_;
	Fsa::Type fsaType_;

	FsaDescriptor inputAlphabetQf_;
	Lexicon::AlphabetMapRef inputAlphabetMap_;

	FsaDescriptor outputAlphabetQf_;
	Lexicon::AlphabetMapRef outputAlphabetMap_;

	/*
    protected:
	std::pair<Fsa::ConstAlphabetRef, Fsa::ConstAlphabetRef> loadAlphabets(
	    const FsaDescriptor &qf, Core::Archive *archive);
	*/

    public:
	FlfContext();
	~FlfContext();

	void overhaul(FlfDescriptorRef, const Fsa::StaticAutomaton &structureFsa) const;
	void adapt(FlfDescriptorRef desc, const Fsa::StaticAutomaton &structureFsa, Core::Archive *archive = 0);
	void update(FlfDescriptorRef, const Fsa::StaticAutomaton &structureFsa, Core::Archive *archive = 0);
	void clear();

	ConstSemiringRef semiring() const;
	void setSemiring(ConstSemiringRef semiring);

	Fsa::Type fsaType() const;
	void setFsaType(Fsa::Type type);

	Lexicon::AlphabetMapRef inputAlphabetMap() const;
	void setInputAlphabet(Fsa::ConstAlphabetRef alphabet, const std::string &alphabetName);
	void loadInputAlphabet(const FsaDescriptor &fromQf, Core::Archive *archive, const std::string &alphabetName);

	Lexicon::AlphabetMapRef outputAlphabetMap() const;
	void setOutputAlphabet(Fsa::ConstAlphabetRef alphabet, const std::string &alphabetName);
	void loadOutputAlphabet(const FsaDescriptor &fromQf, Core::Archive *archive, const std::string &alphabetName);

	std::string info() const;
    };
    typedef Core::Ref<FlfContext> FlfContextRef;



    /**
     * reads lattices stored in the fsa-based lattice format
     **/
    class FlfReader : public LatticeReader {
	typedef LatticeReader Precursor;
    public:
	static const u32 TrustContext;
	static const u32 AdaptContext;
	static const u32 UpdateContext;
	static const Core::ParameterString paramContextMode;
	static const Core::ParameterStringVector paramKeys;
	static const Core::ParameterFloat paramScale;

    private:
	u32 contextHandling_;
	FlfContextRef context_;
	std::set<Key> partialKeys_;
	KeyList appendKeys_;
	ScoreList appendScales_;

    private:
	void init();
	void conform(FlfDescriptorRef desc, const Fsa::StaticAutomaton &structureFsa, Core::Archive *archive);
	void addStructure(StaticLattice &l, const Fsa::StaticAutomaton &f);
	void addScores(StaticLattice &l, ScoreId id, const Fsa::StaticAutomaton &f);

    public:
	FlfReader(const Core::Configuration&);
	FlfReader(const Core::Configuration&, FlfContextRef, u32 contextHandling = TrustContext);
	virtual ~FlfReader();

	FlfContextRef context() const;
	void setContext(FlfContextRef context);

	void setContextHandling(u32);
	u32 contextHandling() const;

	FlfDescriptorRef readDescriptor(
	    const std::string &descFilename,
	    Core::Archive *archive = 0);

	ConstLatticeRef read(
	    FlfDescriptorRef desc,
	    Core::Archive *archive = 0);

	ConstLatticeRef read(
	    const std::string &descFilename,
	    Core::Archive *archive);

	virtual ConstLatticeRef read(const std::string &filename) {
	    return read(filename, 0);
	}
    };



    /**
     * writes lattices using the fsa-based lattice format
     **/
    class FlfWriter : public LatticeWriter {
	typedef LatticeWriter Precursor;
    public:
	static const Core::ParameterStringVector paramKeys;
	static const Core::ParameterBool paramAdd;
    private:
	bool partial_;
	std::set<Key> partialKeys_;
	bool partialAdd_;
    public:
	FlfWriter(const Core::Configuration &config);
	virtual ~FlfWriter();

	FlfDescriptorRef buildDescriptor(
	    ConstLatticeRef f,
	    const std::string &descFilename) const;

	bool writeDescriptor(
	    FlfDescriptorRef desc,
	    const std::string &descFilename,
	    Core::Archive *archive = 0) const;

	bool write(
	    ConstLatticeRef f,
	    FlfDescriptorRef desc,
	    const std::string &descFilename,
	    bool storeAlphabet = true,
	    Core::Archive *archive = 0) const;

	bool write(
	    ConstLatticeRef f,
	    const std::string &descFilename,
	    bool storeAlphabet,
	    Core::Archive *archive) const;

	virtual bool write(ConstLatticeRef f, const std::string &filename) {
	    return write(f, filename, true, 0);
	}
    };



    /**
     * reads lattices from an archive,
     * the lattices must be stored in the fsa-based lattice format
     **/
    class FlfArchiveReader : public LatticeArchiveReader {
	typedef LatticeArchiveReader Precursor;
    public:
	static const Core::ParameterString paramLatticeType;
	static const Core::ParameterString paramName;

    private:
	typedef std::vector<std::string> StringList;

	// Core::Archive *archive_;
	FlfReader *reader_;
	std::string boundariesSuffix_;
	std::string structureSuffix_;
	StringList suffixes_;
	FlfDescriptorRef desc_;

    protected:
	virtual std::string defaultSuffix() const { return ".flf.gz"; }
	void setDescriptor(const Core::Configuration &config, FlfContextRef context);
	void setFsas(const std::string &id);

    public:
	FlfArchiveReader(
	    const Core::Configuration &config,
	    const std::string &pathname);
	virtual ~FlfArchiveReader();

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
     * the lattices are stored in the fsa-based lattice format
     **/
    class FlfArchiveWriter : public LatticeArchiveWriter {
	typedef LatticeArchiveWriter Precursor;

    private:
	// Core::Archive *archive_;

	FlfWriter *writer_;
	Fsa::ConstAlphabetRef inputAlphabet_;
	FsaDescriptor inputAlphabetQf_;
	Fsa::ConstAlphabetRef outputAlphabet_;
	FsaDescriptor outputAlphabetQf_;
	FsaDescriptor alphabetQf_;

    protected:
	virtual std::string defaultSuffix() const { return ".flf.gz"; }
	virtual void finalize();

	void storeAlphabets();
	FlfDescriptorRef buildDescriptor(ConstLatticeRef l, const std::string &id, const std::string &filename);

    public:
	FlfArchiveWriter(
	    const Core::Configuration &config,
	    const std::string &pathname);
	~FlfArchiveWriter();

	virtual void store(const std::string &id, ConstLatticeRef f);
    };

} // namespace Flf

#endif // _FLF_FLF_IO_HH
