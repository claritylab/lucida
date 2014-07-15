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
#ifndef _MM_UTILITIES_HH
#define _MM_UTILITIES_HH

#include <Core/Utility.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/XmlStream.hh>
#include "Types.hh"

namespace Mm {

    /** Creates hash key from a pointer by casting to unsigned int. */
    template<class Referenced>
    struct hashReference : std::unary_function<Core::Ref<Referenced>, size_t> {
	size_t operator()(Core::Ref<Referenced> x) const { return (size_t)x.get(); }
    };

    /** @return is *x == *y */
    template<class Pointer>
    struct equalReferenced : public std::binary_function<Pointer, Pointer, bool> {
	bool operator()(Pointer x, Pointer y) const { return *x == *y; }
    };

    /** log ( sum_i exp(v_i) ) */
    template<class InputIterator>
    double logExpNorm(InputIterator begin, InputIterator end) {
	InputIterator maxIt = std::max_element(begin, end);
	double result = 0;
	for(;begin != end; ++ begin)
	    if (begin != maxIt) result += exp(*begin - *maxIt);
	return log1p(result) + *maxIt;
    };

    /** sum_i log(v_i) */
    template<class InputIterator>
    double logNorm(InputIterator begin, InputIterator end) {
	double result = 0;
	for(;begin != end; ++ begin)
	    result += log(Core::abs(*begin));
	return result;
    };
    /** sum_i ( weight_i * log(v_i) )*/
    template<class InputIterator,class WeightIterator>
    double logNorm(InputIterator beginDiagonal, InputIterator endDiagonal, WeightIterator beginWeights, WeightIterator endWeights) {
	double result = 0;
	for(;beginDiagonal != endDiagonal; ++ beginDiagonal){
	    result += (*beginWeights) * log(Core::abs(*beginDiagonal));
	    ++ beginWeights;
	}
	return result;
    };
    /** N * log(2 * pi) + sum_i log(v_i) */
    template<class InputIterator>
    double gaussLogNormFactor(InputIterator begin, InputIterator end) {
	return (double)std::distance(begin, end) * log((double)2 * M_PI) +
	    logNorm(begin, end);
    }

    /** N * log(2 * pi) + sum_i ( weight_i * log(v_i) ) */
    template<class InputIterator,class WeightIterator>
    double gaussLogNormFactor(InputIterator beginDiagonal, InputIterator endDiagonal, WeightIterator beginWeights, WeightIterator endWeights) {
	return (double)std::distance(beginDiagonal, endDiagonal) * log((double)2 * M_PI) +
	    logNorm(beginDiagonal, endDiagonal, beginWeights, endWeights);
    }

    /** 1 / sqrt(x) */
    template<class T>
    struct inverseSquareRoot : public std::unary_function<T, T> {
	T operator()(T x) const { return (T)1 / (T)sqrt(x); }
    };

    /** (x - y) / normalization */
    template <class T>
    class normalizedMinus : public std::binary_function<T, T, T> {
	T normalization_;
    public:
	normalizedMinus(T normalization) :  normalization_(normalization) { require(normalization > 0); }
	T operator()(T x, T y) const { return (x - y) / normalization_; }
    };

    /** x + weight*y */
    template<class T>
    struct plusWeighted : public std::binary_function<T, T, T> {
	T weight_;
    private:
	plusWeighted() {}
    public:
	plusWeighted(T weight) : weight_(weight) {}
	T operator()(T x, T y) const { return x + weight_ * y; }
    };

    /** x + y^2 */
    template<class T>
    struct plusSquare : public std::binary_function<T, T, T> {
	T operator()(T x, T y) const { return x + y * y; }
    };

    /** x + weight*y^2 */
    template<class T>
    struct plusSquareWeighted : public std::binary_function<T, T, T> {
	T weight_;
    private:
	plusSquareWeighted() {}
    public:
	plusSquareWeighted(T weight) : weight_(weight) {}
	T operator()(T x, T y) const { return x + weight_ * y * y; }
    };

    /** x + y^2 / normalization_ */
    template<class T>
    class plusNormalizedSquare : public std::binary_function<T, T, T> {
	T normalization_;
    public:
	plusNormalizedSquare(T normalization) : normalization_(normalization) {}
	T operator()(T x, T y) const { return x + y * y / normalization_; }
    };

    /** -factor * log (x) */
    template<class T>
    class minusLog : public std::unary_function<T, T> {
	T factor_;
    public:
	minusLog(T factor) : factor_(factor) {}
	T operator()(T x) const { return - factor_ * log(x); }
    };

    /** @return = clip(floor(x) + offset),
     *  where "clip" clips to minimal or to the maximal integer value.
     *
     *  Remark: mean{x} is expected to be 0, so offset is simply to half of the integer interval.
     */
    template<class ValueType, class QuantizedType>
    class quantize : public std::unary_function<ValueType, QuantizedType> {
	QuantizedType clip(int x) {
	    return std::min(std::max(x, int(Core::Type<QuantizedType>::min)),
			    int(Core::Type<QuantizedType>::max));
	}
    public:
#if 1
	QuantizedType operator()(ValueType x) {
	    static const int offset =
		(int)round((Core::Type<QuantizedType>::max + Core::Type<QuantizedType>::min + 1) / 2);
	    return clip((int)round(x) + offset);
	}
#endif
    };

    template<class InputIterator1, class InputIterator2, class OutputIterator, class Operation>
    void unrolledTransform(InputIterator1 begin1, InputIterator1 end1,
			   InputIterator2 begin2, OutputIterator result,
			   Operation operation) {
	switch ((std::distance(begin1, end1) - 1) % 8) {
	    while (begin1 != end1) {
	    case 7: *result = operation(*begin1, *begin2); ++ begin1; ++ begin2; ++ result;
	    case 6: *result = operation(*begin1, *begin2); ++ begin1; ++ begin2; ++ result;
	    case 5: *result = operation(*begin1, *begin2); ++ begin1; ++ begin2; ++ result;
	    case 4: *result = operation(*begin1, *begin2); ++ begin1; ++ begin2; ++ result;
	    case 3: *result = operation(*begin1, *begin2); ++ begin1; ++ begin2; ++ result;
	    case 2: *result = operation(*begin1, *begin2); ++ begin1; ++ begin2; ++ result;
	    case 1: *result = operation(*begin1, *begin2); ++ begin1; ++ begin2; ++ result;
	    case 0: *result = operation(*begin1, *begin2); ++ begin1; ++ begin2; ++ result;
	    }
	}
    }

    template<class T, class DistanceType>
    inline DistanceType unrolledVectorDistance(const T *a, const T *b, size_t dimension) {
      DistanceType df, score = 0;
      u32 cmp = 0;
      switch ((dimension - 1) % 8) {
	while (cmp < dimension) {
	case 7: df = (a[cmp] - b[cmp]); score += df * df; ++cmp;
	case 6: df = (a[cmp] - b[cmp]); score += df * df; ++cmp;
	case 5: df = (a[cmp] - b[cmp]); score += df * df; ++cmp;
	case 4: df = (a[cmp] - b[cmp]); score += df * df; ++cmp;
	case 3: df = (a[cmp] - b[cmp]); score += df * df; ++cmp;
	case 2: df = (a[cmp] - b[cmp]); score += df * df; ++cmp;
	case 1: df = (a[cmp] - b[cmp]); score += df * df; ++cmp;
	case 0: df = (a[cmp] - b[cmp]); score += df * df; ++cmp;
	}
      }
      verify_( cmp == dimension );
      return score;
    }


    /**
     *  Cache class optimized for short creation time.
     *  Optmiziations:
     *  -isCalculated_ array is an optimized array with fast initilization time.
     *  -objects_ is no STL vector to avoid any calls to the constructor of the objects when
     *   after the memory.
     */
    template<class O>
    class Cache {
	std::vector<bool> isCalculated_;
	O* objects_;
    public:
	Cache(size_t size) : isCalculated_(size, false) { objects_ = new O[size]; }
	~Cache() { delete [] objects_; }

	const O &set(size_t index, const O &o) {
	    isCalculated_[index] = true; return (objects_[index] = o);
	}

	bool isCalculated(size_t index) const { return isCalculated_[index]; }
	const O &operator[](size_t index) const { return objects_[index]; }
	size_t size() const { return isCalculated_.size(); }
    };

    /**
     *  Two dimensional cache class optimized for short creation time.
     *  Optmiziations:
     *  -isCalculated_ array is an optimized array with fast initilization time.
     *  -objects_ is no STL vector to avoid any calls to the constructor of the objects when
     *   after the memory.
     */
	template<class O>
	class Cache2D {
		std::vector<std::pair<bool ,std::vector <bool> > > isCalculated_;
		O** objects_;
		bool freedStorage_;
	public:
		Cache2D(size_t first_level_size) :
			isCalculated_(first_level_size, std::make_pair(false, std::vector<bool>())),
			freedStorage_(false)
		{
			objects_ = new O*[first_level_size];
			for(size_t i = 0 ; i < first_level_size; ++i){
				objects_[i] = 0;
			}
		}

		/** deep copy constructor to prevent side effects in combination with STL **/
		Cache2D(const Cache2D<O>& other)
		{
			// copy is calculated
			isCalculated_ = other.isCalculated_;
			s32 first_level_size = (s32)(other.isCalculated_.size());

			// allocate memory for first level
			objects_ = new O*[first_level_size];
			// allocate memory for second level and copy contents
			for(u32 i = 0; i < first_level_size; ++i){
				if(other.objects_[i]){
					// get correct size
					u32 tsize = isCalculated_[i].second.size();
					objects_[i] = new O[tsize];
					O*& obj = objects_[i];
					O*& otherObj = other.objects_[i];
					for(u32 j = 0; j < tsize; ++j){
						obj[j] = otherObj[j];
					}
				} else {
					objects_[i] = 0;
				}
			}
		}

		~Cache2D()
		{
			if(!freedStorage_){
				for(size_t i = 0; i < isCalculated_.size(); ++i ){
					if(objects_[i]){
						delete[] (objects_[i]);
					}
				}
				delete[] objects_;
			}
		}

		void unsetEmissionFlag(size_t eIndex){
			require(!freedStorage_);
			isCalculated_[eIndex].first = false;
		}

		bool initEmission(size_t index, size_t nDens){
			require(!freedStorage_);
			if(isCalculated_[index].first){
				return false;
			}
			objects_[index] = new O[nDens];
			isCalculated_[index].first = true;
			isCalculated_[index].second.resize(nDens,false);
			return true;
		}

		bool isInitialized(size_t index) const{
			require(!freedStorage_);
			return isCalculated_[index].first;
		}

		bool isCalculated(size_t eIndex, size_t dIndex) const{
			require(!freedStorage_);
			return isCalculated_[eIndex].second[dIndex];
		}

		const O& set(size_t eIndex, size_t dIndex, const O& o){
			require(!freedStorage_);
			isCalculated_[eIndex].second[dIndex] = true;
			return( objects_[eIndex][dIndex] = o);
		}

		const O& operator()(size_t eIndex, size_t dIndex) const{
			require(!freedStorage_);
			return( objects_[eIndex][dIndex]);
		}

		size_t size() const {return isCalculated_.size(); }

		void free_storage(){
			if(!freedStorage_){
				for(size_t i = 0; i < isCalculated_.size(); ++i ){
					if(objects_[i]){
						delete[] objects_[i];
					}
					isCalculated_[i].second.clear();
				}
				delete[] objects_;
				isCalculated_.clear();
				freedStorage_ = true;
			}
		} // end function
	};

	/**
	 * Dynamic Cache class
	 * Note: this cache class should only be used with types or classes with
	 *       fast initialization time
	 * Optimizations:
	 * - isCalculated_ array is an optimized array with fast initialization type
	 */
	template<class Type>
	class DynamicCache{
	private:
		std::vector<bool> isCalculated_;
		std::vector<Type> objects_;
	public:
		DynamicCache(): isCalculated_(), objects_() {}
		DynamicCache(size_t size): isCalculated_(size,false), objects_(size, Type()) {}
		~DynamicCache() {}

		const Type& set(size_t index, const Type& obj){
			isCalculated_[index] = true;
			return(objects_[index] = obj);
		}

		bool isCalculated(size_t index) const{
			return isCalculated_[index];
		}

		size_t size() const{
			return isCalculated_.size();
		}

		const Type& operator[](size_t index) const{
			return objects_[index];
		}

		void clear(){
			isCalculated_.clear();
			objects_.clear();
		}

		void addFrame(){
			isCalculated_.push_back(false);
			objects_.push_back(Type());
		}
	};

    /** Histogram statistics for probabilities.
     *  All accumulated probabilities are clipped to fit into the interval [0..1].
     */
    class ProbabilityStatistics {
    private:
	const std::string name_;
	int nBuckets_;
	Weight *counts_;
	Sum *sum_;
    public:
	ProbabilityStatistics(const std::string &name, u32 nBuckets=100);
	~ProbabilityStatistics();
	void operator+=(Weight probability);
	void writeXml(Core::XmlWriter&) const;
    };
}

#endif // _MM_UTILITIES_HH
