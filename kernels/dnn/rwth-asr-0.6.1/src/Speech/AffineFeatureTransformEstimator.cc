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
#include "AffineFeatureTransformEstimator.hh"
#include <Core/Directory.hh>
#include <Math/Module.hh>
#include <Core/MapParser.hh>
#include <Mm/MixtureSet.hh>

using namespace Speech;


const Core::ParameterString AffineFeatureTransformEstimator::paramInitialTransform(
	"initial-transform", "name of initial value for transforms", "");

const Core::ParameterString AffineFeatureTransformEstimator::paramInitialTransformDirectory(
	"initial-transform-directory", "name of directory of initial values for transforms", "");

const Core::ParameterString AffineFeatureTransformEstimator::paramTransformDirectory(
	"transform-directory", "name of directory of transforms", "transforms");

const Core::ParameterInt AffineFeatureTransformEstimator::paramEstimationIterations(
	"estimation-iterations", "number of iterations when estimating transform, 0 = don't estimate", 10);

const Core::ParameterFloat AffineFeatureTransformEstimator::paramMinObservationWeight(
	"min-observation-weight", "minimum observation weight", 2000);

const Core::Choice AffineFeatureTransformEstimator::optimizationCriterionChoice(
	"naive", ConcreteAccumulator::naive,
	"mmi-prime", ConcreteAccumulator::mmiPrime,
	"hlda", ConcreteAccumulator::hlda,
	Core::Choice::endMark());
const Core::ParameterChoice AffineFeatureTransformEstimator::paramOptimizationCriterion(
	"optimization-criterion", &optimizationCriterionChoice,
	"criterion used for transform scoring", ConcreteAccumulator::mmiPrime);

AffineFeatureTransformEstimator::AffineFeatureTransformEstimator(const Core::Configuration &c, Operation op) :
    Component(c),
    Precursor(c, op)
{
    Core::IoRef<Mm::AbstractAdaptationAccumulator>::registerClass
	<ConcreteAccumulator>();
}

AffineFeatureTransformEstimator::~AffineFeatureTransformEstimator()
{ }

void AffineFeatureTransformEstimator::createAccumulator(std::string key)
{
    currentAccumulator_= new Accumulator(
	new ConcreteAccumulator(featureDimension_, modelDimension_, key));
}

void AffineFeatureTransformEstimator::postProcess()
{
    std::string outputDirectory= paramTransformDirectory(config);
    if (paramEstimationIterations(config) > 0)
	Core::createDirectory(outputDirectory);

    bool initialDirectory(paramInitialTransformDirectory(config) != "");
    bool initialFile(paramInitialTransform(config) != "");

    ConcreteAccumulator::Transform initialTransform;

    if (initialDirectory && initialFile)
	warning("Value for 'initial-transform-directory' overrides value for 'initial-transform'.");
    else if (initialFile)
	if (!Math::Module::instance().formats().read(paramInitialTransform(config), initialTransform))
	    error("Failed to read matrix from file '%s'.",
		    paramInitialTransform(config).c_str());

    std::set<std::string> seenKeys= accumulatorCache_.keys();
    log("number of keys: %zu", seenKeys.size());

    for (std::set<std::string>::iterator key= seenKeys.begin(); key!=seenKeys.end(); ++key) {
	/*! @todo this is a more or less diry hack.
	  what we want to do here is to modify the object
	  without writing the modification back to cache
	*/
	currentAccumulator_ = const_cast<Accumulator*>(accumulatorCache_.findForReadAccess(*key));
	verify(currentAccumulator_);

	ConcreteAccumulator *concreteCurrentAccumulator =
	    dynamic_cast<ConcreteAccumulator*>(&**currentAccumulator_);

	concreteCurrentAccumulator->finalize();
	if (paramEstimationIterations(config) > 0) {
	    ConcreteAccumulator::Transform transform;
	    if (initialDirectory) {
		if (!Math::Module::instance().formats().read(Core::joinPaths(
			paramInitialTransformDirectory(config),
			    *key + transformExtension()),
			initialTransform))
		{
		    error("Failed to read matrix from file '%s'.",
			    Core::joinPaths(paramInitialTransformDirectory(config),
				*key + transformExtension()).c_str() );
		    initialTransform.clear();
		}
		transform=
		    concreteCurrentAccumulator->estimate(paramEstimationIterations(config),
							 paramMinObservationWeight(config),
							 static_cast<ConcreteAccumulator::Criterion>(paramOptimizationCriterion(config)),
							 initialTransform);
	    }
	    else if (initialFile) {
		transform=
		    concreteCurrentAccumulator->estimate(paramEstimationIterations(config),
							 paramMinObservationWeight(config),
							 static_cast<ConcreteAccumulator::Criterion>(paramOptimizationCriterion(config)),
							 initialTransform);
	    }
	    else {
		transform=
		    concreteCurrentAccumulator->estimate(paramEstimationIterations(config),
							 paramMinObservationWeight(config),
							 static_cast<ConcreteAccumulator::Criterion>(paramOptimizationCriterion(config)));
	    }
	    std::string filename = Core::joinPaths(outputDirectory, *key + transformExtension());
	    if (!Core::createDirectory(Core::directoryName(filename))) {
		error("Cannot create directory '%s'", Core::directoryName(filename).c_str());
	    }
	    Core::XmlOutputStream os(filename);
	    if (!os.good()) {
		error("Cannot write to file '%s'", filename.c_str());
	    }
	    os << transform;
	}
	concreteCurrentAccumulator->compact();
    }
}

void AffineFeatureTransformEstimator::scoreTransforms()
{
    std::string transformDirectory= paramTransformDirectory(config);

    std::vector<std::pair<std::string, ConcreteAccumulator::Transform> > transforms;

    Core::DirectoryFileIterator::FileExtensionFilter filter(transformExtension());

    for (Core::DirectoryFileIterator file(transformDirectory, &filter); file; ++file) {
	ConcreteAccumulator::Transform transform;
	Math::Module::instance().formats().read(Core::joinPaths(transformDirectory, file.path()), transform);
	std::string fileKey(file.path().substr(0,file.path().size()-std::string(transformExtension()).size()));
	transforms.push_back(std::make_pair(fileKey, transform));
    }

    std::set<std::string> seenKeys= accumulatorCache_.keys();
    for (std::set<std::string>::iterator key= seenKeys.begin(); key!=seenKeys.end(); ++key) {
	currentAccumulator_ = const_cast<Accumulator*>(accumulatorCache_.findForReadAccess(*key));
	verify(currentAccumulator_);

	ConcreteAccumulator *concreteCurrentAccumulator =
	    dynamic_cast<ConcreteAccumulator*>(&**currentAccumulator_);

	concreteCurrentAccumulator->finalize();

	for (size_t i=0; i<transforms.size(); i++) {
	    // TODO: Nicer logging!
	    Mm::Sum score=concreteCurrentAccumulator->score(transforms[i].second,
							    static_cast<ConcreteAccumulator::Criterion>(paramOptimizationCriterion(config)));
	    std::cerr << *key << " " << transforms[i].first << " " << score << std::endl;
	    log("Score, corpus-key=\"%s\" transform=\"%s\": %f", key->c_str(), transforms[i].first.c_str(), score);
	}
    }
}
