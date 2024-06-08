// -*- C++ -*-

#ifndef _ALIGNEDMMAPHEAP_H_
#define _ALIGNEDMMAPHEAP_H_

#include "vamcommon.h"

#include "mapsizeheap.h"
#include "mmapheap.h"

#include "heaplayers.h"

using namespace HL;

namespace VAM {

// AlignedMmapHeap: an mmap-based heap that allocates aligned memory
class AlignedMmapHeap : public MapSizeHeap<PrivateMmapHeap> {

public:

	// allocate aligned mmap region
	inline void * malloc(size_t size, size_t alignment = PAGE_SIZE) {
		assert(size != 0 && (size & ~PAGE_MASK) == 0);
		assert(alignment != 0 && (alignment & ~PAGE_MASK) == 0);
		if (size == 0 || (size & ~PAGE_MASK) != 0 || alignment == 0 || (alignment & ~PAGE_MASK) != 0)
			return NULL;

		size_t start = reinterpret_cast<size_t>(PrivateMmapHeap::malloc(size + alignment));
		abort_on(start == 0);

		size_t ptr = ((start - 1) / alignment + 1) * alignment;
		if (ptr != start) {
			PrivateMmapHeap::free(reinterpret_cast<void *>(start), ptr - start);
		}
		PrivateMmapHeap::free(reinterpret_cast<void *>(ptr + size), start + alignment - ptr);

		_map_lock.lock();
		_ptr_size_map[reinterpret_cast<void * const>(ptr)] = size;
		_map_lock.unlock();

		return reinterpret_cast<void *>(ptr);
	}

};	//end of class AlignedMmapHeap

// TheOneAlignedMmapHeap: singleton of AlignedMmapHeap
class TheOneAlignedMmapHeap {

public:

	TheOneAlignedMmapHeap() {
		static AlignedMmapHeap aligned_mmap_heap;
		_heap = &aligned_mmap_heap;
	}

	inline void * malloc(size_t size, size_t alignment = PAGE_SIZE) {
		return _heap->malloc(size, alignment);
	}

	inline void free(void * ptr) {
		_heap->free(ptr);
	}

	inline size_t getSize(void * ptr) {
		return _heap->getSize(ptr);
	}

private:

	AlignedMmapHeap * _heap;

};	// end of class TheOneAlignedMmapHeap

};	// end of namespace VAM

#endif
