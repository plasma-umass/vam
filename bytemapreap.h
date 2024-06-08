// -*- C++ -*-

#ifndef _BYTEMAPREAP_H_
#define _BYTEMAPREAP_H_

#include "vamcommon.h"

#include "reapbase.h"

namespace VAM {

// BytemapReap: a reap that uses a bytemap to recycle freed objects
class BytemapReap : public ReapBase {

public:

	BytemapReap(size_t size, size_t object_size) {
		// calculate the bytemap size in bytes
		size_t max_num_objects = (size - sizeof(*this)) / object_size;
		size_t bytemap_size = max_num_objects * sizeof(*_bytemap);

		// the bytemap is right after us
		_bytemap = reinterpret_cast<unsigned char *>(this + 1);
		memset(_bytemap, 0, bytemap_size);

		// the allocation base is right after the bytemap
		size_t base_ptr = (reinterpret_cast<size_t>(_bytemap) + bytemap_size + sizeof(double) - 1) & ~(sizeof(double) - 1);
		assert(base_ptr % sizeof(double) == 0);

		// calculate the number of allocable objects
		size_t num_total = (reinterpret_cast<size_t>(this) + size - base_ptr) / object_size;
		initReapBase(object_size, num_total, num_total, base_ptr);
		_lowest_byte = num_total;
	}

	inline void * malloc() {
		void * ptr = ReapBase::malloc();

		if (ptr == NULL && _num_free > 0) {
			unsigned char * bm;
			for (bm = _bytemap + _lowest_byte; !*bm; bm++);
			assert(*bm == 1);
			*bm = 0;

			size_t offset = bm - _bytemap;
			assert(offset < _num_total);

			ptr = reinterpret_cast<void *>(_base_ptr + _object_size * offset);
			_lowest_byte = offset + 1;

			_num_free--;
		}

		return ptr;
	}

	inline void free(void * ptr) {
		assert(_num_free < _num_total);
		assert((reinterpret_cast<size_t>(ptr) - _base_ptr) % _object_size == 0);

		size_t offset = (reinterpret_cast<size_t>(ptr) - _base_ptr) / _object_size;
		assert(offset >= 0 && offset < _num_total);
		assert(_bytemap[offset] == 0);
		_bytemap[offset] = 1;

		_num_free++;

		if (offset < _lowest_byte)
			_lowest_byte = offset;
	}

	inline static BytemapReap * listToHeap(list_head * list) {
		return container_of(list, BytemapReap, _list);
	}

	inline list_head * getList() {
		return &_list;
	}

private:

	unsigned char * _bytemap;
	size_t _lowest_byte;
	list_head _list;

};	// end of class BytemapReap

};	// end of namespace VAM

#endif
