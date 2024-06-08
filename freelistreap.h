// -*- C++ -*-

#ifndef _FREELISTREAP_H_
#define _FREELISTREAP_H_

#include "vamcommon.h"

#include "reapbase.h"

namespace VAM {

// FreelistReap: a reap that uses a freelist to recycle freed objects
class FreelistReap : public ReapBase {

public:

	FreelistReap(size_t size, size_t object_size) {
		// the allocation base is right after us
		size_t base_ptr = (reinterpret_cast<size_t>(this + 1) + sizeof(double) - 1) & ~(sizeof(double) - 1);
		assert(base_ptr % sizeof(double) == 0);

		// calculate the number of allocable objects
		size_t num_total = (size - sizeof(*this)) / object_size;

		initReapBase(object_size, num_total, num_total, base_ptr);
		_freelist = NULL;
	}

	inline void * malloc() {
		void * ptr = ReapBase::malloc();

		if (ptr == NULL && _num_free > 0) {
			assert(_freelist != NULL);
			ptr = _freelist;
			_freelist = _freelist->next;
			_num_free--;
		}

		return ptr;
	}

	inline void free(void * ptr) {
		assert(_num_free < _num_total);
		assert((reinterpret_cast<size_t>(ptr) - _base_ptr) % _object_size == 0);

		FreeObject * free_obj = reinterpret_cast<FreeObject *>(ptr);
		free_obj->next = _freelist;
		_freelist = free_obj;

		_num_free++;
	}

	inline static FreelistReap * listToHeap(list_head * list) {
		return container_of(list, FreelistReap, _list);
	}

	inline list_head * getList() {
		return &_list;
	}

private:

	struct FreeObject {
		FreeObject * next;
	};

	FreeObject * _freelist;
	list_head _list;

};	// end of class FreelistReap

};	// end of namespace VAM

#endif
