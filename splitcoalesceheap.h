// -*- C++ -*-

#ifndef _SPLITCOALESCEHEAP_H_
#define _SPLITCOALESCEHEAP_H_

#include "objectheader.h"

namespace VAM {

// SplitCoalesceHeap: a heap that performs splitting and coalescing
template<class SuperHeap1, class SuperHeap2, size_t SuperChunkSize>
class SplitCoalesceHeap : public SuperHeap1, public SuperHeap2 {

public:

	inline void * malloc(size_t size) {
		void * ptr = SuperHeap1::malloc(size);
		ObjectHeader * header;

		if (ptr != NULL) {
			header = ObjectHeader::getHeader(ptr);
		}
		else {
			header = reinterpret_cast<ObjectHeader *>(SuperHeap2::malloc(SuperChunkSize));
			if (header != NULL) {
				// set headers, 2 at the beginning and 2 at the end

				// the first header is for an empty object
				header->_size = 0;

				// the second header is for the actual object
				header++;
				header->_size = MAX_OBJECT_SIZE;
				header->_prev_size = 0;
				header->_prev_free = 0;	// set the previous object non-free to avoid coalescing
				assert(header->getPrevHeader()->getNextHeader() == header);

				// now set the tail headers, the very last header is there only to occupy the free bit of the second last header
				ObjectHeader * tail_header = header->getNextHeader();
				tail_header->_prev_size = MAX_OBJECT_SIZE;
				tail_header->_prev_free = 1;
				tail_header->_size = 0;
				tail_header->setFree(0);

				ptr = header->getObject();
			}
		}

		// ok, we've got something and we need to do some work
		if (ptr != NULL) {
			assert(header == ObjectHeader::getHeader(ptr));
			assert(header->isFree());

			header->setFree(0);

			// split the object if possible
			ObjectHeader * split_piece_header = split(header, size);
			if (split_piece_header != NULL) {
				assert(split_piece_header->isFree());
				SuperHeap1::free(split_piece_header->getObject());

				assert(header == split_piece_header->getPrevHeader());
				assert(split_piece_header == header->getNextHeader());
				assert(split_piece_header == split_piece_header->getNextHeader()->getPrevHeader());
			}

			assert(header->getNextHeader()->getPrevHeader() == header);
			assert(!header->isFree());
			assert(header->_size >= size);
		}

		return ptr;
	}

	inline void free(void * ptr) {
		ObjectHeader * header = ObjectHeader::getHeader(ptr);
		assert(!header->isFree());

		list_head * node;

		assert(header->getPrevHeader()->getNextHeader() == header);
		assert(header->getNextHeader()->getPrevHeader() == header);

		header->setFree(1);

		// check if we can coalesce with the previous object
		if (header->_prev_free) {
			ObjectHeader * prev_header = header->getPrevHeader();
			SuperHeap1::remove(prev_header->getObject());

			coalesce(prev_header, header);
			header = prev_header;
		}

		// check if we can coalesce with the next object
		if (header->getNextHeader()->isFree()) {
			ObjectHeader * next_header = header->getNextHeader();
			SuperHeap1::remove(next_header->getObject());

			coalesce(header, next_header);
		}

		SuperHeap1::free(header->getObject());
	}

	inline size_t getSize(void * ptr) {
		return ObjectHeader::getHeader(ptr)->_size;
	}

protected:
	enum {
		MAX_OBJECT_SIZE = SuperChunkSize - 4 * sizeof(ObjectHeader),
	};

private:

	// split an object just allocated in order to reuse the remainder
	inline static ObjectHeader * split(ObjectHeader * header, size_t requested_size) {
		size_t actual_size = header->_size;
		size_t remaining_size = actual_size - requested_size;
		assert(actual_size >= requested_size);
		assert(!header->isFree());

		// split the object if the remainder is big enough
		if (remaining_size >= sizeof(ObjectHeader) + sizeof(list_head)) {
			header->_size = requested_size;

			ObjectHeader * split_piece_header = header->getNextHeader();
			split_piece_header->_size = remaining_size - sizeof(ObjectHeader);
			split_piece_header->_prev_size = requested_size;
			split_piece_header->_prev_free = 0;
			assert(!split_piece_header->isFree());

			ObjectHeader * next_header = split_piece_header->getNextHeader();
			assert(next_header->_prev_size == actual_size);
			next_header->_prev_size = remaining_size - sizeof(ObjectHeader);
			next_header->_prev_free = 1;

			return split_piece_header;
		}
		return NULL;
	}

	// coalesce two consecutive free objects
	inline static void coalesce(ObjectHeader * first, ObjectHeader * second) {
		assert(first->getNextHeader() == second);
		assert(second->getPrevHeader() == first);
		assert(first->isFree() && second->isFree());

		size_t new_size = reinterpret_cast<size_t>(second) - reinterpret_cast<size_t>(first) + second->_size;

		first->_size = new_size;
		first->getNextHeader()->_prev_size = new_size;

		assert(first->getNextHeader() == second->getNextHeader());
		assert(first->getNextHeader()->getPrevHeader() == first);
	}

};	// end of class SplitCoalesceHeap

};	// end of namespace VAM

#endif
