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
#ifndef _COMPRESSED_MEMORY_MANAGER_HH
#define _COMPRESSED_MEMORY_MANAGER_HH

#include <stdlib.h>
#ifdef _WITH_ZLIB
#include <zlib.h>
#endif
#ifdef _WITH_MINILZO
#include "minilzo.h"
#endif

#include <iostream>
#include <list>
#include <slist>
#include <vector>

#include "Assertions.hh"
#include "Types.hh"

namespace Core {

    /*
     * remarks/ideas:
     *
     * - how to handle uninitialized object space inside blocks? use functions defined in <memory>?
     * - what about classes that allocate memory for member variables?
     * - fill unused and uninitialized memory with 0's to achieve better compression ratios
     */

    template<class T> class CompressedMemoryManager {
    private:
	typedef u32 BlockNumber;
	typedef u32 ObjectNumber;

	static const ObjectNumber objectsPerBlock = (1 << 16) / sizeof(T);
	static const size_t blockSize = objectsPerBlock * sizeof(T);

	class Block {
	private:
	    mutable T *objects_;
	    mutable void *compressedObjects_;
#ifdef _WITH_ZLIB
	    uLongf size_, allocated_;
	    uLongf compressedSize_;
#endif
#ifdef _WITH_MINILZO
	    lzo_uint size_, allocated_;
	    lzo_uint compressedSize_;
	    lzo_align_t __LZO_MMODEL workmem[(LZO1X_1_MEM_COMPRESS + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)];
#endif
	public:
	    Block() : objects_(0), compressedObjects_(0), size_(0), allocated_(0), compressedSize_(0) {}
	    Block(const Block &b) {
		objects_ = b.objects_;
		b.objects_ = 0;
		compressedObjects_ = b.compressedObjects_;
		b.compressedObjects_ = 0;
		size_ = b.size_;
		compressedSize_ = b.compressedSize_;
	    }
	    ~Block() {
		if (compressedObjects_) ::free(compressedObjects_);
		if (objects_) ::free(objects_);
	    }

	    bool isCompressed() const { return (compressedSize_ != 0); }

	    T* object(ObjectNumber object) {
		require(!isCompressed());
		require(object < size_);
		return &objects_[object];
	    }
	    ObjectNumber allocatable() const {
		require(!isCompressed());
		return size_ - allocated_;
	    }
	    ObjectNumber allocate(ObjectNumber n) {
		require(!isCompressed());
		if (objects_ == 0) {
		    size_ = std::max(objectsPerBlock, n);
		    allocated_ = n;
		    objects_ = (T*)calloc(size_, sizeof(T));
		} else {
		    require(n < size_ - allocated_);
		    allocated_ += n;
		}
		return allocated_ - n;
	    }
	    void deallocate(ObjectNumber object, ObjectNumber n) {
		require(!isCompressed());
		memset(&objects_[object], 0, n * sizeof(T));
		if (object + n == allocated_) allocated_ = object;
		if (allocated_ == 0) {
		    ::free(objects_);
		    objects_ = 0;
		    size_ = 0;
		}
	    }

	    double compress() {
		require(!isCompressed());
#ifdef _WITH_ZLIB
		compressedSize_ = int(1.002 * sizeof(T) * size_ + 12);
		compressedObjects_ = malloc(compressedSize_);
		int ok = ::compress2(reinterpret_cast<Bytef*>(compressedObjects_), &compressedSize_,
				     reinterpret_cast<const Bytef*>(objects_), size_,
				     Z_BEST_SPEED);
		require(ok == Z_OK);
#endif
#ifdef _WITH_MINILZO
		compressedSize_ = int(sizeof(T) * size_ + (sizeof(T) * size_) / 64 + 16 + 3);
		compressedObjects_ = malloc(compressedSize_);
		int ok = ::lzo1x_1_compress(reinterpret_cast<const lzo_byte*>(objects_), size_,
					    reinterpret_cast<lzo_byte*>(compressedObjects_),
					    &compressedSize_, workmem);
		require(ok == LZO_E_OK);
#endif
		compressedObjects_ = realloc(compressedObjects_, compressedSize_);
		::free(objects_);
		objects_ = 0;
		//std::cout << "compressed " << size_ * sizeof(T) << " to " << compressedSize_ << std::endl;
		return (sizeof(T) * size_) / compressedSize_;
	    };
	    void uncompress() {
		if (isCompressed()) {
#ifdef _WITH_ZLIB
		    objects_ = (T*)malloc(size_t(size_));
		    ::uncompress(reinterpret_cast<Bytef*>(objects_), &size_,
				 reinterpret_cast<const Bytef*>(compressedObjects_), compressedSize_);
#endif
#ifdef _WITH_MINILZO
		    objects_ = (T*)malloc(size_t(size_));
		    ::lzo1x_decompress(reinterpret_cast<const lzo_byte*>(compressedObjects_),
				       compressedSize_, reinterpret_cast<lzo_byte*>(objects_), &size_, 0);
#endif
		    compressedSize_ = 0;
		    ::free(compressedObjects_);
		    compressedObjects_ = 0;
		}
	    };
	};

    public:
	class Pointer {
	private:
	    CompressedMemoryManager *manager_;
	    BlockNumber block_;
	    ObjectNumber firstObject_;
	    ObjectNumber nObjects_;
	public:
	    Pointer() : manager_(0) {}
	    Pointer(CompressedMemoryManager *manager, BlockNumber block, ObjectNumber firstObject,
		    ObjectNumber nObjects)
		: manager_(manager), block_(block), firstObject_(firstObject), nObjects_(nObjects) {}

	    BlockNumber block() const { return block_; }
	    ObjectNumber object() const { return firstObject_; }
	    ObjectNumber nObjects() const { return nObjects_; }

	    T& operator-> () {
		return manager_->uncompressBlock(block_)->object(firstObject_);
	    }
	    T& operator[] (size_t i) {
		require(i < nObjects_);
		return *(manager_->uncompressBlock(block_)->object(firstObject_ + i));
	    }
	};

    private:
	BlockNumber maximumUncompressedCacheSize_;
	std::vector<Block> blocks_;
	std::list<BlockNumber> uncompressedSortedByTime_;
	std::vector<std::slist<BlockNumber> > blocksThatAreNotFull_;
#ifdef _WITH_STATISTICS
	u32 nCompressions_;
	double averageCompressionRatio_;
	double minimumCompressionRatio_;
	double maximumCompressionRatio_;
#endif

    public:
	CompressedMemoryManager(BlockNumber nBlocks = 100) : blocksThatAreNotFull_(objectsPerBlock) {
	    if (nBlocks <= 0) nBlocks = 1;
	    maximumUncompressedCacheSize_ = nBlocks;
#ifdef _WITH_STATISTICS
	    nCompressions_ = 0;
	    averageCompressionRatio_ = 0.0;
	    minimumCompressionRatio_ = Core::Type<double>::max;
	    maximumCompressionRatio_ = 0.0;
#endif
	}

	BlockNumber newBlock() {
	    BlockNumber b;
	    if (blocksThatAreNotFull_[0].empty()) {
		b = blocks_.size();
		blocks_.resize(blocks_.size() + 1);
	    } else {
		b = blocksThatAreNotFull_[0].front();
		blocksThatAreNotFull_[0].pop_front();
	    }
	    uncompressedSortedByTime_.push_back(b);
	    compressBlocks();
	    return b;
	}
	void compressBlocks() {
	    while (uncompressedSortedByTime_.size() > maximumUncompressedCacheSize_) {
		double ratio = blocks_[uncompressedSortedByTime_.front()].compress();
#ifdef _WITH_STATISTICS
		nCompressions_++;
		averageCompressionRatio_ += ratio;
		minimumCompressionRatio_ = std::min(minimumCompressionRatio_, ratio);
		maximumCompressionRatio_ = std::max(maximumCompressionRatio_, ratio);
#endif
		uncompressedSortedByTime_.pop_front();
	    }
	}
	Block* uncompressBlock(BlockNumber block) {
	    if (blocks_[block].isCompressed()) {
		blocks_[block].uncompress();
		uncompressedSortedByTime_.push_back(block);
		compressBlocks();
	    }
	    return &blocks_[block];
	}

	Pointer allocate(size_t n = 1) {
	    require(n <= Core::Type<ObjectNumber>::max);
	    BlockNumber b;
	    ObjectNumber o;
	    if (n < objectsPerBlock) {
		if (!blocksThatAreNotFull_[n].empty()) {
		    b = blocksThatAreNotFull_[n].front();
		    blocksThatAreNotFull_[n].pop_front();
		} else b = newBlock();
		o = blocks_[b].allocate(n);
		if (blocks_[b].allocatable() > 0) blocksThatAreNotFull_[n].push_front(b);
	    } else {
		b = newBlock();
		o = blocks_[b].allocate(n);
	    }
	    return Pointer(this, b, o, n);
	}
	void deallocate(Pointer p) {
	    Block &b = blocks_[p->block()];
	    if (b.isCompressed()) b.uncompress();
	    b->deallocate(p->object(), p->nObjects());
	}

	void printStatistics(std::ostream &o) {
#ifdef _WITH_STATISTICS
	    o.form("average compression ratio %.2f\n", averageCompressionRatio_ / (double)nCompressions_);
	    o.form("minimum compression ratio %.2f\n", minimumCompressionRatio_);
	    o.form("maximum compression ratio %.2f\n", maximumCompressionRatio_);
#endif
	}
    };

} // namespace Core

#endif // _COMPRESSED_MEMORY_MANAGER_HH
