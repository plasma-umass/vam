// -*- C++ -*-

#ifndef _BITMAPREAP_H_
#define _BITMAPREAP_H_

#include "vamcommon.h"

#include "reapbase.h"

namespace VAM {

// BitmapReap: a reap that uses a bitmap to recycle freed objects
class BitmapReap : public ReapBase {

public:

	BitmapReap(size_t size, size_t object_size) {
		// calculate the bitmap size in bytes
		size_t max_num_objects = (size - sizeof(*this)) / object_size;
		size_t bitmap_size = (max_num_objects + SIZE_T_BIT - 1) / SIZE_T_BIT * sizeof(size_t);

		// the bitmap is right after us
		_bitmap = reinterpret_cast<size_t *>(this + 1);
		memset(_bitmap, 0, bitmap_size);

		// the allocation base is right after the bitmap
		size_t base_ptr = (reinterpret_cast<size_t>(_bitmap) + bitmap_size + sizeof(double) - 1) & ~(sizeof(double) - 1);
		assert(base_ptr % sizeof(double) == 0);

		// calculate the number of allocable objects
		size_t num_total = (reinterpret_cast<size_t>(this) + size - base_ptr) / object_size;
		initReapBase(object_size, num_total, num_total, base_ptr);
		_lowest_bit = num_total;
	}

	inline void * malloc() {
		void * ptr = ReapBase::malloc();

		if (ptr == NULL && _num_free > 0) {
			// find the first non-zero word of the bitmap
			size_t * bm;
			for (bm = _bitmap + _lowest_bit / SIZE_T_BIT; !*bm; bm++);

			// find the bit that indicates the first free object and clear it
			size_t mask = 1;
			size_t offset = (bm - _bitmap) * SIZE_T_BIT;
			while (!(*bm & mask)) {
				mask <<= 1;
				offset++;
			}
			assert(offset < _num_total);
			*bm ^= mask;

			ptr = reinterpret_cast<void *>(_base_ptr + _object_size * offset);
			_lowest_bit = offset + 1;

			_num_free--;
		}

		return ptr;
	}

	inline void free(void * ptr) {
		assert(_num_free < _num_total);
		assert((reinterpret_cast<size_t>(ptr) - _base_ptr) % _object_size == 0);

		size_t offset = (reinterpret_cast<size_t>(ptr) - _base_ptr) / _object_size;
		assert(offset >= 0 && offset < _num_total);
		assert((_bitmap[offset / SIZE_T_BIT] & (1 << (offset % SIZE_T_BIT))) == 0);
		_bitmap[offset / SIZE_T_BIT] |= 1 << (offset % SIZE_T_BIT);

		_num_free++;

		if (offset < _lowest_bit)
			_lowest_bit = offset;
	}

	inline static BitmapReap * listToHeap(list_head * list) {
		return container_of(list, BitmapReap, _list);
	}

	inline list_head * getList() {
		return &_list;
	}

private:

	size_t * _bitmap;
	size_t _lowest_bit;
	list_head _list;

};	// end of class BitmapReap

};	// end of namespace VAM

#endif
