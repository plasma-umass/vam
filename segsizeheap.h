// -*- C++ -*-

#ifndef _SEGSIZEHEAP_H_
#define _SEGSIZEHEAP_H_

namespace VAM {

// SegSizeHeap: a heap that segregates objects based on their sizes
template<size_t MaxObjectSize, class SuperHeap>
class SegSizeHeap : public SuperHeap {

public:

	inline void * malloc(size_t size) {
		assert(size <= MaxObjectSize);

		return _subheap[SIZE_TO_INDEX(size)].malloc(size);
	}

	inline void free(void * ptr) {
		size_t size = SuperHeap::getSize(ptr);
		assert(size <= MaxObjectSize);
		_subheap[SIZE_TO_INDEX(size)].free(ptr);
	}

private:

	SuperHeap _subheap[SIZE_TO_INDEX(MaxObjectSize) + 1];

};	// end of class SegSizeHeap

};	// end of namespace VAM

#endif
