// -*- C++ -*-

#ifndef _TWOHEAP_H_
#define _TWOHEAP_H_

#include "objectheader.h"

namespace VAM {

// TwoHeap: a heap that allocates objects from SuperHeap1 if possbile and otherwise from SuperHeap2
template<class SuperHeap1, class SuperHeap2, size_t PartitionSize>
class TwoHeap : public SuperHeap1 {

public:

	inline void * malloc(size_t size) {
		void * ptr = NULL;

		// allocate regular objects from SuperHeap1
		if (size <= SuperHeap1::MAX_OBJECT_SIZE) {
			ptr = SuperHeap1::malloc(size);
		}
		// allocate very large objects from SuperHeap2
		else {
			// FIXME: we are making assumptions about how our SuperHeap works here
			// the size must be page-aligned and larger than the underlying partition size
			size_t huge_size = size + sizeof(ObjectHeader);
			if (huge_size <= PartitionSize)
				huge_size = PartitionSize + PAGE_SIZE;
			else
				huge_size = (huge_size + PAGE_SIZE - 1) & PAGE_MASK;

			ObjectHeader * header = reinterpret_cast<ObjectHeader *>(_heap.malloc(huge_size, USE_HEADER_TYPE));
			if (header != NULL) {
				header->_size = size;
				ptr = header->getObject();
			}
		}

		return ptr;
	}

	inline void free(void * ptr) {
		ObjectHeader * header = ObjectHeader::getHeader(ptr);

		// check where the object was allocated from
		if (header->_size <= SuperHeap1::MAX_OBJECT_SIZE) {
			SuperHeap1::free(ptr);
		}
		else {
			_heap.free(header);
		}
	}

private:

	SuperHeap2 _heap;

};	// end of class TwoHeap

};	// end of namespace VAM

#endif
