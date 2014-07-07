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
#ifndef _MM_DENSITY_CLUSTERING_HH
#define _MM_DENSITY_CLUSTERING_HH

#include <vector>
#include <Core/Component.hh>
#include <Core/MappedArchive.hh>

namespace Core {
class BinaryInputStream;
class BinaryOutputStream;
}

namespace Mm {

class DensityClusteringBase : public Core::Component
{
public:
    typedef u8 ClusterIndex;
    typedef std::vector<ClusterIndex>::const_iterator ClusterIndexIterator;

    static const Core::ParameterInt paramClusteringIterations;
    static const Core::ParameterInt paramNumClusters;
    static const Core::ParameterInt paramSelectClusters;
    static const Core::ParameterString paramCacheArchive;
    static const Core::ParameterFloat paramBackoffScore;

    DensityClusteringBase(const Core::Configuration &config);
    virtual ~DensityClusteringBase() {}

    /** Initialize the Clustering for the given feature dimension
     * and number of densities.
     * Must be called before build().
     */
    void init(u32 dimension, u32 nDensities);

    /** Maps densities to the assigned cluster index.
     */
    ClusterIndex clusterIndexForDensity(size_t density) const {
      return clusterIndexForDensity_[density];
    }
    /** Iterator over density to cluster index assignment.
     * Iterator starts at the given density index.
     */
    ClusterIndexIterator clusterIndexIterator(size_t density) const {
	return clusterIndexForDensity_.begin() + density;
    }

    u32 nClusters() const { return nClusters_; }

protected:
    /** Save the clustering to the cache.
     * Returns whether writing was successful.
     */
    bool write() const;

    /** Load the clustering from the cache.
     * Returns false on failure, true on success.
     */
    bool load();

    virtual bool writeMeans(Core::MappedArchiveWriter) const = 0;
    virtual bool readMeans(Core::MappedArchiveReader) = 0;
    virtual bool writeTypes(Core::MappedArchiveWriter) const = 0;
    virtual bool readTypes(Core::MappedArchiveReader) const = 0;
    std::vector<ClusterIndex> clusterIndexForDensity_;

    u32 nClusters_, nSelected_;
    u32 dimension_, nDensities_;
    const float backoffScore_;

    static const std::string FileMagic;
    static const u32 FileFormatVersion;
};


template<class F, class D>
class DensityClustering : public DensityClusteringBase
{
public:
    typedef F FeatureType;
    typedef D DistanceType;

    DensityClustering(const Core::Configuration &c) :
	DensityClusteringBase(c),
	clusterMeans_(0) {}

    ~DensityClustering() {
	delete[] clusterMeans_;
    }

    /**
     * Build the density clustering.
     * If paramFile is not empty, the clustering is read from the given file
     * if possible. If the clustering cannot be read from file it is build and
     * written to the file.
     */
    void build(const FeatureType *densityMeans);

    /**
     * @param selection A boolean array where for each selected cluster 'true' will be stored,
     *                  for all unselected 'false'. Must be as large as the number of clusters.
     *                  Using a bitvector would be more memory efficient, but introduces additional
     *                  computations.
     * @param select Number of clusters to select.
     * @param feature The feature, must have the size paddedDimension.
     * */
    void selectClusters(bool *selection, FeatureType* feature) const;

    DistanceType backoffScore() const {
	return backoffScore_;
    }

private:
    bool writeMeans(Core::MappedArchiveWriter) const;
    bool readMeans(Core::MappedArchiveReader);
    bool writeTypes(Core::MappedArchiveWriter) const;
    bool readTypes(Core::MappedArchiveReader) const;


    FeatureType* meanForCluster(ClusterIndex cluster)
    {
	return clusterMeans_ + cluster * dimension_;
    }
    const FeatureType* meanForCluster(ClusterIndex cluster) const
    {
	return clusterMeans_ + cluster * dimension_;
    }

    const FeatureType* meanForDensity(const FeatureType *means, u32 density) const
    {
	return means + density * dimension_;
    }

    typedef std::vector< std::vector<u32> > DensityAssignment;
    void initializeClusters(const FeatureType *densities);
    void assignDensities(const FeatureType *densities, DensityAssignment &densityAssignment);
    f64 updateClusterMeans(const FeatureType *densities, DensityAssignment &densityAssignment);

    FeatureType* clusterMeans_;
};

} // namespace Mm

// Include the template definitions
#include <Mm/DensityClustering.tcc>

#endif // _MM_DENSITY_CLUSTERING_HH
