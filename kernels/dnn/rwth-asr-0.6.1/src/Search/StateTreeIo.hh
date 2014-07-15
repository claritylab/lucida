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
#ifndef _SEACRH_STATE_TREE_IO_HH
#define _SEACRH_STATE_TREE_IO_HH

#include "StateTree.hh"
#include <Core/BinaryStream.hh>
#include <Core/Hash.hh>

namespace Search
{

    /**
     * State tree input / output functions.
     *
     * Cannot use global stream operators because reading / writing
     * some of the objects requires additional information from the
     * lexicon or the acoustic model.
     */
    class StateTreeIo
    {
    protected:
	typedef Core::BinaryInputStream Input;
	typedef Core::BinaryOutputStream Output;
	typedef u64 Position;
	typedef u32 ContainerSize;
    private:
	Bliss::LexiconRef lexicon_;
	Am::AcousticModelRef acousticModel_;

	typedef Am::AcousticModel::StateTransitionIndex StateTransitionIndex;
	typedef Core::hash_map<const Am::StateTransitionModel*, StateTransitionIndex,
			       Core::PointerHash<Am::StateTransitionModel> >
	TransitionModelToIndexMap;
	TransitionModelToIndexMap stateTransitionModelIndexes_;

    protected:
	void getDependencies();
	void getTreeDependencies(const StateTree &tree);
	static const std::string magic;
	static const int fileFormatVersion;
	static const std::string sectionDependencies, sectionStateTree;
	Core::DependencySet dependencies_;

    protected:
	template<class T>
	void read(Input &i, T &v) const {
	    i >> v;
	}

	template<class T>
	void write(Output &o, const T &v) const {
	    o << v;
	}

	template<class T, template<typename, typename> class Container >
	void read(Input &i, Container<T, std::allocator<T> > &c) const {
	    ContainerSize size;
	    read(i, size);
	    c.resize(size);
	    read(i, c.begin(), c.end());
	}

	template<class T, template<typename, typename> class Container>
	void write(Output &o, const Container<T, std::allocator<T> > c) const {
	    write(o, static_cast<ContainerSize>(std::distance(c.begin(), c.end())));
	    write(o, c.begin(), c.end());
	}


	template<class Iterator>
	void write(Output &o, Iterator begin, Iterator end) const {
	    for (; begin != end; ++begin)
		write(o, *begin);
	}

	template<class Iterator>
	void read(Input &i, Iterator begin, Iterator end) const {
	    for (; begin != end; ++begin)
		read(i, *begin);
	}

    public:
	StateTreeIo(Bliss::LexiconRef lexicon, Am::AcousticModelRef acousticModel);

    };

    template<>
    void StateTreeIo::read<StateTree::Exit>(Input &i, StateTree::Exit &exit) const;

    template<>
    void StateTreeIo::write<StateTree::Exit>(Output &o, const StateTree::Exit &exit) const;

    template<>
    void StateTreeIo::read<StateTree::StateDesc>(Input &i, StateTree::StateDesc &desc) const;

    template<>
    void StateTreeIo::write<StateTree::StateDesc>(Output &o, const StateTree::StateDesc &desc) const;

    template<>
    void StateTreeIo::read<StateTree::State>(Input &i, StateTree::State &state) const;

    template<>
    void StateTreeIo::write<StateTree::State>(Output &o, const StateTree::State &state) const;

    template<>
    void StateTreeIo::read<StateTree::CoarticulationStructure::PhonemePair>(
	Input &i, StateTree::CoarticulationStructure::PhonemePair &pair) const;

    template<>
    void StateTreeIo::write<StateTree::CoarticulationStructure::PhonemePair>(
	Output &o, const StateTree::CoarticulationStructure::PhonemePair &pair) const;


    /**
     * Writes a state tree to a binary file
     */
    class StateTreeWriter : public StateTreeIo
    {
    public:
	StateTreeWriter(Bliss::LexiconRef lexicon, Am::AcousticModelRef acousticModel);

	bool write(const StateTree &tree, const std::string &filename);
    protected:
	Position writeHeader(Core::BinaryOutputStream &) const;
	void writeDependenciesPosition(Core::BinaryOutputStream &, Position) const;
	bool writeDependencies(const std::string &filename, Position) const;
	bool writeTree(const StateTree &tree, Core::BinaryOutputStream &) const;
    };

    /**
     * Reconstructs a state tree from a file
     */
    class StateTreeReader : public StateTreeIo
    {
    public:
	StateTreeReader(Bliss::LexiconRef lexicon, Am::AcousticModelRef acousticModel)
	    : StateTreeIo(lexicon, acousticModel) {}

	bool read(StateTree &tree, const std::string &filename);
    protected:
	bool checkDependencies(const StateTree &tree, const std::string &filename, Position) const;
	bool checkHeader(Core::BinaryInputStream &in, Position &) const;
	bool readTree(StateTree &tree, Core::BinaryInputStream &in) const;
    };
}

#endif
