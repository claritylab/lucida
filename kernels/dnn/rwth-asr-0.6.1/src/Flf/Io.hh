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
#ifndef _FLF_IO_HH
#define _FLF_IO_HH

#include <iostream>

#include <Bliss/Lexicon.hh>
#include <Core/Archive.hh>
#include <Core/Choice.hh>
#include <Core/Component.hh>
#include <Core/CompressedStream.hh>
#include <Core/Parameter.hh>

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Flf {

    /**
     * 1) Removes .gz, if existent
     * 2) Removes the last dot-seperated suffix, if existent
     * Returns stripped filename and true if ending .gz was detected (and removed)
     **/
    std::pair<std::string, bool> stripExtension(const std::string &filename);


    /**
     * Convinient wrapper,
     * read from archive, if given
     * read from file system, else
     **/
    struct InputStream {
	Core::ArchiveReader *ar;
	Core::CompressedInputStream *cis;
	std::istream *is;

	InputStream(const std::string &filename, Core::Archive *archive);
	~InputStream();

	operator bool() { return (is) ? bool(*is) : false; }
	operator std::istream&() { require(is); return *is; }
	operator std::istream*() { require(is); return is; }

	std::istream & get() { require(is); return *is; }
	std::istream * steal();
    };


    /**
     * Convinient wrapper,
     * write to archive, if given
     * write to file system, else
     **/
    struct OutputStream {
	Core::ArchiveWriter *aw;
	Core::CompressedOutputStream *cos;
	std::ostream *os;

	OutputStream(const std::string &filename, Core::Archive *archive);
	~OutputStream();

	operator bool() { return (os) ? bool(*os) : false; }
	operator std::ostream&() { require(os); return *os; }
	operator std::ostream*() { require(os); return os; }

	std::ostream & get() { require(os); return *os; }
	std::ostream * steal();
    };


    /**
     * i/o flags
     **/
    class IoFlag {
    public:
	static const u32 WriteHead;
	static const u32 WriteBody;
	static const u32 WriteAll;
    };


    /**
     * i/o formats
     **/
    class IoFormat {
    public:
	static const Core::ParameterString paramEncoding;
	static const Core::ParameterInt paramMargin;
	static const Core::ParameterInt paramIndentation;

	typedef enum {
	    LatticeFormatFlf,
	    LatticeFormatHtkSlf,
	    LatticeFormatLatticeProcessor, // deperecated
	    LatticeFormatOpenFst
	} LatticeFormat;
	static const Core::Choice choiceLatticeFormat;
	static const Core::ParameterChoice paramLatticeFormat;

	typedef enum {
	    CnFormatText,
	    CnFormatXml
	} CnFormat;
	static const Core::Choice choiceCnFormat;
	static const Core::ParameterChoice paramCnFormat;

	typedef enum {
	    PosteriorCnFormatText,
	    PosteriorCnFormatXml,
	    PosteriorCnFormatFlowAlignment
	} PosteriorCnFormat;
	static const Core::Choice choicePosteriorCnFormat;
	static const Core::ParameterChoice paramPosteriorCnFormat;
    };


    /**
     * boundaries i/o
     **/
    bool readBoundariesBinary(ConstLatticeRef l, std::istream &is);
    bool writeBoundariesBinary(ConstLatticeRef l, std::ostream &os);


    /**
     * lattices i/o
     **/
    class LatticeReader;
    class LatticeWriter;
    class LatticeIo : public IoFormat {
    public:
	static LatticeReader * getReader(const Core::Configuration &config);
	static LatticeWriter * getWriter(const Core::Configuration &config);
    };

    class LatticeReader : public Core::Component {
    public:
	LatticeReader(const Core::Configuration &config) : Core::Component(config) {}
	virtual ConstLatticeRef read(const std::string &filename) = 0;
    };

    class LatticeWriter : public Core::Component {
    public:
	LatticeWriter(const Core::Configuration &config) : Core::Component(config) {}
	virtual bool write(ConstLatticeRef f, const std::string &filename) = 0;
    };

    /*
      - read lattice path (and reference) from file, one lattice path
      per line, if file is specified
      - read lattice path (and reference) from command line, if no
      file is specified
      - writes lattice to port 0, id(path as found in file minus
      extension) to port 1, reference (if to be loaded) to port 2
    */
    NodeRef createLatticeReaderNode(const std::string &name, const Core::Configuration &config);

    /*
      - write lattice to file; build filename from id and fixed prefix
      and suffix; request id from port 2, take prefix and postfix from
      config
      - write lattice to file; take filename from config, if no income
      at port 2
      - id usually comes from either an IdNode or an
      SpeechSegmentToIdNode
    */
    NodeRef createLatticeWriterNode(const std::string &name, const Core::Configuration &config);


    /**
     * dot /o
     **/
    /*
      - write lattice to file; build filename from id and fixed prefix;
      request id from port 2, take prefix from config and add suffix
      ".dot"
      - write lattice to file; take filename from config, if no income
      at port 2
      - id usually comes from either an IdNode or an
      SpeechSegmentToIdNode
    */
    NodeRef createDrawerNode(const std::string &name, const Core::Configuration &config);


    /**
     * fsa i/
     * -> use lattice/flf-format for writing fsas
     **/
    /*
      - read fsa path (and reference) from file, one fsa path
      per line, if file is specified
      - read fsa path (and reference) from command line, if no
      file is specified
      - writes fsa or one-dimensional lattice to port 0, id(path as found in file minus
      extension) to port 1, reference (if to be loaded) to port 2
    */
    NodeRef createFsaReaderNode(const std::string &name, const Core::Configuration &config);


    /**
     * ctm i/
     * -> use dump-traceback node for writing ctms
     **/
    /*
      - read ctm-file from path
      - for a request: cut time-overlapping sequence from ctm-file and build linear lattice
      - writes lattice to port 0
    */
    NodeRef createCtmReaderNode(const std::string &name, const Core::Configuration &config);


} // namesapce Lat

#endif // _FLF_IO_HH
