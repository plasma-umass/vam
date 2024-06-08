// -*- C++ -*-

#ifndef _BITMAPCACHINGREAP_H_
#define _BITMAPCACHINGREAP_H_

#include "vamcommon.h"

#include "reapbase.h"

namespace VAM {

// BitmapCachingReap: a reap that uses a bitmap to recycle freed objects and a cache to minimize bitmap manipulation
class BitmapCachingReap : public ReapBase {

public:

	BitmapCachingReap(size_t size, size_t object_size) {
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

		_num_cached = 0;
	}

	inline void * malloc() {
		void * ptr = ReapBase::malloc();

		if (ptr == NULL && _num_free > 0) {
			if (_num_cached > 0) {
				// allocate from the cache
				assert(_num_cached <= CACHE_SIZE);
				ptr = reinterpret_cast<void *>(_base_ptr + _cached_offsets[--_num_cached]);
			}
			else {
				// refill the cache

				// find the first non-zero word of the bitmap
				size_t * bm;
				for (bm = _bitmap + _lowest_bit / SIZE_T_BIT; !*bm; bm++);

				// find the bits that indicate free objects and put them into the cache
				size_t mask = *bm;
				size_t offset = (bm + 1 - _bitmap) * SIZE_T_BIT;
				_lowest_bit = offset;

				do {
					offset--;

					if (mask & (1UL << (SIZE_T_BIT - 1))) {
						assert(offset < _num_total);
						assert(_num_cached < CACHE_SIZE);
						_cached_offsets[_num_cached++] = _object_size * offset;
					}

					mask <<= 1;
				} while (mask);

				*bm = 0;

				assert(_num_cached > 0);
				assert(_num_cached <= _num_free);

				ptr = reinterpret_cast<void *>(_base_ptr + _cached_offsets[--_num_cached]);
			}

			_num_free--;
		}

		return ptr;
	}

	inline void free(void * ptr) {
		assert(_num_free < _num_total);
		assert(_num_cached < CACHE_SIZE);
		assert((reinterpret_cast<size_t>(ptr) - _base_ptr) % _object_size == 0);

		// put the free object into the cache
		_cached_offsets[_num_cached++] = reinterpret_cast<size_t>(ptr) - _base_ptr;

		// empty the cache if it's full
		if (_num_cached == CACHE_SIZE) {

			for (size_t i = 0; i < CACHE_SIZE; i++) {
				size_t offset = _cached_offsets[i] / _object_size;
				assert(_cached_offsets[i] % _object_size == 0);
				assert(offset >= 0 && offset < _num_total);
				assert((_bitmap[offset / SIZE_T_BIT] & (1 << (offset % SIZE_T_BIT))) == 0);

				_bitmap[offset / SIZE_T_BIT] |= 1 << (offset % SIZE_T_BIT);

				if (offset < _lowest_bit)
					_lowest_bit = offset;
			}
			_num_cached = 0;
		}

		_num_free++;
	}

	inline static BitmapCachingReap * listToHeap(list_head * list) {
		return container_of(list, BitmapCachingReap, _list);
	}

	inline list_head * getList() {
		return &_list;
	}

private:

	enum {
		CACHE_SIZE = SIZE_T_BIT,
	};

	size_t * _bitmap;
	size_t _lowest_bit;
	size_t _num_cached;
	unsigned short _cached_offsets[CACHE_SIZE];
	list_head _list;

};	// end of class BitmapCachingReap

};	// end of namespace VAM

#endif
