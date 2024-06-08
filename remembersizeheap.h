// -*- C++ -*-

#ifndef _REMEMBERSIZEHEAP_H_
#define _REMEMBERSIZEHEAP_H_

#include "heaplayers.h"

using namespace HL;

namespace VAM {

// RememberSizeHeap: a heap that stores sizes of allocated objects in the headers and frees with the size information
template<class SuperHeap>
class RememberSizeHeap : public SuperHeap {

public:

	inline void * malloc(size_t size) {
		void * ptr = NULL;
		Header * header = reinterpret_cast<Header *>(SuperHeap::malloc(sizeof(Header) + size));

		if (header != NULL) {
			ptr = header + 1;
			header->size = size;
		}

		return ptr;
	}

	inline void free(void * ptr) {
		Header * header = reinterpret_cast<Header *>(ptr) - 1;
		SuperHeap::free(header, sizeof(Header) + header->size);
	}

	inline size_t getSize(void * ptr) {
		Header * header = reinterpret_cast<Header *>(ptr) - 1;
		return header->size;
	}

private:

	struct Header {
		union {
			size_t size;
			double dummy;
		};
	};

};	// end of class RememberSizeHeap

};	// end of namespace VAM

#endif
