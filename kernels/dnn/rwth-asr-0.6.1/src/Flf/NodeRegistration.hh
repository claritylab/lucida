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
#ifndef _FLF_NODE_REGISTRATION_HH
#define _FLF_NODE_REGISTRATION_HH

#include <Modules.hh>

#include "Archive.hh"
#include "Best.hh"
#include "Cache.hh"
#include "CenterFrameConfusionNetworkBuilder.hh"
#include "Compose.hh"
#include "Concatenate.hh"
#include "ConfusionNetwork.hh"
#include "ConfusionNetworkCombination.hh"
#include "ConfusionNetworkIo.hh"
#include "Convert.hh"
#include "Copy.hh"
#include "CorpusProcessor.hh"
#include "Determinize.hh"
#include "Evaluate.hh"
#include "EpsilonRemoval.hh"
#include "Filter.hh"
#include "GammaCorrection.hh"
#include "Info.hh"
#include "Io.hh"
#include "LanguageModel.hh"
#include "Lexicon.hh"
#include "LocalCostDecoder.hh"
#include "Map.hh"
#include "Miscellaneous.hh"
#include "Module.hh"
#include "NBest.hh"
#include "Network.hh"
#include "NodeFactory.hh"
#include "NonWordFilter.hh"
#include "PivotArcConfusionNetworkBuilder.hh"
#include "Prune.hh"
#include "Recognizer.hh"
#include "Rescale.hh"
#include "Rescore.hh"
#include "RescoreLm.hh"
#include "Segment.hh"
#include "StateClusterConfusionNetworkBuilder.hh"
#include "TimeAlignment.hh"
#include "TimeframeConfusionNetwork.hh"
#include "TimeframeConfusionNetworkBuilder.hh"
#include "TimeframeConfusionNetworkCombination.hh"
#include "TimeframeConfusionNetworkIo.hh"
#include "TimeframeError.hh"
#include "Traceback.hh"
#include "Union.hh"


namespace Flf {
    /**
      List of nodes:
    **/
    void registerNodeCreators(NodeFactory *factory) {

	/*
	  Initial nodes
	*/
	factory->add(
	    NodeCreator(
		"speech-segment",
		"Distributes the speech segments provided\n"
		"by the Bliss corpus visitor.\n"
		"The segment is provided as Bliss speech segment\n"
		"and as Flf segment.",
		"[*.network.speech-segment]\n"
		"type                   = speech-segment",
		"no input\n"
		"output:\n"
		"  0:segment 1:bliss-speech-segment",
		&createSpeechSegmentNode));

	factory->add(
	    NodeCreator(
		"batch",
		"Read argument list(s) either from command line or from file;\n"
		"in the case of a file, every line is interpreted as an argument list.\n"
		"Argument number x is accessed via port x.\n",
		"[*.network.batch]\n"
		"type                        = batch\n"
		"file			     = <batch-list>\n"
		"encoding                    = utf-8",
		"no input\n"
		"output:\n"
		"  x: argument[x]",
		&createBatchNode));

	factory->add(
	    NodeCreator(
		"segment-builder",
		"Combines incoming data to a segment;\n"
		"missing data fields are replaced by default values.",
		"[*.network.segment-builder]\n"
		"type                        = segment-builder\n"
		"progress.channel            = <unset>",
		"input:\n"
		"  [0:bliss-speech-segment]\n"
		"  [1:audio-filename(string)]\n"
		"  [2:start-time(float)]\n"
		"  [3:end-time(float)]\n"
		"  [4:track(int)]\n"
		"  [5:orthography(string)]\n"
		"  [6:speaker-id(string)]\n"
		"  [7:condition-id(string)]\n"
		"  [8:recording-id(string)]\n"
		"  [9:segment-id(string)]\n"
		"output:\n"
		"  0: segment",
		&createSegmentBuilderNode));

	/*
	  Input/Output
	*/
	factory->add(
	    NodeCreator(
		"reader",
		"Read lattice(s) from file;\n"
		"All filenames are interpreted relative to a given directory,\n"
		"if specified, else to the current directory.\n"
		"The current lattice is buffered for multiple access.",
		"[*.network.reader]\n"
		"type                        = reader\n"
		"format                      = flf|htk\n"
		"path                        = <lattice-base-dir>\n"
		"# if format is flf\n"
		"[*.network.reader.flf]\n"
		"context-mode                = trust|adapt|*update\n"
		"[*.network.reader.flf.partial]\n"
		"keys                        = key1 key2 ...\n"
		"[*.network.reader.flf.append]\n"
		"keys                        = key1 key2 ...\n"
		"key1.scale                  = 1.0\n"
		"key2.scale                  = 1.0\n"
		"...\n"
		"# if format is htk\n"
		"[*.network.reader.htk]\n"
		"context-mode                = trust|adapt|*update\n"
		"log-comments                = false\n"
		"suffix                      = .lat\n"
		"fps                         = 100\n"
		"encoding                    = utf-8\n"
		"slf-type                    = forward|backward\n"
		"capitalize                  = false\n"
		"word-penalty                = <f32>\n"
		"silence-penalty             = <f32>\n"
		"merge-penalties             = false\n"
		"set-coarticulation          = false",
		"input:\n"
		"  1:segment | 2:string\n"
		"output:\n"
		"  0:lattice",
		&createLatticeReaderNode));
	factory->add(
	    NodeCreator(
		"writer",
		"Write lattice(s) to file;\n"
		"If input at port 1, use segment id as base name,\n"
		"if input at port 2, use string as base name,\n"
		"else, get filename from config.\n"
		"Base name is modified by adding suffix and prefix, if given.\n"
		"All filenames are interpreted relative to a given directory,\n"
		"if specified, else to the current directory.",
		"[*.network.writer]\n"
		"type                        = writer\n"
		"format                      = flf|htk\n"
		"# to store a single lattice\n"
		"file                        = <lattice-file>\n"
		"# to store multiple lattices,\n"
		"# i.e. if incoming connection at port 1 or 2\n"
		"path                        = <lattice-base-dir>\n"
		"prefix                      = <file-prefix>\n"
		"suffix                      = <file-suffix>\n"
		"[*.network.writer.flf.partial]\n"
		"keys                        = key1 key2 ...\n"
		"add                         = false\n"
		"[*.network.writer.htk]\n"
		"fps                         = 100\n"
		"encoding                    = utf-8",
		"input:\n"
		"  0:lattice[, 1:segment | 2:string]\n"
		"output:\n"
		"  0:lattice",
		&createLatticeWriterNode));
	factory->add(
	    NodeCreator(
		"archive-reader",
		"Read lattices from archive;\n"
		"the lattice is buffered for multiple access.",
		"[*.network.archive-reader]\n"
		"type                        = archive-reader\n"
		"format                      = flf|htk\n"
		"path                        = <archive-path>\n"
		"info                        = false\n"
		"# if format is flf\n"
		"[*.network.archive-reader.flf]\n"
		"suffix                      = .flf.gz\n"
		"[*.network.archive-reader.flf.partial]\n"
		"keys                        = key1 key2 ...\n"
		"[*.network.archive-reader.flf.append]\n"
		"keys                        = key1 key2 ...\n"
		"key1.scale                  = 1.0\n"
		"key2.scale                  = 1.0\n"
		"...\n"
		"# if format is htk\n"
		"[*.network.archive-reader.htk]\n"
		"suffix                      = .lat.gz\n"
		"fps                         = 100\n"
		"encoding                    = utf-8\n"
		"slf-type                    = forward|backward\n"
		"capitalize                  = false\n"
		"word-penalty                = <f32>\n"
		"silence-penalty             = <f32>\n"
		"merge-penalties             = false\n"
		"set-coarticulation          = false\n"
		"eps-symbol                  = !NULL\n"
		"# archive specific options\n"
		"[*.network.archive-reader.*.semiring]\n"
		"type                        = tropical|log\n"
		"tolerance                   = <default-tolerance>\n"
		"keys                        = key1 key2 ...\n"
		"key1.scale                  = <f32>\n"
		"key2.scale                  = <f32>\n"
		"...\n"
		"# if format is flf AND semiring is specified\n"
		"[*.network.archive-reader.flf]\n"
		"input-alphabet.name         = {lemma-pronunciation*|lemma|syntax|evaluation}\n"
		"input-alphabet.format       = bin\n"
		"input-alphabet.file         = <alphabet-file>\n"
		"output-alphabet.name        = {lemma-pronunciation*|lemma|syntax|evaluation}\n"
		"output-alphabet.format      = bin\n"
		"output-alphabet.file        = <alphabet-file>\n"
		"boundaries.suffix           = <boundaries-file-suffix>\n"
		"key1.format                 = bin\n"
		"key1.suffix                 = <fsa-file-suffix>\n"
		"...",
		"input:\n"
		"  1:segment | 2:string\n"
		"output:\n"
		"  0:lattice",
		&createLatticeArchiveReaderNode));
	factory->add(
	    NodeCreator(
		"archive-writer",
		"Store lattices in archive.",
		"[*.network.archive-writer]\n"
		"type                        = archive-writer\n"
		"format                      = flf|htk|lattice-processor\n"
		"path                        = <archive-path>\n"
		"info                        = false\n"
		"# if format is flf\n"
		"[*.network.archive-writer.flf]\n"
		"suffix                      = .flf.gz\n"
		"input-alphabet.format       = bin\n"
		"input-alphabet.file         = bin:input-alphabet.binfsa.gz\n"
		"output-alphabet.format      = bin\n"
		"output-alphabet.file        = bin:output-alphabet.binfsa.gz\n"
		"alphabets.format            = \n"
		"alphabets.file              = \n"
		"[*.network.archive-writer.flf.partial]\n"
		"keys                        = key1 key2 ...\n"
		"add                         = false\n"
		"# if format is htk\n"
		"[*.network.archive-writer.htk]\n"
		"suffix                      = .lat.gz\n"
		"fps                         = 100\n"
		"encoding                    = utf-8\n"
		"# if format is htk\n"
		"[*.network.archive-writer.lattice-processor]\n"
		"pronunciation-scale         = <required>",
		"input:\n"
		"  0:lattice, 1:segment | 2:string\n"
		"output:\n"
		"  0:lattice",
		&createLatticeArchiveWriterNode));
	factory->add(
	    NodeCreator(
		"fsa-reader",
		"Read fsas.\n"
		"All filenames are interpreted relative to a given directory,\n"
		"if specified, else to the current directory.\n"
		"The current fsa is buffered for multiple access\n",
		"[*.network.fsa-reader]\n"
		"type                        = fsa-reader\n"
		"path                        = <path>\n"
		"# in case of acceptors\n"
		"alphabet.name               = {lemma-pronunciation|lemma*|syntax|evaluation}\n"
		"# in case of transducers\n"
		"input-alphabet.name         = {lemma-pronunciation|lemma*|syntax|evaluation}\n"
		"output-alphabet.name        = {lemma-pronunciation|lemma*|syntax|evaluation}",
		"input:\n"
		"  1:segment | 2:string\n"
		"output:\n"
		"  0:lattice, 1:fsa",
		&createFsaReaderNode));
	factory->add(
	    NodeCreator(
		"ctm-reader",
		"Read a single ctm-file.\n"
		"CTM format is:\n"
		"<name> <track> <start> <duration> <word> [<score1> [<score2> ...]]\n"
		"For a given segment a linear lattice is build from the\n"
		"from overlapping part.\n"
		"A semiring can be specified as well as list of keys mapping\n"
		"the CTM scores to the semiring dimensions.\n"
		"If no keys are given, the keys from the semiring are used.\n"
		"If no semiring is given, the keys are used to build a semiring.\n"
		"If none is given, the empty semiring is used.\n"
		"Example:\n"
		"Configuration for a CTM file providing confidence scores.\n"
		"scores             = confidence\n"
		"confidence.default = 1.0",
		"[*.network.ctm-reader]\n"
		"type                        = ctm-reader\n"
		"path                        = <path>\n"
		"encoding                    = utf-8\n"
		"scores                      = key1 key2 ...\n"
		"key1.default                = <f32>\n"
		"key2.default                = <f32>\n"
		"...\n"
		"[*.network.ctm-reader.semiring]\n"
		"type                        = tropical|log\n"
		"tolerance                   = <default-tolerance>\n"
		"keys                        = key1 key2 ...\n"
		"key1.scale                  = <f32>\n"
		"key2.scale                  = <f32>\n"
		"...",
		"input:\n"
		"  1:segment\n"
		"output:\n"
		"  0:lattice",
		&createCtmReaderNode));
	factory->add(
	    NodeCreator(
		"drawer",
		"Draw lattice(s) in dot format to file.\n"
		"For filename generation see \"writer\".",
		"[*.network.drawer]\n"
		"type                        = drawer\n"
		"hints                       = {detailed best probability unscaled}\n"
		"# to draw a single lattice\n"
		"file                        = <dot-file>\n"
		"# to draw multiple files,\n"
		"# i.e. if incoming connection at port 1\n"
		"path                        = <dot-base-dir>\n"
		"prefix                      = <file-prefix>\n"
		"suffix                      = <file-suffix>",
		"input:\n"
		"  0:lattice[, 1:segment | 2:string]\n"
		"output:\n"
		"  0:lattice",
		&createDrawerNode));

	/*
	  Flow
	*/
	factory->add(
	    NodeCreator(
		"sink",
		"Let all incoming lattices/CNs/fCNs sink.",
		"[*.network.sink]\n"
		"type                        = sink\n"
		"sink-type                   = lattice*|CN|fCN\n"
		"warn-on-empty               = true\n"
		"error-on-empty              = false",
		"input:\n"
		"  x:lattice/CN/fCN\n"
		"no output",
		&createSink));
	factory->add(
	    NodeCreator(
		"buffer",
		"Incoming lattice is buffered until next sync and\n"
		"manifolded to all outgoing ports.",
		"[*.network.buffer]\n"
		"type                        = buffer",
		"input:\n"
		"  x:lattice (at exactly one port)\n"
		"output:\n"
		"  x:lattice",
		&createLatticeBufferNode));
	factory->add(
	    NodeCreator(
		"cache",
		"State requests to incoming lattice are cached;\n"
		"see Fsa for details.",
		"[*.network.cache]\n"
		"type                        = cache\n"
		"max-age                     = 10000",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createCacheNode));
	factory->add(
	    NodeCreator(
		"copy",
		"Make static copy of incoming lattice.\n"
		"By default, scores are copied by reference.\n"
		"Optional in-sito trimming and/or state numbering normalization\n"
		"is supported.",
		"[*.network.copy]\n"
		"type                        = copy\n"
		"# make deep copy, i.e. copy scores by value and not by reference\n"
		"deep                        = false\n"
		"trim                        = false\n"
		"normalize                   = false",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createCopyNode));

	/*
	  Info
	*/
	factory->add(
	    NodeCreator(
		"info",
		"Dump information and statistics for incoming lattice.\n"
		"Runtime/memory requirements:\n"
		"cheap:    O(1), lattice is not traversed.\n"
		"normal:   O(N), lattice is traversed once; no caching.\n"
		"extended: O(N), lattice is traversed multiple times,\n"
		"          lattice is cached.\n"
		"memory:   n/a\n"
		"Attention: \"extended\" requires an acyclic lattice.",
		"[*.network.info]\n"
		"type                        = info\n"
		"info-type                   = cheap|normal*|extended|memory",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createInfoNode));

	/*
	  Best/Traceback
	*/
	factory->add(
	    NodeCreator(
		"best",
		"Find the best path in a lattice.\n"
		"Usually, Dijkstra is faster than Bellman-Ford,\n"
		"but Dijkstra does not guarantee correct results in the\n"
		"presence of negative arc scores.",
		"[*.network.best]\n"
		"type                        = best\n"
		"algorithm                   = dijkstra*|bellman-ford|projecting-bellman-ford",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createBestNode));
	factory->add(
	    NodeCreator(
		"n-best",
		"Find the n best paths in a lattice.\n"
		"The algorithm is based on Eppstein and is\n"
		"optimized for discarding duplicates, i.e.\n"
		"the algorithm is not optimal (compared to\n"
		"the origianl Eppstein algorithm) for generating\n"
		"n-best lists containing duplicates.\n"
		"The algorithm seems to scale well at least up\n"
		"to 100.000-best lists without duplicates.\n"
		"If the \"ignore-non-word\" option is activated,\n"
		"then two hypotheses only differing in non-words\n"
		"are considered duplicates.\n"
		"The resulting n-best list preserves all non-word-\n"
		"and epsilon-arcs and has correct time boundaries.",
		"[*.network.n-best]\n"
		"type                        = best\n"
		"n                           = 1\n"
		"remove-duplicates           = true\n"
		"ignore-non-words            = true\n"
		"score-key                   = <unset>",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createNBestNode));
	factory->add(
	    NodeCreator(
		"select-n-best",
		"Gets an n-best lattice as input and provides at port x\n"
		"the xth best lattice, or the empty lattice if x exceeds\n"
		"the size of the n-best list; the indexing starts from 0.",
		"[*.network.select-n-best]\n"
		"type                        = select-n-best",
		"input:\n"
		"  0:n-best-lattice\n"
		"output:\n"
		"  x:linear-lattice",
		&createSelectNBestNode));
	factory->add(
	    NodeCreator(
		"dump-n-best",
		"Dumps a linear or n-best-list lattice.",
		"[*.network.dump-n-best]\n"
		"type                        = dump-n-best\n"
		"dump.channel                = <file>\n"
		"scores                      = <key-1> <key-2> ... # default is all scores",
		"input:\n"
		"  0:n-best-lattice[, 1:segment]\n"
		"output:\n"
		"  0:n-best-lattice",
		&createDumpNBestNode));
	factory->add(
	    NodeCreator(
		"dump-traceback",
		"Dumps a linear lattice or an n-best list in a traceback format,\n"
		"i.e. the output includes time information for each item.\n"
		"For tracebacks in Bliss format, the lattice is mapped to lemma-pronunciation.\n"
		"The CTM format is independent of the input alphabet; if the \"dump-orthography\"\n"
		"option is active, the lattice is mapped to lemma.\n"
		"For phoneme or subword alignments, the input alphabet must be lemma or lemma-\n"
		"pronunciation and at port 1 a valid Bliss-segment is required. If an alignment\n"
		"for a lemma is requested, the result is the Viterbi alignment over all matching\n"
		"pronunciations.",
		"[*.network.dump-traceback]\n"
		"type                        = dump-traceback\n"
		"format                      = bliss|corpus|ctm*\n"
		"dump.channel                = <file>\n"
		"[*.network.dump-traceback.ctm]\n"
		"dump-orthography            = true\n"
		"dump-coarticulation         = false\n"
		"dump-non-word               = false\n"
		"dump-eps                    = <dump-non-word>\n"
		"non-word-symbol             = <unset> # use lexicon representation for non-words \n"
		"                                        and !NULL for eps arcs; if set, then use\n"
		"                                        for non-word and for eps arcs.\n"
		"scores                      = <key-1> <key-2> ...\n"
		"dump-type                   = false\n"
		"dump-phoneme-alignment      = false\n"
		"dump-subword-alignment      = false\n"
		"subword-map.file            = <unset>",
		"input:\n"
		"  0:lattice[, 1:segment]\n"
		"output:\n"
		"  0:lattice",
		&createDumpTracebackNode));
	factory->add(
	    NodeCreator(
		"dump-all-pairs-best",
		"Calculates and dumps the shortest distance between all state pairs\n"
		"and dump them in plain text. The shortest distance is the minimum sum\n"
		"of the projected arc scores; thus the distance is a scalar.\n"
		"If time threshold is set, then only pairs of states are considered,\n"
		"where the distance in time does not exceed the threshold.",
		"[*.network.dump-all-pairs-best]\n"
		"type                        = dump-all-pairs-best\n"
		"dump.channel                = <file>\n"
		"time-threshold              = <unset>",
		"input:\n"
		"  0:lattice[, 1:segment]\n"
		"output:\n"
		"  0:lattice",
		&createDumpAllPairsShortestDistanceNode));


	/*
	  Evaluation
	*/
	factory->add(
	    NodeCreator(
		"evaluator",
		"Calculate WER and/or GER.",
		"[*.network.evaluator]\n"
		"type                        = evaluator\n"
		"single-best                 = true\n"
		"best-in-lattice             = true\n"
		"word-errors                 = true\n"
		"letter-errors               = false\n"
		"phoneme-errors              = false\n"
		"[*.network.evaluator.layer]\n"
		"use                         = true\n"
		"name                        = <node-name>\n"
		"[*.network.evaluator.edit-distance]\n"
		"format                      = bliss*|nist\n"
		"allow-broken-words          = false\n"
		"sub-cost                    = 1\n"
		"ins-cost                    = 1\n"
		"del-cost                    = 1\n"
		"#semiring used for decoding lattice\n"
		"[*.network.evaluator.semiring]\n"
		"type                        = tropical|log\n"
		"tolerance                   = <default-tolerance>\n"
		"keys                        = key1 key2 ...\n"
		"key1.scale                  = <f32>\n"
		"key2.scale                  = <f32>\n"
		"...",
		"input:\n"
		"  0:lattice, {1:segment | 2: reference string}\n"
		"output:\n"
		"  0:lattice",
		&createEvaluatorNode));

	/*
	  Composition
	*/
	factory->add(
	    NodeCreator(
		"compose",
		"see compose-matching",
		"[*.network.compose]\n"
		"type                        = compose",
		"see compose-matching",
		&createComposeMatchingNode));
	factory->add(
	    NodeCreator(
		"compose-matching",
		"Compose two lattices; for algorithm details see FSA.\n"
		"If the left lattice is unweighted, then its weights are\n"
		"set to semiring one (of the semiring of the right lattice)\n"
		"and its word boundaries are invalidated.",
		"[*.network.compose-matching]\n"
		"type                        = compose-matching\n"
		"unweight-left               = false\n"
		"unweight-right              = false",
		"input:\n"
		"  0:lattice, 1:lattice\n"
		"output:\n"
		"  0:lattice",
		&createComposeMatchingNode));
	factory->add(
	    NodeCreator(
		"compose-sequencing",
		"Compose two lattices; for algorithm details see FSA.",
		"[*.network.compose-sequencing]\n"
		"type                        = compose-sequencing",
		"input:\n"
		"  0:lattice, 1:lattice\n"
		"output:\n"
		"  0:lattice",
		&createComposeSequencingNode));
	factory->add(
	    NodeCreator(
		"difference",
		"Difference of two lattices; for algorithm details see FSA.",
		"[*.network.difference]\n"
		"type                        = difference",
		"input:\n"
		"  0:lattice, 1:lattice\n"
		"output:\n"
		"  0:lattice",
		&createDifferenceNode));
	factory->add(
	    NodeCreator(
		"intersection",
		"Intersection of two lattices; for algorithm details see FSA.",
		"[*.network.intersection]\n"
		"type                        = intersection\n"
		"append                      = false",
		"input:\n"
		"  0:lattice, 1:lattice\n"
		"output:\n"
		"  0:lattice",
		&createIntersectionNode));
	factory->add(
	    NodeCreator(
		"compose-with-fsa",
		"Compose with an fsa and rescore a single lattice dimension.\n"
		"Composition uses the \"compose sequencing\" algorithm, see FSA.",
		"[*.network.compose-with-fsa]\n"
		"type                        = compose-with-fsa\n"
		"append                      = false\n"
		"key                         = <symbolic key or dim>\n"
		"scale                       = 1\n"
		"rescore-mode                = clone*|in-place-cached|in-place\n"
		"# i.e. if port 1 is not connected\n"
		"file                        = <fsa-filename>\n"
		"# in case of acceptor\n"
		"alphabet.name               = {lemma-pronunciation|lemma|syntax|evaluation}\n"
		"# in case of transducer\n"
		"input-alphabet.name         = {lemma-pronunciation|lemma|syntax|evaluation}\n"
		"output-alphabet.name        = {lemma-pronunciation|lemma|syntax|evaluation}",
		"input:\n"
		"  0:lattice[, 1: fsa]\n"
		"output:\n"
		"  0:lattice",
		&createComposeWithFsaNode));
	factory->add(
	    NodeCreator(
		"compose-with-lm",
		"Compose LM with lattice and rescore a single lattice dimension.\n"
		"The \"force-sentence-end=true\", then each segment end is treated as a\n"
		"sentence end, regardless of any arcs labeled with the sentence end symbol.\n",
		"[*.network.compose-with-lm]\n"
		"type                        = compose-with-lm\n"
		"append                      = false\n"
		"key                         = <symbolic key or dim>\n"
		"scale                       = 1\n"
		"force-sentence-end          = true\n"
		"project-input               = false\n"
		"[*.network.compose-with-lm.lm]\n"
		"(see module Lm)",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createComposeWithLmNode));

	/*
	  Extend/Rescore
	*/
	factory->add(
	    NodeCreator(
		"add",
		"Manipulate a single dimension:\n"
		"f(x_d) = x_d + <score>",
		"[*.network.add]\n"
		"type                        = add\n"
		"append                      = false\n"
		"key                         = <symbolic key or dim>\n"
		"score                       = 0.0\n"
		"rescore-mode                = {clone*, in-place-cached, in-place}",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createAddNode));
	factory->add(
	    NodeCreator(
		"multiply",
		"Manipulate a single dimension:\n"
		"f(x_d) = <scale> * x_d",
		"[*.network.multiply]\n"
		"type                        = multiply\n"
		"append                      = false\n"
		"key                         = <symbolic key or dim>\n"
		"scale                       = 1.0\n"
		"rescore-mode                = {clone*, in-place-cached, in-place}",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createMultiplyNode));
	factory->add(
	    NodeCreator(
		"exp",
		"Manipulate a single dimension:\n"
		"f(x_d) = exp(<scale> * x_d)",
		"[*.network.exp]\n"
		"type                        = exp\n"
		"append                      = false\n"
		"key                         = <symbolic key or dim>\n"
		"scale                       = 1.0\n"
		"rescore-mode                = {clone*, in-place-cached, in-place}",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createExpNode));
	factory->add(
	    NodeCreator(
		"log",
		"Manipulate a single dimension:\n"
		"f(x_d) = <scale> * log(x_d)",
		"[*.network.log]\n"
		"type                        = log\n"
		"append                      = false\n"
		"key                         = <symbolic key or dim>\n"
		"scale                       = 1.0\n"
		"rescore-mode                = {clone*, in-place-cached, in-place}",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createLogNode));
	factory->add(
	    NodeCreator(
		"extend-by-penalty",
		"Penalize a single dimension. The penalty can be made input-label\n"
		"dependent:\n"
		"First, a list of class labels is defined. Second, each class label gets\n"
		"a list of othographies and a penalty assigned.\n"
		"Class penalties overwrites the default penalty.",
		"[*.network.extend-by-penalty]\n"
		"type                        = extend-by-penalty\n"
		"append                      = false\n"
		"key                         = <symbolic key or dim>\n"
		"scale                       = 1.0\n"
		"rescore-mode                = {clone*, in-place-cached, in-place}\n"
		"# default penalty\n"
		"penalty                     = 0.0\n"
		"# class dependent penalties (optional)\n"
		"[*.network.extend-by-penalty.mapping]\n"
		"classes                     = <class1> <class2> ...\n"
		"<class1>.orth               = <orth1> <orth2> ...\n"
		"<class1>.penalty            = 0.0",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createExtendByPenaltyNode));
	factory->add(
	    NodeCreator(
		"extend-by-pronunciation-score",
		"A single dimension is extended by the pronunciation score.\n"
		"The pronunciation score is derived form the lexicon.",
		"[*.network.extend-by-pronunciation-score]\n"
		"type                        = extend-by-pronunciation-score\n"
		"append                      = false\n"
		"key                         = <symbolic key or dim>\n"
		"scale                       = 1.0\n"
		"rescore-mode                = {clone*, in-place-cached, in-place}",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createExtendByPronunciationScoreNode));
	factory->add(
	    NodeCreator(
		"append",
		"Append two lattices score-wise;\n"
		"both lattices must have equal topology\n"
		"(down to state numbering).\n"
		"The resulting lattice has a semiring consisting\n"
		"of the concatenatation\n"
		"of the two incoming semirings.",
		"[*.network.append]\n"
		"type                        = append",
		"input:\n"
		"  0:lattice 1:lattice\n"
		"output:\n"
		"  0:lattice",
		&createAppendScoresNode));
	factory->add(
	    NodeCreator(
		"decode-rescore-lm",
		"Performs a pruned rescoring/decoding on the lattice.\n"
		"This is most useful to expand mesh lattices (see the mesh node).\n"
		"The applied word end pruning is equivalent to the one applied\n"
		"during decoding (the threshold is relative to the LM scale).\n",
		"[*.network.decode-rescore]\n"
		"type                        = decode-rescore-lm\n",
		"word-end-beam               = 20.0\n"
		"word-end-limit              = 50000\n"
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createDecodeRescoreLmNode));
	factory->add(
	    NodeCreator(
		"expand-transits",
		"Modifies the lattice by expanding the transitions so that\n"
		"each state corresponds to one left and right coarticuled phoneme,\n"
		"or to a non-coarticulated transition.\n"
		"This may be required for correct word boundary information if\n"
		"the decoder doesn't produce it correctly.",
		"[*.network.expand-transits]\n"
		"type                        = expand-transits",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createExpandTransitsNode));

	/*
	  Semiring Modification
	*/
	factory->add(
	    NodeCreator(
		"rescale",
		"Rescales and rename single dimensions of lattice's current semiring.\n"
		"Technically, the semiring of the lattice is replaced by a new one.",
		"[*.network.rescale]\n"
		"type                        = rescale\n"
		"<key1>.scale                = <keep existing scale>\n"
		"<key1>.key                  = <keep existing key name>\n"
		"...",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createRescaleNode));
	factory->add(
	    NodeCreator(
		"change-semiring",
		"Replace the semiring.\n"
		"The target semiring might have a different dimensionality;\n"
		"mapping from the old to the new semiring is done via keys, i.e.\n"
		"the names of the dimensions.\n"
		"The operation does not affect the scores.",
		"[*.network.change-semiring]\n"
		"type                        = change-semiring\n"
		"[*.network.change-semiring.semiring]\n"
		"type                        = tropical|log\n"
		"tolerance                   = <default-tolerance>\n"
		"keys                        = key1 key2 ...\n"
		"key1.scale                  = <f32>\n"
		"key2.scale                  = <f32>\n"
		"...",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createChangeSemiringNode));
	factory->add(
	    NodeCreator(
		"reduce",
		"Reduce the scores of two or more dimensions to the first given dimension.\n"
		"Basically the weighted score of the second, third, and so on key\n"
		"are added to the first score of the first key and then set to semiring one,\n"
		"i.e. 0, and the scale of the dimension is set to 1.\n"
		"The weighted sum of the score vector remains unchanged.",
		"[*.network.scores-reduce-scores]\n"
		"type                        = reduce\n"
		"keys                        = <key1> <key2> ...",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createReduceScoresNode));
	factory->add(
	    NodeCreator(
		"project",
		"Change the semiring by projecting the source semiring onto the target semiring.",
		"[*.network.projection]\n"
		"type                        = project\n"
		"scaled                      = true\n"
		"[*.network.projection.semiring]\n"
		"type                        = tropical|log\n"
		"tolerance                   = <default-tolerance>\n"
		"keys                        = key1 key2 ...\n"
		"key1.scale                  = <f32>\n"
		"key2.scale                  = <f32>\n"
		"...\n"
		"[*.network.projection.matrix]\n"
		"key1.row                    = <old-key[1,1]> <old-key[1,2]> ...\n"
		"key2.row                    = <old-key[2,1]> <old-key[2,2]> ...\n"
		"...",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createProjectSemiringNode));

	/*
	  Determinization
	*/
	factory->add(
	    NodeCreator(
		"determinize",
		"Determinize lattice; for algorithm details see FSA.",
		"[*.network.determinize]\n"
		"type                        = determinize\n"
		"log-semiring                = true|false*\n"
		"log-semiring.alpha          = <unset>",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createDeterminizeNode));
	factory->add(
	    NodeCreator(
		"minimize",
		"Determinize and minimize lattice; for algorithm details see FSA.",
		"[*.network.minimize]\n"
		"type                        = minimize",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createMinimizeNode));

	/*
	  Epsilon/Non-Word closures
	*/
	factory->add(
	    NodeCreator(
		"remove-null-arcs",
		"Remove arcs of length 0(regardless if input/output is eps).",
		"[*.network.remove-null-arcs]\n"
		"type                        = remove-null-arcs\n"
		"log-semiring                = true|false*\n"
		"log-semiring.alpha          = <unset>",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createNullArcsRemovalNode));
	factory->add(
	    NodeCreator(
		"remove-epsilons",
		"Those arcs are removed having epsilon as input and output.",
		"[*.network.remove-epsilons]\n"
		"type                        = remove-epsilons\n"
		"log-semiring                = true|false*\n"
		"log-semiring.alpha          = <unset>",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createEpsilonRemovalNode));
	factory->add(
	    NodeCreator(
		"non-word-closure-filter",
		"Given states s and e. Pathes_w(s,e) is the set of all pathes from\n"
		"s to e having exactly one arc labeled with w and all others labeled\n"
		"with epsilon. Arcs_w(s,e) is the set of all arcs in Pathes_w(s,e) \n"
		"labeled with w. Arcs_s'/w(s,e) is the set of all arcs in Arcs_w(s,e)\n"
		"having source state s'. Pathes_s'/w(s,e) is the subset of Pathes_w(s,e),\n"
		"such that each path in Pathes_s'/w(s,e) includes an arc in Arcs_s'/w(s,e).\n"
		"\n"
		"for each w, (s,e):\n"
		"    for each a in Arcs_w(s,e) keep only the best scoring path in\n"
		"    Pathes_w(s,e) that includes a.\n"
		"-> see classical epsilon-removal over the tropical semiring\n"
		"\n"
		"The resulting graph is a subgraph of the original input and contains the\n"
		"Viterbi path of the original graph.\n"
		"The implementation is static, i.e not lazy.",
		"[*.network.non-word-closure-filter]\n"
		"type                        = non-word-closure-filter",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createNonWordClosureFilterNode));
	factory->add(
	    NodeCreator(
		"non-word-closure-weak-determinization-filter",
		"Given states s and e. Pathes_w(s,e) is the set of all pathes from\n"
		"s to e having exactly one arc labeled with w and all others labeled\n"
		"with epsilon. Arcs_w(s,e) is the set of all arcs in Pathes_w(s,e) \n"
		"labeled with w. Arcs_s'/w(s,e) is the set of all arcs in Arcs_w(s,e)\n"
		"having source state s'. Pathes_s'/w(s,e) is the subset of Pathes_w(s,e),\n"
		"such that each path in Pathes_s'/w(s,e) includes an arc in Arcs_s'/w(s,e).\n"
		"\n"
		"for each w, (s,e):\n"
		"    for each s' keep only the best scoring path in Pathes_s'/w(s,e)\n"
		"-> classical epsilon-removal over the tropical semiring with statewise\n"
		"   determinization\n"
		"\n"
		"The resulting graph is a subgraph of the original input and contains the\n"
		"Viterbi path of the original graph.\n"
		"The implementation is static, i.e not lazy.",
		"[*.network.non-word-closure-weak-determinization-filter]\n"
		"type                        = non-word-closure-weak-determinization-filter",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createNonWordClosureWeakDeterminizationFilterNode));
	factory->add(
	    NodeCreator(
		"non-word-closure-strong-determinization-filter",
		"Given states s and e. Pathes_w(s,e) is the set of all pathes from\n"
		"s to e having exactly one arc labeled with w and all others labeled\n"
		"with epsilon. Arcs_w(s,e) is the set of all arcs in Pathes_w(s,e) \n"
		"labeled with w. Arcs_s'/w(s,e) is the set of all arcs in Arcs_w(s,e)\n"
		"having source state s'. Pathes_s'/w(s,e) is the subset of Pathes_w(s,e),\n"
		"such that each path in Pathes_s'/w(s,e) includes an arc in Arcs_s'/w(s,e).\n"
		"\n"
		"for each w, (s,e):\n"
		"    keep only the best scoring path in Pathes_w(s,e)\n"
		"-> classical epsilon-removal over the tropical semiring with\n"
		"   determinization over all pathes from s to e\n"
		"\n"
		"Attention: \n"
		"Due to the retaining of non-word arcs the determinization can not\n"
		"always be guaranteed.\n"
		"\n"
		"The resulting graph is a subgraph of the original input and contains the\n"
		"Viterbi path of the original graph.\n"
		"The implementation is static, i.e not lazy.",
		"[*.network.non-word-closure-strong-determinization-filter]\n"
		"type                        = non-word-closure-strong-determinization-filter",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createNonWordClosureStrongDeterminizationFilterNode));
	factory->add(
	    NodeCreator(
		"non-word-closure-normalization-filter",
		"If a state s has at least one outgoing arc, and all outgoing arcs\n"
		"are non-word arcs, then s is disacarded and all outgoing arcs are\n"
		"joined with previous/next non-word arcs to a new eps-arc. All scores\n"
		"and word-arc times are kept w.r.t. to the given semiring.\n",
		"[*.network.non-word-closure-normalization-filter]\n"
		"type                        = non-word-closure-normalization-filter",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createNonWordClosureNormalizationFilterNode));
	factory->add(
	    NodeCreator(
		"non-word-closure-removal-filter",
		"For each state s and each word arc a leaving a state of the\n"
		"non-word closure of s, let a start from s, attach the correct\n"
		"score w.r.t to the used semiring (e.g. score of best path for\n"
		"the tropical semiring) and add the additional time (i.e. the\n"
		"time nedded for \"crossing\" the closure.",
		"[*.network.non-word-closure-removal-filter]\n"
		"type                        = non-word-closure-removal-filter",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createNonWordClosureRemovalFilterNode));


	/*
	  Miscellaneous
	*/
	factory->add(
	    NodeCreator(
		"dummy",
		"If it gets input at port 0, it behaves like a filter,\n"
		"but passes lattices just through.\n"
		"Else it does nothing, ignoring any input from other ports."
		"input:\n"
		"  0:lattice or no input\n"
		"output:\n"
		"  0:lattice, if input at port 0",
		&createDummyFilterNode));
	factory->add(
	    NodeCreator(
		"map-alphabet",
		"Map the input(output, or both) labels of the incoming lattice to another\n"
		"alphabet. The concrete mapping is specified by the used lexicon.\n"
		"If the incoming lattice is an acceptor and output mapping is activated,\n"
		"the resulting lattice is a transducer.\n"
		"For lemma-pronunciation <-> lemma correct time boundary preservation\n"
		"is guaranteed, for all other mappings it is not.\n"
		"For lemma -> preferred-lemma-pronunciation a successfull mapping is\n"
		"guaranteed, if the lexicon's read-only flag is not set, i.e. for a\n"
		"lemma with no pronunciation, the empty pronunciation is added.\n"
		"If project input(output) is activated, the resulting lattice is an\n"
		"acceptor, where the labels are the former input(output) labels.\n"
		"If invert is activated and the lattice is a transducer, input and\n"
		"output labels are toggled.\n"
		"All mappings have a lazy implementation.",
		"[*.network.map-alphabet]\n"
		"type                        = map-alphabet\n"
		"map-input                   = to-phoneme|to-lemma|to-lemma-pron|to-preferred-lemma-pron|to-synt|to-eval|to-preferred-eval\n"
		"map-output                  = to-phoneme|to-lemma|to-lemma-pron|to-preferred-lemma-pron|to-synt|to-eval|to-preferred-eval\n"
		"project-input               = false\n"
		"project-output              = false\n"
		"invert                      = false",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createAlphabetMapNode));
	factory->add(
	    NodeCreator(
		"map-labels",
		"Map the input labels of the incoming lattice according to the\n"
		"specified mappings:\n"
		"- non-words, i.e. words having the empty eval. tok. seq., to epsilon\n"
		"- compound word splitting, i.e. split at \" \", \"_\", or \"-\"\n"
		"- static mapping, where the mappings are loaded from a file; the\n"
		"  format is \"<source-word> <target-word-1> <target-word-2> ...\\n\n"
		"All mappings preserve or interpolate time boundaries, all mappings\n"
		"have a static implementation.",
		"[*.network.map-labels]\n"
		"type                        = map-labels\n"
		"map-to-lower-case           = false\n"
		"map-non-words-to-eps        = false\n"
		"split-compound-words        = false\n"
		"map.file                    = \n"
		"map.encoding                = utf-8\n"
		"map.from                    = lemma\n"
		"map.to                      = lemma\n"
		"project-input               = false",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createLabelMapNode));
	factory->add(
	    NodeCreator(
		"filter",
		"Filter lattice by input(output).",
		"[*.network.filter]\n"
		"type                        = filter\n"
		"input                       = <unset>\n"
		"output                      = <unset>",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createFilterArcNode));
	factory->add(
	    NodeCreator(
		"unite",
		"Build union of incoming lattices.\n"
		"Incoming lattices need to have\n"
		"- same alphabets and\n"
		"- same semiring\n"
		"  or a new semiring is defined.",
		"[*.network.unite]\n"
		"type                        = unite\n"
		"[*.network.unite.semiring]\n"
		"type                        = tropical|log\n"
		"tolerance                   = <default-tolerance>\n"
		"keys                        = key1 key2 ...\n"
		"key1.scale                  = <f32>\n"
		"key2.scale                  = <f32>\n"
		"...",
		"input:\n"
		"  0:lattice [1:lattice [2:lattice ...]]\n"
		"output:\n"
		"  0:lattice",
		&createUnionNode));
	factory->add(
	    NodeCreator(
		"mesh",
		"Reducde lattice to its boundary-conditioned form:\n"
		"either using the full boundary information or only\n"
		"the time information, i.e. building the purely\n"
		"time-conditioned form.",
		"[*.network.mesh]\n"
		"type                        = mesh",
		"mesh-type                   = full*|time\n"
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createMeshNode));
	factory->add(
	    NodeCreator(
		"properties",
		"Change and/or dump lattice and fsa properties.",
		"[*.network.properties]\n"
		"type                        = properties\n"
		"dump                        = true|false",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createPropertiesNode));

	/**
	 * Fwd/Bwd scores
	 **/
	factory->add(
	    NodeCreator(
		"FB-builder",
		"Build Fwd./Bwd. scores from incoming lattice(s).\n"
		"In the case of multiple incoming lattices, the result\n"
		"is the union of all incoming lattices.\n"
		"There are some major differences between doing single or multiple\n"
		"lattice FB:\n"
		"Single lattice:\n"
		"  The semiring from the incoming lattice is preserved;\n"
		"  all dimensions used need to be present in this semiring,\n"
		"  e.g. score.key.\n"
		"  The topology of the incoming and outgoing lattice are equal.\n"
		"  Risk calculation is available.\n"
		"Multiple lattices:\n"
		"  Union of all lattices are build.\n"
		"  The union lattice has a new semiring consisting either of\n"
		"  score.key only, or of the concatenated scores of the incoming lattices.\n"
		"  Optionally, a label identifying the source system is set as the output label\n"
		"  in the union lattice.",
		"[*.network.FB-builder]\n"
		"type                        = FB-builder\n"
		"[*.network.FB-builder.multi-lattice-algorithm]\n"
		"force                       = false\n"
		// "extend-keys                 = \n"
		"[*.network.FB-builder.fb]\n"
		"configuration.channel       = nil\n"
		"statistics.channel          = nil\n"
		"\n"
		"# single lattice FB\n"
		"score.key                   = <unset>\n"
		"risk.key                    = <unset>\n"
		"risk.normalize              = false\n"
		"cost.key                    = <unset> # required, if risk.key is specified\n"
		"# Default alpha is 1/<max scale>; alpha is ignored, if a\n"
		"# semiring is given (see below).\n"
		"alpha                       = <unset>\n"
		"# If a semiring is specified, then the number of dimensions\n"
		"# of old and new semiring must be equal.\n"
		"semiring.type               = <unset>*|tropical|log\n"
		"semiring.tolerance          = <default-tolerance>\n"
		"semiring.keys               = key1 key2 ...\n"
		"semiring.key1.scale         = <f32>\n"
		"semiring.key2.scale         = <f32>\n"
		"...\n"
		"\n"
		"# multiple lattice FB\n"
		"score-combination.type      = discard|*concatenate\n"
		"score.key                   = <unset>\n"
		"system-labels               = false\n"
		"set-posterior-semiring      = false\n"
		"[*.network.FB-builder.fb.lattice-0]\n"
		"weight                      = 1.0\n"
		"# Default alpha is 1/<max scale>; alpha is ignored, if a\n"
		"# semiring is given (see below).\n"
		"alpha                       = <unset>\n"
		"# If a semiring is specified, then the number of dimensions\n"
		"# of old and new semiring must be equal.\n"
		"semiring.type               = <unset>*|tropical|log\n"
		"semiring.tolerance          = <default-tolerance>\n"
		"semiring.keys               = key1 key2 ...\n"
		"semiring.key1.scale         = <f32>\n"
		"semiring.key2.scale         = <f32>\n"
		"...\n"
		"label                       = system-0\n"
		"# experimental\n"
		"norm.key                    = <unset>\n"
		"norm.fsa                    = false\n"
		"weight.key                  = <unset>\n"
		"[*.network.FB-builder.fb.lattice-1]\n"
		"...\n",
		"input:\n"
		"  0:lattice [1:lattice [...]]\n"
		"output:\n"
		"  0:lattice 1:FwdBwd",
		&createFwdBwdBuilderNode));

	/**
	 * Pruning
	 **/
	factory->add(
	    NodeCreator(
		"prune-posterior",
		"Prune arcs by posterior scores.\n"
		"By default, the fwd/bwd scores are calculated over the normalized\n"
		"log semiring derived from the lattice's semiring. Alternatively,\n"
		"a semiring can be specified.\n"
		"If the lattice is empty afte pruning, the single best result is\n"
		"returned (only if trimming is activated).",
		"[*.network.prune-posterior]\n"
		"type                        = prune-posterior\n"
		"configuration.channel       = nil\n"
		"statistics.channel          = nil\n"
		"trim                        = true\n"
		"# pruning parameters\n"
		"relative                    = true\n"
		"as-probability              = false\n"
		"threshold                   = inf\n"
		"...\n"
		"# parameter for fwd./bwd. calculation\n"
		"[*.network.prune-posterior.fb]\n"
		"see FB-builder ...\n",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createFwdBwdPruningNode));

	/**
	 * fCN
	 **/
	factory->add(
	    NodeCreator(
		"fCN-archive-reader",
		"Read posterior CNs, i.e. normally frame-wise CNs,\n"
		"from archive; the CN is buffered for multiple\n"
		"access.",
		"[*.network.fCN-archive-reader]\n"
		"type                        = fCN-archive-reader\n"
		"format                      = xml\n"
		"# xml format\n"
		"[*.network.fCN-archive-reader.archive]\n"
		"path                        = <archive-path>\n"
		"suffix                      = .<format>.fcn.gz\n"
		"encoding                    = utf-8",
		"input:\n"
		"  1:segment | 2:string\n"
		"output:\n"
		"  0:fCN",
		&createPosteriorCnArchiveReaderNode));

	factory->add(
	    NodeCreator(
		"fCN-archive-writer",
		"Store posterior CNs in archive.",
		"[*.network.fCN-archive-writer]\n"
		"type                        = fCN-archive-writer\n"
		"format                      = text|xml*|flow-alignment\n"
		"# text|xml format\n"
		"[*.network.fCN-archive-writer.archive]\n"
		"path                        = <unset>\n"
		"suffix                      = .<format>.fcn.gz\n"
		"encoding                    = utf-8\n"
		"# flow-alignment format\n"
		"[*.network.fCN-archive-writer.flow-cache]\n"
		"path                        = <unset>\n"
		"compress                    = false\n"
		"gather                      = inf\n"
		"cast                        = <unset>",
		"input:\n"
		"  0:fCN, 1:segment | 2:string\n"
		"output:\n"
		"  0:fCN",
		&createPosteriorCnArchiveWriterNode));

	factory->add(
	    NodeCreator(
		"dump-fCN",
		"Dump a textual representation of a\n"
		"frame wise confusion network (or any\n"
		"posterior CN).\n"
		"Slots are sorted by decreasing probability.\n"
		"At port 0 the lattice representation of the CN is\n"
		"provided. Port 1 provides the fCN itself and port 2\n"
		"provides an empty dummy lattice which can be connected\n"
		"to a sink.",
		"[*.network.dump-CN]\n"
		"type                        = dump-CN\n"
		"dump.channel                = nil\n"
		"format                      = text|xml*",
		"input:\n"
		"  0:fCN [1:segment]\n"
		"output:\n"
		"  0:lattice 1:fCN 2-n:dummy-lattice",
		&createDumpPosteriorCnNode));

	factory->add(
	    NodeCreator(
		"fCN-builder",
		"Build fCN from incoming lattice(s).\n"
		"First, the union of the lattices is builde and the\n"
		"weighted fwd/bwd scores of the union are calculated.\n"
		"Second, from the union the fCN is derived.",
		"[*.network.fCN-builder]\n"
		"type                        = fCN-builder\n"
		"[*.network.fCN-builder.fb]\n"
		"see FB-builder ...\n"
		"# Pruning is applied before fwd/bwd score calculation",
		"input:\n"
		"  0:lattice [1:lattice [...]]\n"
		"output:\n"
		"  0:lattice(union) 1:fCN 2:lattice(fCN)",
		&createFramePosteriorCnBuilderNode));

	factory->add(
	    NodeCreator(
		"fCN-gamma-correction",
		"Perform a in-situ gamma correction of the slot-wise\n"
		"posterior probability distribution.",
		"[*.network.fCN-gamma-correction]\n"
		"type                        = fCN-gamma-correction\n"
		"gamma                       = 1.0\n"
		"normalize                   = true",
		"input:\n"
		"  0:fCN\n"
		"output:\n"
		"  0:fCN",
		&createPosteriorCnGammaCorrectionNode));

	factory->add(
	    NodeCreator(
		"prune-fCN",
		"Prune fCN slotwise.\n"
		"If a threshold is given, probability mass pruning is\n"
		"applied, i.e. per slot only the first n entries having\n"
		"in sum the desired probability mass are kept.\n"
		"If the maximum slot size n is given, then at most n\n"
		"arcs are kept per slot.\n"
		"On request, the slot-wise probability distribution\n"
		"is re-normalized.\n"
		"If epsilon slot removal is activated, then all slots will\n"
		"be removed, where the posterior probability of the epsilon\n"
		"arc exceeds the threshold.\n"
		"Attention: In situ pruning is performed.",
		"[*.network.prune-fCN]\n"
		"type                        = prune-fCN\n"
		"threshold                   = <unset>\n"
		"max-slot-size               = <unset>\n"
		"normalize                   = true\n"
		"remove-eps-slots            = false\n"
		"eps-slot-removal.threshold  = 1.0",
		"input:\n"
		"  x:fCN\n"
		"output:\n"
		"  x:fCN",
		&createPosteriorCnPruningNode));

	factory->add(
	    NodeCreator(
		"fCN-combination",
		"Build joint fCN over all incoming fCNs\n"
		"by bulding the frame and word-wise joint probability.\n"
		"Optionally use the word-wise maximum approximation.",
		"[*.network.fCN-combination]\n"
		"type                        = fCN-combination\n"
		"weighting                   = static*|min-entropy|inverse-entropy\n"
		"fCN-0.weight                = 1.0\n"
		"...",
		"input:\n"
		"  0:fCN [1:fCN [...]]\n"
		"output:\n"
		"  0:lattice 1:fCN",
		&createFramePosteriorCnCombinationNode));

	factory->add(
	    NodeCreator(
		"fCN-features",
		"Take fCN from port 1, if provided, else build the\n"
		"frame-wise fCN either from the lattice provided at port 2\n"
		"or from the incoming lattice itself.\n"
		"A gamma != 1.0 performs a slot-wise gamma-correction on the\n"
		"frame-wise word posterior distributions.\n"
		"Per arc, set the value for a feature derived from the fCN\n"
		"to the corresponding dimension.\n"
		"Features:\n"
		"confidence: Frank-Wessel's confidence scores\n"
		"error:      smoothed, expected time frame error\n"
		"            alpha=0.0 -> unsmoothed error\n"
		"            fCN[t]=0.0|1.0 -> (smoothed) time frame error\n"
		"            Min.fWER-decoding: select the path with the\n"
		"            lowest error\n"
		"\"Accuracy/Error lattices:\n"
		"The calculation of arc-wise frame errors can be done by\n"
		"providing the reference as a linear lattice at port 2.\n"
		"Alternatively, a fCN or lattice storing the \"true\" frame-\n"
		"wise posterior distribution can be used.",
		"[*.network.fCN-features]\n"
		"type                        = fCN-features\n"
		"gamma                       = 1.0\n"
		"rescore-mode                = clone*|in-place-cached|in-place\n"
		"# features\n"
		"confidence-key              = <unset>\n"
		"error-key                   = <unset>\n"
		"error.alpha                 = 0.05\n"
		"[*.network.fCN-features.fb]\n"
		"see FB-builder ...",
		"input:\n"
		"  0:lattice [1:fCN] [2:lattice]\n"
		"output:\n"
		"  0:lattice",
		&createFramePosteriorCnFeatureNode));

	factory->add(
	    NodeCreator(
		"fCN-confidence",
		"Calculate word confidence using Frank Wessel's approach.\n"
		"Take fCN from port 1, if provided, else build the\n"
		"frame-wise fCN for the incoming lattice.",
		"[*.network.fCN-confidence]\n"
		"type                        = fCN-confidence\n"
		"gamma                       = 1.0\n"
		"append                      = false\n"
		"key                         = <symbolic key or dim>\n"
		"rescore-mode                = clone*|in-place-cached|in-place\n"
		"[*.network.fCN-confidence.fb]\n"
		"see FB-builder ...",
		"input:\n"
		"  0:lattice [1:fCN]\n"
		"output:\n"
		"  0:lattice",
		&createExtendByPosteriorCnConfidenceNode));
	factory->add(
	    NodeCreator(
		"add-word-confidence",
		"DEPRECATED: see \"fcn-confidence\" and/or \"fcn-features\"",
		"[*.network.add-word-confidence]\n"
		"type                        = add-word-confidence\n"
		"... see fCN-confidence",
		"input:\n"
		"  0:lattice [1:fCN]\n"
		"output:\n"
		"  0:lattice",
		&createExtendByPosteriorCnConfidenceNode));

	factory->add(
	    NodeCreator(
		"fWER-evaluator",
		"Calculate smoothed and unsmoothed (expected) time frame error.\n"
		"Hypothesis and reference lattice must be linear. Alternatively,\n"
		"an fCN can be provided as reference allowing to calculate an\n"
		"expected fWER; see min.fWER-decoding.",
		"[*.network.fWER-evaluator]\n"
		"type                        = fWER-evaluator\n"
		"dump.channel                = <unset>\n"
		"alpha                       = 0.05",
		"input:\n"
		"  0:lattice 1:reference-lattice|2:reference-fCN\n"
		"output:\n"
		"  0:lattice",
		&createTimeframeErrorNode));

	factory->add(
	    NodeCreator(
		"min-fWER-decoder",
		"Decode over all incoming lattices.\n"
		"Search space:\n"
		"union: Decode over union of all lattices.\n"
		"mesh:  Decode over time-conditioned lattice build\n"
		"       build from the union of all lattices.\n"
		"cn:    Decode from fCN directly, unrestriced\n"
		"       search space\n"
		"If no fCN is provided at port 0, then\n"
		"a fCN is calculated over all incoming lattices.",
		"[*.network.min-fWER-decoder]\n"
		"type                        = min-fWER-decoder\n"
		"search-space                = union|mesh*|cn\n"
		"[*.network.min-fWER-decoder.union]\n"
		"alpha                       = 0.05\n"
		"non-word-alpha              = 0.05\n"
		"confidence-key              = <unset>\n"
		"[*.network.min-fWER-decoder.mesh]\n"
		"alpha                       = 0.05\n"
		"non-word-alpha              = 0.05\n"
		"confidence-key              = <unset>\n"
		"[*.network.min-fWER-decoder.cn]\n"
		"word-penalty                = 2.5"
		"# fwd/bwd scores are used for calculating fCN, if not specified\n"
		"# and for applying fwd/bwd pruning\n"
		"[*.network.min-fWER-decoder.fb]\n"
		"see FB-builder ...",
		"input:\n"
		"  [0:fCN] 1:lattice [2:lattice [...]]\n"
		"output:\n"
		"  0:lattice",
		&createMinimumFrameWerDecoderNode));

	/**
	 * CN
	 **/
	factory->add(
	    NodeCreator(
		"CN-archive-reader",
		"Read CNs from archive;\n"
		"the CN is buffered for multiple access.",
		"[*.network.CN-archive-reader]\n"
		"type                        = CN-archive-reader\n"
		"format                      = xml\n"
		"path                        = <archive-path>\n"
		"suffix                      = .<format>.cn.gz\n"
		"encoding                    = utf-8",
		"input:\n"
		"  1:segment | 2:string\n"
		"output:\n"
		"  0:CN",
		&createConfusionNetworkArchiveReaderNode));

	factory->add(
	    NodeCreator(
		"CN-archive-writer",
		"Store CNs in archive.",
		"[*.network.cn-archive-writer]\n"
		"type                        = CN-archive-writer\n"
		"format                      = text|xml*\n"
		"path                        = <archive-path>\n"
		"archive.suffix              = .<format>.cn.gz\n"
		"archive.encoding            = utf-8",
		"input:\n"
		"  0:CN, 1:segment | 2:string\n"
		"output:\n"
		"  0:CN",
		&createConfusionNetworkArchiveWriterNode));

	factory->add(
	    NodeCreator(
		"dump-CN",
		"Dump a textual representation of a\n"
		"confusion network.\n"
		"At port 0 the lattice representation of the CN is\n"
		"provided. Port 1 provides the CN itself and port 2\n"
		"provides an empty dummy lattice which can be connected\n"
		"to a sink.",
		"[*.network.dump-CN]\n"
		"type                        = dump-CN\n"
		"dump.channel                = nil\n"
		"format                      = text|xml*",
		"input:\n"
		"  0:CN [1:segment]\n"
		"output:\n"
		"  0:lattice 1:CN 2-n:dummy-lattice",
		&createDumpConfusionNetworkNode));

	factory->add(
	    NodeCreator(
		"state-cluster-CN-builder",
		"Build CN from incoming lattice(s).\n"
		"The algorithm builds state clusters first and deduces\n"
		"from them arc clusters.\n"
		"Setting map=true stores a lattice <-> CN mapping, which is\n"
		"required for producing CN based lattice features.\n"
		"The algorithm is a little picky w.r.t. to the structure of\n"
		"the incoming lattice; try remove-null-arcs(Remark: this is\n"
		"only a hack, better someone fixes this in general!)",
		"[*.network.state-cluster-CN-builder]\n"
		"type                        = cluster-CN-builder\n"
		"statistics.channel          = nil\n"
		"confidence-key              = <unset>\n"
		"map                         = false\n"
		"remove-null-arcs            = false\n"
		"allow-bwd-match             = false\n"
		"[*.network.state-cluster-CN-builder.fb]\n"
		"see FB-builder ...",
		"input:\n"
		"   0:lattice [1:lattice [...]]\n"
		"output:\n"
		"                      0:lattice(best)\n"
		"   1:CN(normalized)   2:lattice(normalized CN)\n"
		"   3:CN               4:lattice(CN)\n"
		"                      6:lattice(state cluster)",
		&createStateClusterCnBuilderNode));
	factory->add(
	    NodeCreator(
		"cluster-CN-builder",
		"DEPRECATED: see \"state-cluster-CN-builder\"",
		"",
		"input:\n"
		"   0:lattice [1:lattice [...]]\n"
		"output:\n"
		"                      0:lattice(best)\n"
		"   1:CN(normalized)   2:lattice(normalized CN)\n"
		"   3:CN               4:lattice(CN)\n"
		"                      6:lattice(state cluster)",
		&createStateClusterCnBuilderNode));

	factory->add(
	    NodeCreator(
		"pivot-arc-CN-builder",
		"Build CN from incoming lattice(s).\n"
		"The pivot elements are the arcs form the lattice path with the\n"
		"maximum a posterior probability, i.e. lowest fwd/bwd score.\n"
		"Setting map=true stores a lattice <-> CN mapping, which is\n"
		"required for producing CN based lattice features.",
		"[*.network.pivot-arc-CN-builder]\n"
		"type                        = pivot-arc-CN-builder\n"
		"statistics.channel          = nil\n"
		"confidence-key              = <unset>\n"
		"map                         = false\n"
		"distance                    = weighted-time*|weighted-pivot-time\n"
		"[*.network.pivot-arc-CN-builder.weighted-time]\n"
		"posterior-impact            = 0.1\n"
		"edit-distance               = false\n"
		"[*.network.pivot-arc-CN-builder.weighted-pivot-time]\n"
		"posterior-impact            = 0.1\n"
		"edit-distance               = false\n"
		"fast                        = false\n"
		"[*.network.pivot-arc-CN-builder.fb]\n"
		"see FB-builder ...",
		"input:\n"
		"   0:lattice [1:lattice [...]]\n"
		"output:\n"
		"output:\n"
		"                      0:lattice(best)\n"
		"   1:CN(normalized)   2:lattice(normalized CN)\n"
		"   3:CN               4:lattice(CN)\n"
		"                      6:lattice(union)",
		&createPivotArcCnBuilderNode));
	factory->add(
	    NodeCreator(
		"pivot-CN-builder",
		"DEPRECATED: see \"pivot-arc-CN-builder\"",
		"",
		"input:\n"
		"   0:lattice [1:lattice [...]]\n"
		"output:\n"
		"output:\n"
		"                      0:lattice(best)\n"
		"   1:CN(normalized)   2:lattice(normalized CN)\n"
		"   3:CN               4:lattice(CN)\n"
		"                      6:lattice(union)",
		&createPivotArcCnBuilderNode));

	factory->add(
	    NodeCreator(
		"center-frame-CN-builder",
		"Build CN from incoming lattice(s).\n"
		"The algorithm is based on finding an example or prototype frame for each word.",
		"[*.network.center-frame-CN-builder]\n"
		"type                        = frame-CN-builder\n"
		"statistics.channel          = nil\n"
		"confidence-key              = <unset>\n"
		"map                         = false\n"
		"[*.network.center-frame-CN-builder.fb]\n"
		"see FB-builder ...",
		"input:\n"
		"   0:lattice [1:lattice [...]]\n"
		"output:\n"
		"output:\n"
		"                      0:lattice(best)\n"
		"   1:CN(normalized)   2:lattice(normalized CN)\n"
		"   3:CN               4:lattice(CN)\n"
		"   5:fCN              6:lattice(union)",
		&createCenterFrameCnBuilderNode));
	factory->add(
	    NodeCreator(
		"frame-CN-builder",
		"DEPRECATED: see \"center-frame-CN-builder\"",
		"",
		"input:\n"
		"   0:lattice [1:lattice [...]]\n"
		"output:\n"
		"output:\n"
		"                      0:lattice(best)\n"
		"   1:CN(normalized)   2:lattice(normalized CN)\n"
		"   3:CN               4:lattice(CN)\n"
		"   5:fCN              6:lattice(union)",
		&createCenterFrameCnBuilderNode));

	factory->add(
	    NodeCreator(
		"CN-gamma-correction",
		"Perform a in-situ gamma correction of the slot-wise\n"
		"posterior probability distribution.\n"
		"The CN must be normalized.",
		"[*.network.CN-gamma-correction]\n"
		"type                        = CN-gamma-correction\n"
		"gamma                       = 1.0\n"
		"normalize                   = true",
		"input:\n"
		"  0:CN(normalized)\n"
		"output:\n"
		"  0:CN",
		&createNormalizedCnGammaCorrectionNode));

	factory->add(
	    NodeCreator(
		"prune-CN",
		"Prune CN slotwise; CN must be normalized.\n"
		"If a threshold is given, probability mass pruning is\n"
		"applied, i.e. per slot only the first n entries having\n"
		"in sum the desired probability mass are kept.\n"
		"If the maximum slot size n is given, then at most n\n"
		"arcs are kept per slot.\n"
		"On request, the slot-wise probability distribution\n"
		"is re-normalized.\n"
		"If epsilon slot removal is activated, then all slots will\n"
		"be removed, where the posterior probability of the epsilon\n"
		"arc exceeds the threshold.\n"
		"Attention: In situ pruning is performed.",
		"[*.network.prune-CN]\n"
		"type                        = prune-CN\n"
		"threshold                   = <unset>\n"
		"max-slot-size               = <unset>\n"
		"normalize                   = true\n"
		"remove-eps-slots            = false\n"
		"eps-slot-removal.threshold  = 1.0",
		"input:\n"
		"  x:CN\n"
		"output:\n"
		"  x:CN",
		&createNormalizedCnPruningNode));

	factory->add(
	    NodeCreator(
		"CN-combination",
		"Combine and decode incoming posterior CNs.",
		"[*.network.CN-combination]\n"
		"type                        = CN-combination\n"
		"cost                        = expected-loss|expected-error*\n"
		"posterior-key               = confidence\n"
		"score-combination.type      = discard|*concatenate\n"
		"beam-width                  = 100\n"
		"cn-0.weight                 = 1.0\n"
		"cn-0.posterior-key          = <unset>\n"
		"...",
		"input:\n"
		"  0:normalized-CN [1:normalized-CN [...]]\n"
		"output:\n"
		"  0:top-best-lattice 1:normalized-CN 2:normalized-CN-lattice",
		&createConfusionNetworkCombinationNode));

	factory->add(
	    NodeCreator(
		"ROVER-combination",
		"Combine and decode incoming lattices.",
		"[*.network.ROVER-combination]\n"
		"type                        = ROVER-combination\n"
		"cost                        = sclite-word-cost|*sclite-time-mediated-cost\n"
		"null-word                   = @\n"
		"null-confidence             = 0.7\n"
		"alpha                       = 0.0\n"
		"posterior-key               = confidence\n"
		"score-combination.type      = discard|*concatenate\n"
		"beam-width                  = 100\n"
		"lattice-0.weight            = 1.0\n"
		"lattice-0.confidence-key    = <unset>\n"
		"...",
		"input:\n"
		"  0:lattice [1:lattice [...]]\n"
		"output:\n"
		"  0:top-best-lattice 1:normalized-CN 2:normalized-CN-lattice 3:n-best-CN 4:n-best-CN-lattice",
		&createRoverCombinationNode));

	factory->add(
	    NodeCreator(
		"oracle-alignment",
		"Compute oracle alignment between CN and reference.\n"
		"The oracle loss requires a posterior score, i.e.\n"
		"Cost functions:\n"
		"- oracle-error\n"
		"    0, if word in slot\n"
		"    1, else\n"
		"- weighted-oracle-error\n"
		"    i**alpha, where\n"
		"    i is the position of the reference word in the slot,\n"
		"    resp. 100, if the reference word is not in the slot\n"
		"- oracle-loss\n"
		"    1 - p(word|slot), if word in slot\n"
		"    100, else,\n"
		"    i.e. align w.r.t to minimum oracle error as primary criterion\n"
		"    and minimum expected error as secondary criterion\n"
		"either a normalized CN or posterior key defined.",
		"[*.network.oracle-alignment]\n"
		"type                        = oracle-alignment\n"
		"cost                        = oracle-cost*|weighted-oracle-cost|oracle-loss\n"
		"weighted-oracle-cost.alpha  = 1.0\n"
		"posterior-key               = <unset>\n"
		"beam-width                  = 100",
		"input:\n"
		"  0:CN 1:lattice|2:string|3:CN|4:segment(with orthography)\n"
		"output:\n"
		"  0:oracle-CN",
		&createOracleAlignmentNode));

	factory->add(
	    NodeCreator(
		"CN-features",
		"WARNING: beta status\n"
		"Per arc, set the value for a feature derived from the CN to\n"
		"the corresponding dimension.\n"
		"Features:\n"
		"- confidence:    slot based confidence\n"
		"- score:         negative logarithm of confidence\n"
		"- cost:          oracle alignment based cost;\n"
		"                 0, if oracle label equals arc label, 1, else\n"
		"- oracle-output: store oracle alignment as output label\n"
		"- entropy:       entropy of normalized slot\n"
		"- slot:          number of the slot the lattice arc falls into\n"
		"- non-eps-slot:  Same as \"slot\", but slots containing only epsilon arcs\n"
		"                 are ignored; epsilon arcs do not get this feature.\n"
		"                 If the threshold is < 1.0, then all slots with an\n"
		"                 epsilon mass >= threshold are ignored; the input of\n"
		"                 lattice arcs pointing at these slots are set to epsilon.\n"
		"Attention: confidence, score, and entropy feature require the\n"
		"            defintion of \"cn.posterior-key\".",
		"[*.network.CN-features]\n"
		"type                        = CN-features\n"
		"compose                     = false\n"
		"duplicate-output            = false\n"
		"# features\n"
		"confidence.key              = <unset>\n"
		"score.key                   = <unset>\n"
		"cost.key                    = <unset>\n"
		"oracle-output               = false\n"
		"entropy.key                 = <unset>\n"
		"slot.key                    = <unset>\n"
		"non-eps-slot.key            = <unset>\n"
		"non-eps-slot.threshold      = 1.0\n"
		"[*.network.CN-features.cn]\n"
		"posterior-key               = <unset>",
		"input:\n"
		"  0:lattice 1:CN\n"
		"output:\n"
		"  0:lattice",
		&createCnFeatureNode));

	factory->add(
	    NodeCreator(
		"CN-decoder",
		"Decode incoming CN, where the CN is provided at port 0 or\n"
		"alternatively a lattice with sausage topology at port 1.\n"
		"The posterior key defines the dimension of the semiring which\n"
		"provides a word-wise probability distribution per slot and\n"
		"is to be used for slot-wise decoding.",
		"[*.network.CN-decoder]\n"
		"type                        = CN-decoder\n"
		"posterior-key               = <unset>",
		"input:\n"
		"  0:CN | 1:sausage-lattice\n"
		"output:\n"
		"  0:best-lattice 1:sausage-lattice",
		&createCnDecoderNode));


	/**
	 * local cost decoder
	 **/
	factory->add(
	    NodeCreator(
		"local-cost-decoder",
		"Computes an arc-wise score comprised of a\n"
		"word penalty and an approximated risk.\n"
		"The approximated risk is based on the\n"
		"time overlap of hypothesis and reference\n"
		"arc, e.g. ",
		"[*.network.local-cost-decoder]\n"
		"type                        = approximated-risk-scorer\n"
		"rescore-mode                = clone\n"
		"score-key                   = <unset>\n"
		"confidence-key              = <unset>\n"
		"word-penalty                = 0.0\n"
		"search-space                = union|mesh*\n"
		"risk-builder                = overlap*|local-alignment\n"
		"[*.network.local-cost-decoder.overlap]\n"
		"scorer                      = path-symetric*|arc-symetric\n"
		"path-symetric.alpha         = 0.5 # [0.0,1.0]\n"
		"[*.network.local-cost-decoder.local-alignment]\n"
		"scorer                      = approximated-accuracy|continous-cost1|continous-cost2*|discrete-cost\n"
		"continous-cost1.alpha       = 1.0 # [0.0,1.0]\n"
		"continous-cost2.alpha       = 0.5 # [0.0,0.5]\n"
		"discrete-cost.alpha         = 0.5 # [0.0,0.5]\n"
		"[*.network.local-cost-decoder.fb]\n"
		"see FB-builder ...",
		"input:\n"
		"   0:lattice [1:lattice [...]]\n"
		"output:\n"
		"   0:lattice(best) 1:lattice(rescored)",
		&createLocalCostDecoderNode));
	factory->add(
	    NodeCreator(
		"approximated-risk-scorer",
		"DEPRECATED: see \"local-cost-decoder\"",
		"",
		"input:\n"
		"   0:lattice [1:lattice [...]]\n"
		"output:\n"
		"   0:lattice(best) 1:lattice(rescored)",
		&createLocalCostDecoderNode));


	/**
	 * Time alignment
	**/
	factory->add(
	    NodeCreator(
		"aligner",
		"Align a linear hypothesis against a reference lattice or\n"
		"a reference fCN.\n"
		"The algorithm works as follows:\n"
		"1) try intersection with reference lattice,\n"
		"   if intersection is empty then\n"
		"2) align against reference fCN\n"
		"If a reference fCN is required and a connection at port 1 exist,\n"
		"the reference fCN is taken from port 1, else the fCN is\n"
		"calculated from the lattice at port 2.\n"
		"If intersection is false, step 1) is skipped.",
		"[*.network.aligner]\n"
		"type                        = aligner\n"
		"intersection                = true\n"
		"[*.network.aligner.fb]\n"
		"see FB-builder ...",
		"input:\n"
		"  0:hypothesis-lattice {1:reference-fCN | 2:reference-lattice}\n"
		"output:\n"
		"  0:aligned-lattice",
		&createTimeAlignmentNode));

	factory->add(
	    NodeCreator(
		"dump-vocab",
		"Extracts and dumps all words occuring at least once\n"
		"as input token in a lattice.",
		"[*.network.dump-vocab]\n"
		"type                        = dump-vocab\n"
		"dump.channel                = <file>",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createWordListExtractorNode));

	factory->add(
	    NodeCreator(
		"recognizer",
		"The Sprint Recognizer.\n"
		"Output are linear or full lattices in Flf format.\n"
		"The most common operations on recognizer output can be directly\n"
		"performed by the node (in the given order):\n"
		"1. apply non-word-closure filter\n"
		"2. confidence score calculation\n"
		"3. posterior pruning\n"
		"If lattices are provided at port 0, the search-space is restricted\n"
		"to the lattice, i.e. the lattice is used as language model.\n"
		"The parameter \"grammar-key\" allows to choose a dimension that\n"
		"provides the lm-score, otherwise the projection defined by the\n"
		"semiring is used.",
		"[*.network.recognizer]\n"
		"type                        = recognizer\n"
		"grammar.key                 = <unset>\n"
		"grammar.arcs-limit          = <unset>\n"
		"grammar.log.channel         = <unset>\n"
		"<all parameters belonging to the search configuration>\n"
		"add-pronunciation-score     = false\n"
		"add-confidence-score        = false\n"
		"apply-non-word-closure-filter= false\n"
		"apply-posterior-pruning     = false\n"
		"posterior-pruning.threshold = 200\n"
		"fb.alpha                    = <1/lm-scale>",
		"input:\n"
		"  [0:lattice] 1:bliss-speech-segment\n"
		"output:\n"
		"  0:lattice",
		&createRecognizerNode));

	factory->add(
	    NodeCreator(
		"fit",
		"Fit lattice into segment boundaries.\n"
		"The fitted lattice has the following properties:\n"
		"- single initial state (id=0) s_i and single final state s_f (id=1)\n"
		"- weigth of the final state s_f is semiring one\n"
		"- 0 = time(s_i) <= time(s) < time(s_f)\n"
		"- for each path in the original lattice, there exist a\n"
		"  a path in the fitted lattice with the same score (w.r.t\n"
		"  to the used semiring); and vice versa\n"
		"- optional: each arc ending in s_f has </s>-label\n"
		"The bounding box is given by the segment provided at port 1.\n"
		"If no segment is provided, start time is 0 and end time is\n"
		"is the max. time of all states in the lattice.\n"
		"Remark: This node can be used to normalize the final states\n"
		"of a lattice.",
		"[*.network.fit]\n"
		"type                        = fit\n"
		"force-sentence-end-labels   = false",
		"input:\n"
		"  0:lattice [1:segment]\n"
		"output:\n"
		"  0:lattice [1:segment]",
		&createFitLatticeNode));

	factory->add(
	    NodeCreator(
		"string-to-lattice",
		"Convert a string to a linear lattice.",
		"[*.network.string-to-lattice]\n"
		"type                        = string-to-lattice\n"
		"alphabet                    = lemma-pronunciation|lemma|syntax|evaluation\n"
		"[*.network.string-to-lattice.semiring]\n"
		"type                        = tropical|log\n"
		"tolerance                   = <default-tolerance>\n"
		"keys                        = key1 key2 ...\n"
		"key1.scale                  = <f32>\n"
		"key2.scale                  = <f32>\n"
		"...",
		"input:\n"
		"  0:string\n"
		"output:\n"
		"  0:lattice",
		&createStringConverterNode));

	factory->add(
	    NodeCreator(
		"concatenate-lattices",
		"Concatenate all segments corresponding to the same recording:\n"
		"At port 1 a list of segments has to be provided, where each\n"
		"segment defines uniquely a recording.\n"
		"At port 0 a list of segments has to be provided with arbitrary\n"
		"many segments per recording. The segments do not need to\n"
		"partitionate the recording: gaps and overlaps are allowed.\n"
		"At port 0 the concatenated lattice is provided. And at port 1\n"
		"the corresponding segment, i.e. the \"recording\"-segment that\n"
		"was provided at port 1.\n"
		"Attention:\n"
		"Nodes being providing segments to this node must NOT be\n"
		"connected to any other node.",
		"[*.network.concatenate-lattices]\n"
		"type                        = concatenate-lattices\n"
		"dump.channel                = <unset>\n"
		"see archive-reader",
		"input:\n"
		"  0:segment 1:segment\n"
		"output:\n"
		"  0:lattice 1:segment",
		&createConcatenateLatticesNode));

	factory->add(
	    NodeCreator(
		"concatenate-fCNs",
		"Concatenate all segments corresponding to the same recording:\n"
		"At port 1 a list of segments has to be provided, where each\n"
		"segment defines uniquely a recording.\n"
		"At port 0 a list of segments has to be provided with arbitrary\n"
		"many segments per recording. The segments do not need to\n"
		"partitionate the recording: gaps and overlaps are allowed.\n"
		"At port 0 the concatenated fCN is provided. And at port 1\n"
		"the corresponding segment, i.e. the \"recording\"-segment that\n"
		"was provided at port 1.\n"
		"Attention:\n"
		"Nodes being providing segments to this node must NOT be\n"
		"connected to any other node.",
		"[*.network.concatenate-fCNs]\n"
		"type                        = concatenate-fCNs\n"
		"dump.channel                = <unset>\n"
		"see fCN-archive-reader",
		"input:\n"
		"  0:segment 1:segment\n"
		"output:\n"
		"  0:fCN 1:segment",
		&createConcatenateFCnsNode));

	factory->add(
	    NodeCreator(
		"clean-up",
		"Clean up lattice. Arcs that\n"
		"- close a cycle\n"
		"- have an invalid label id\n"
		"- have an invalid or semiring-zero score in at least one dimension\n"
		"are discarded and the lattice is trimmed.\n"
		"Thus, the resulting lattice is guaranteed to be\n"
		"acyclic, trim, and zero-sum free.",
		"[*.network.clean-up]\n"
		"type                        = clean-up",
		"input:\n"
		"  0:lattice\n"
		"output:\n"
		"  0:lattice",
		&createCleanUpNode));



    } // registerNodeCreators

} // namespace

#endif //_FLF_NODE_REGISTRATION_HH
