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
#include <Mm/DensityClustering.hh>
#include <Core/Application.hh>

using namespace Mm;

const Core::ParameterInt DensityClusteringBase::paramNumClusters(
    "clusters", "number of density clusters to build for density preselection", 256, 1, 256);
const Core::ParameterInt DensityClusteringBase::paramSelectClusters(
    "select-clusters",
    "number of clusters to select in density preselection."
    "when it equals the total number of clusters, no preselection is performed.", 32, 1, 256);
const Core::ParameterString DensityClusteringBase::paramCacheArchive(
    "cache-archive", "cache-archive where to cache the clustering of the density preselection", "global-cache");
const Core::ParameterInt DensityClusteringBase::paramClusteringIterations(
    "iterations", "number of clustering iterations", 5);
const Core::ParameterFloat DensityClusteringBase::paramBackoffScore(
    "backoff-score", "score used if no cluster is selected", 40000);

const std::string DensityClusteringBase::FileMagic = "SPRINT-DC";
const u32 DensityClusteringBase::FileFormatVersion = 2;

DensityClusteringBase::DensityClusteringBase(const Core::Configuration &config) :
	Core::Component(config),
	nClusters_(paramNumClusters(config)),
	nSelected_(paramSelectClusters(config)),
	dimension_(0), nDensities_(0),
	backoffScore_(paramBackoffScore(config)) {}


void DensityClusteringBase::init(u32 dimension, u32 nDensities)
{
    dimension_ = dimension;
    nDensities_ = nDensities;
    clusterIndexForDensity_.resize(nDensities_, 0);
    // verification to make sure that ClusterIndex can hold the cluster-indices
    verify(nClusters_ - 1 <= Core::Type<ClusterIndex>::max);
    if(nDensities_ < nClusters_)
    {
	log() << "reducing number of clusters from " << nClusters_ << " to " << nDensities_ << " because there are too few densities";
	nClusters_ = nDensities_;
    }
    verify(nClusters_ <= nDensities_);
    verify(nSelected_ <= nClusters_);
}


bool DensityClusteringBase::load()
{
    Core::MappedArchiveReader is = Core::Application::us()->getCacheArchiveReader(paramCacheArchive(config), "density-clustering");
    if(!is.good())
	return false;

    if(!is.check<std::string>( FileMagic, "magic token" ) ||
	!is.check<u32>( FileFormatVersion, "format version" ))
	return false;

    if (!readTypes(is)) {
	warning("cannot read type information");
	return false;
    }

    if(!is.check<u32>(dimension_, "dimension") ||
	!is.check<u32>(nClusters_, "number of clusters") ||
	!is.check<u32>(nDensities_, "number of densities") )
	return false;

    is >> clusterIndexForDensity_;
    return readMeans(is);
}

bool DensityClusteringBase::write() const
{
    Core::MappedArchiveWriter os = Core::Application::us()->getCacheArchiveWriter(paramCacheArchive(config), "density-clustering");
    if(!os.good())
	return false;

    os << FileMagic << FileFormatVersion;
    if (!writeTypes(os))
	return false;
    os << dimension_ << nClusters_ << nDensities_;
    os << clusterIndexForDensity_;
    if (!writeMeans(os))
	return false;
    return os.good();
}
