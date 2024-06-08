// -*- C++ -*-

#ifndef _ONESIZEHEAP_H_
#define _ONESIZEHEAP_H_

#include "vamcommon.h"

namespace VAM {

// OneSizeHeap: a heap that allocates objects of a fixed size and creates new subheaps in exponentially increasing sizes
template <unsigned char MaxSubHeapType, class SubHeap, class SuperHeap>
class OneSizeHeap : public SuperHeap {

public:

	OneSizeHeap() : _object_size(0), _next_subheap_type(1) {
		INIT_LIST_HEAD(&_full_subheap_list);
		INIT_LIST_HEAD(&_avai_subheap_list);

		dbprintf("OneSizeHeap: sizeof(SubHeap)=%u\n", sizeof(SubHeap));
	}

	~OneSizeHeap() {
		// free all subheaps
		while (!list_empty(&_full_subheap_list)) {
			list_head * node = _full_subheap_list.next;
			SubHeap * subheap = SubHeap::listToHeap(node);
			assert(subheap->getList() == node);

			removeSubHeap(subheap);
		}
		while (!list_empty(&_avai_subheap_list)) {
			list_head * node = _avai_subheap_list.next;
			SubHeap * subheap = SubHeap::listToHeap(node);
			assert(subheap->getList() == node);

			removeSubHeap(subheap);
		}
	}

	inline void * malloc(size_t size) {
		sanityCheck();

		void * ptr = NULL;
		assert(_object_size == 0 || size == _object_size);
		assert(size < PAGE_SIZE);
		SubHeap * subheap;

		// the first allocation sets the fixed object size
		if (_object_size == 0)
			_object_size = size;

		// allocate the object in an available subheap
		while (!list_empty(&_avai_subheap_list)) {
			subheap = SubHeap::listToHeap(_avai_subheap_list.next);
			ptr = subheap->malloc();
			if (ptr != NULL)
				break;
			assert(subheap->getNumFree() == 0);
			list_move(subheap->getList(), &_full_subheap_list);
		}

		// if all subheaps are full, create a new one
		if (ptr == NULL) {
			subheap = createSubHeap();
			if (subheap != NULL) {
				ptr = subheap->malloc();
				assert(ptr != NULL);
			}
		}

		assert(ptr == NULL || getSubHeap(ptr) == subheap);
		sanityCheck();

		return ptr;
	}

	inline void free(void * ptr) {
		sanityCheck();

		SubHeap * subheap = getSubHeap(ptr);
		subheap->free(ptr);

		if (subheap->getNumFree() == 1)
			list_move(subheap->getList(), &_avai_subheap_list);
		else if (subheap->getNumFree() == subheap->getNumTotal())
			removeSubHeap(subheap);

		sanityCheck();
	}

	inline size_t getSize(void * ptr) {
		return getSubHeap(ptr)->getObjectSize();
	}

	void sanityCheck() {
#ifdef DEBUG
#if SANITY_CHECK
		if (!list_empty(&_full_subheap_list)) {
			list_head * node = _full_subheap_list.next;
			while (node != &_full_subheap_list) {
				SubHeap * subheap = SubHeap::listToHeap(node);
				assert(reinterpret_cast<size_t>(subheap) % PAGE_SIZE == 0);
				assert(subheap->getObjectSize() == _object_size);
				assert(subheap->getNumFree() == 0);
				assert(subheap->getList() == node);

				node = node->next;
			}
		}

		if (!list_empty(&_avai_subheap_list)) {
			list_head * node = _avai_subheap_list.next;
			while (node != &_avai_subheap_list) {
				SubHeap * subheap = SubHeap::listToHeap(node);
				assert(reinterpret_cast<size_t>(subheap) % PAGE_SIZE == 0);
				assert(subheap->getObjectSize() == _object_size);
				assert(subheap->getNumFree() <= subheap->getNumTotal());
				assert(subheap->getList() == node);

				node = node->next;
			}
		}
#endif
#endif
	}

private:

	list_head _full_subheap_list;
	list_head _avai_subheap_list;

	size_t _object_size;
	unsigned char _next_subheap_type;

	// create a new subheap with doubled size
	inline SubHeap * createSubHeap() {
		size_t subheap_size = PAGE_SIZE << (_next_subheap_type - 1);
		SubHeap * subheap = reinterpret_cast<SubHeap *>(SuperHeap::malloc(subheap_size, _next_subheap_type));
		assert(subheap != NULL);
		assert(reinterpret_cast<size_t>(subheap) % subheap_size == 0);

		if (subheap != NULL) {
			// initialize the new subheap
			subheap = new (subheap) SubHeap(subheap_size, _object_size);

			list_add(subheap->getList(), &_avai_subheap_list);
			if (_next_subheap_type < MaxSubHeapType)
				_next_subheap_type++;
		}
		return subheap;
	}

	// remove the subheap when it's empty
	inline void removeSubHeap(SubHeap * subheap) {
		assert(subheap->getNumFree() == subheap->getNumTotal());
		list_del(subheap->getList());
		SuperHeap::free(subheap);

		if (_next_subheap_type > 1)
			_next_subheap_type--;
	}

	// find the subheap from any address inside the subheap
	inline SubHeap * getSubHeap(void * ptr) {
		unsigned char type = SuperHeap::ptrToType(ptr);
		assert(type != 0);

		SubHeap * subheap = reinterpret_cast<SubHeap *>(reinterpret_cast<size_t>(ptr) & (PAGE_MASK << (type - 1)));
		return subheap;
	}

};	// end of class OneSizeHeap

};	// end of namespace VAM

#endif
