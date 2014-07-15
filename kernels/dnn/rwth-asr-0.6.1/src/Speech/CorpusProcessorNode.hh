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
#ifndef _SPEECH_CORPUS_PROCESSOR_NODE_HH
#define _SPEECH_CORPUS_PROCESSOR_NODE_HH

#include <Core/Types.hh>
#include <Flow/Node.hh>
#include "CorpusVisitor.hh"


namespace Speech {

    /** CorpusProcessorNode: base class for nodes implementing a whole pass over a corpus
     *
     * This class derives from Flow::Node and from an arbirtary CorpusProcessor
     * The node generates output objects by passing processing corpus file: data is extracted from the
     * dataSource (Flow::Network) and it is processed by the CorpusProcessor base class.
     * The generated objects are sent the the next node in the network.
     *
     */

    template<class CorpusProcessorType>
    class CorpusProcessorNode :
	public virtual Flow::Node,
	public CorpusProcessorType
    {
    private:

	typedef CorpusProcessorType CorpusProcessorPrecursor;

    private:

	const std::string dataSourceSelection;
	const std::string corpusSelection;

	Flow::Network dataSource_;

    private:

	bool dataSourceParameterName(std::string &name) {

	    if (name.find(dataSourceSelection + ".") == 0) {

		name = name.substr(dataSourceSelection.size() + 1);
		return true;
	    }
	    return false;
	}

	virtual bool work(Flow::PortId p) {

	    Speech::CorpusVisitor corpusVisitor(CorpusProcessorPrecursor::config, dataSource_, *this);
	    Bliss::CorpusDescription corpusDescription(CorpusProcessorPrecursor::select(corpusSelection));

	    corpusDescription.accept(&corpusVisitor);

	    return putData(p);
	}

    protected:

	/** putData: callback function to put the output into the network.
	 *
	 * It is called after the CorpusVisitor object has finished the corpus.
	 * Use the base functionality of Flow:Node to put the results of the processing into the network
	 *
	 * @param portId is the port on which a data object was requested
	 */

	virtual bool putData(Flow::PortId portId) = 0;

    public:

	CorpusProcessorNode(const Core::Configuration &c) :
	    Core::Component(c),
	    Flow::Node(c),
	    CorpusProcessorPrecursor(c),
	    dataSourceSelection("data-source"),
	    corpusSelection("corpus"),
	    dataSource_(Flow::Node::select(dataSourceSelection))
	{
	    dataSource_.respondToDelayedErrors();
	}


	~CorpusProcessorNode() {}

	/** setParameter: passes parameters to the underlying dataSource network
	 *
	 * Network parameters of the main network are passed to the underlying dataSource network.
	 * E.g.: <node name="corpus-processor-1" filter="speech-corpus-processor"
	 *             data-source.process-time="$(time)">
	 * The example node corpus-processor-1 hands on the value of its network parameter $(time) to the underlying
	 * dataSource network under the name "process-time".
	 *
	 * Parameters starting with "data-source." are only handed on to the underlying network.
	 *
	 * Remark:
	 *  do not forget to call this function from the derived classes
	 */

	virtual bool setParameter(const std::string &name, const std::string &value) {

	    std::string n(name);

	    if (dataSourceParameterName(n))
		return dataSource_.setParameter(n, value);
	    else
		return false;

	    return true;
	}
    };

} // namespace Speech

#endif // _SPEECH_CORPUS_PROCESSOR_NODE_HH
