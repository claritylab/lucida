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
#include "StateTreeIo.hh"
#include <Am/ClassicAcousticModel.hh>

using namespace Search;

template<>
void StateTreeIo::read<StateTree::Exit>(Input &i, StateTree::Exit &exit) const
{
    s32 id;
    read(i, exit.transitEntry);
    read(i, id);
    if(id != Bliss::LemmaPronunciation::invalidId)
	exit.pronunciation = lexicon_->lemmaPronunciation(id);
    else
	exit.pronunciation = 0;
}

template<>
void StateTreeIo::write<StateTree::Exit>(Output &o, const StateTree::Exit &exit) const
{
    write(o, exit.transitEntry);
    write(o, exit.pronunciation ? exit.pronunciation->id() : Bliss::LemmaPronunciation::invalidId);
}

template<>
void StateTreeIo::read<StateTree::StateDesc>(Input &i, StateTree::StateDesc &desc) const
{
    read(i, desc.acousticModel);
    read(i, desc.transitionModelIndex);
}

template<>
void StateTreeIo::write<StateTree::StateDesc>(Output &o, const StateTree::StateDesc &desc) const
{
    write(o, desc.acousticModel);
    write(o, desc.transitionModelIndex);
}

template<>
void StateTreeIo::read<StateTree::State>(Input &i, StateTree::State &state) const
{
    read(i, state.desc);
    read(i, state.depth);
    read(i, state.exits);
    read(i, state.successors);
}

template<>
void StateTreeIo::write<StateTree::State>(Output &o, const StateTree::State &state) const
{
    write(o, state.desc);
    write(o, state.depth);
    write(o, state.exits);
    write(o, state.successors);
}

template<>
void StateTreeIo::read<StateTree::CoarticulationStructure::PhonemePair>(
    Input &i, StateTree::CoarticulationStructure::PhonemePair &pair) const
{
    read(i, pair.final);
    read(i, pair.initial);
}

template<>
void StateTreeIo::write<StateTree::CoarticulationStructure::PhonemePair>(
    Output &o, const StateTree::CoarticulationStructure::PhonemePair &pair) const
{
    write(o, pair.final);
    write(o, pair.initial);
}


const std::string StateTreeIo::magic = "SPRINT-ST";
const int StateTreeIo::fileFormatVersion = 5;

StateTreeIo::StateTreeIo(Bliss::LexiconRef lexicon, Am::AcousticModelRef acousticModel)
    : lexicon_(lexicon), acousticModel_(acousticModel)
{
    getDependencies();
}

void StateTreeIo::getDependencies()
{
    const Am::ClassicAcousticModel *am = required_cast(const Am::ClassicAcousticModel*, acousticModel_.get());
    Core::DependencySet d;
    am->stateModel()->hmmTopologySet().getDependencies(d);
    am->stateTying()->getDependencies(d);
    dependencies_.add("acoustic model", d);
    dependencies_.add("lexicon", lexicon_->getDependency());
    dependencies_.add("state-transitions", am->nStateTransitions());
}

void StateTreeIo::getTreeDependencies(const StateTree &tree)
{
    dependencies_.add("skip-transitions", tree.allowSkipTransitions_);
    dependencies_.add("ci-crossword-transitions", tree.allowCiCrossWordTransitions_);
    dependencies_.add("path-recombination-in-fan-in", tree.isPathRecombinationInFanInEnabled_);
}

StateTreeWriter::StateTreeWriter(Bliss::LexiconRef lexicon, Am::AcousticModelRef acousticModel)
    : StateTreeIo(lexicon, acousticModel)
{}


bool StateTreeWriter::write(const StateTree &tree, const std::string &filename)
{
    Core::BinaryOutputStream out(filename);
    if (!out.good()) {
	tree.warning("failed to open state tree file");
	return false;
    }

    Position pos = writeHeader(out);
    if (!writeTree(tree, out)) {
	tree.warning("failed to write tree");
	return false;
    }
    getTreeDependencies(tree);
    writeDependenciesPosition(out, pos);
    Position dependenciesPos = out.position();
    out.close();
    if (!writeDependencies(filename, dependenciesPos)) {
	tree.warning("failed to write dependencies");
	return false;
    }
    return true;
}

StateTreeIo::Position StateTreeWriter::writeHeader(Core::BinaryOutputStream &out) const
{
    StateTreeIo::write<std::string>(out, magic);
    StateTreeIo::write(out, fileFormatVersion);
    Position positionHole = out.position();
    // write current position as place holder
    // will be overwriten later by writeTreeSize
    StateTreeIo::write(out, positionHole);
    return positionHole;
}

/**
 * Write position of the dependencies to the file at position @c position.
 */
void StateTreeWriter::writeDependenciesPosition(Core::BinaryOutputStream &out, Position position) const
{
    Position dependenciesPosition = out.position();
    out.seek(position);
    StateTreeIo::write(out, dependenciesPosition);
    out.seek(dependenciesPosition);
}


bool StateTreeWriter::writeDependencies(const std::string &filename, Position position) const
{
    std::ofstream *ofs = new std::ofstream(filename.c_str(), std::ios::binary | std::ios::app);
    verify(ofs->good());
    ofs->seekp(position);
    Core::XmlOutputStream xmlOutput(ofs);
    if (!xmlOutput.good() || !dependencies_.write(xmlOutput)) {
	return false;
    }
    // ofs will be deleted internally by XmlOutputStream
    return true;
}

bool StateTreeWriter::writeTree(const StateTree &tree, Core::BinaryOutputStream &out) const
{
    verify(out.good());

    StateTreeIo::write(out, tree.states_);
    StateTreeIo::write(out, tree.root_);
    StateTreeIo::write(out, tree.ciRoot_);
    StateTreeIo::write(out, tree.batches_);
    StateTreeIo::write(out, tree.haveSuccessorBatches_);
    StateTreeIo::write(out, tree.successorBatches_);
    StateTreeIo::write(out, tree.emptyBatch_);
    StateTreeIo::write(out, tree.wordHeadsEnd_);
    bool haveCoarticulatedRoot = tree.coarticulationStructure_ != 0;
    StateTreeIo::write(out, haveCoarticulatedRoot);
    if (haveCoarticulatedRoot) {
	StateTreeIo::write(out, tree.coarticulationStructure_->initialPhonemes);
	StateTreeIo::write(out, tree.coarticulationStructure_->finalPhonemes);
	StateTreeIo::write(out, tree.coarticulationStructure_->roots);
	StateTreeIo::write(out, tree.coarticulationStructure_->boundaryPhonemes);
    }
    return true;
}




bool StateTreeReader::read(StateTree &tree, const std::string &filename)
{
    bool error = false;
    Core::BinaryInputStream in(filename);
    if (!in.isOpen()) {
	tree.log("no state tree file to open");
	return false;
    }
    Position dependencyPosition;
    if (!checkHeader(in, dependencyPosition)) {
	tree.warning("wrong file format in state tree file");
	return false;
    }
    getTreeDependencies(tree);
    if (!checkDependencies(tree, filename, dependencyPosition)) {
	tree.warning("failed to read dependencies");
	error = true;
    } else {
	error = !readTree(tree, in);
    }
    return !error;
}

bool StateTreeReader::checkDependencies(const StateTree &tree, const std::string &filename, Position position) const
{
    Core::DependencySet readDependencies;
    bool ok = true;
    std::ifstream ifs(filename.c_str());
    ifs.seekg(position);
    if (! (readDependencies.read(tree.config, ifs) &&
	   readDependencies.satisfies(dependencies_)) ) {
	ok = false;
    }
    ifs.close();
    return ok;
}

bool StateTreeReader::checkHeader(Core::BinaryInputStream &in, Position &dependencyPosition) const
{
    std::string readMagic;
    int readVersion;
    StateTreeIo::read<std::string>(in, readMagic);
    StateTreeIo::read(in, readVersion);
    if (readMagic == magic && readVersion == fileFormatVersion) {
	StateTreeIo::read(in, dependencyPosition);
	return true;
    } else {
	return false;
    }

}

bool StateTreeReader::readTree(StateTree &tree, Core::BinaryInputStream &in) const
{
    StateTreeIo::read(in, tree.states_);
    StateTreeIo::read(in, tree.root_);
    StateTreeIo::read(in, tree.ciRoot_);
    StateTreeIo::read(in, tree.batches_);
    StateTreeIo::read(in, tree.haveSuccessorBatches_);
    StateTreeIo::read(in, tree.successorBatches_);
    StateTreeIo::read(in, tree.emptyBatch_);
    StateTreeIo::read(in, tree.wordHeadsEnd_);
    bool haveCoarticulatedRoot = false;
    StateTreeIo::read(in, haveCoarticulatedRoot);
    if (haveCoarticulatedRoot) {
	tree.coarticulationStructure_ = new StateTree::CoarticulationStructure();
	tree.initialPhonemes_ = &tree.coarticulationStructure_->initialPhonemes;
	StateTreeIo::read(in, tree.coarticulationStructure_->initialPhonemes);
	StateTreeIo::read(in, tree.coarticulationStructure_->finalPhonemes);
	StateTreeIo::read(in, tree.coarticulationStructure_->roots);
	StateTreeIo::read(in, tree.coarticulationStructure_->boundaryPhonemes);
    }
    return true;
}
