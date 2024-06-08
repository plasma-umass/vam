// -*- C++ -*-

#ifndef _CACHINGHEAP_H_
#define _CACHINGHEAP_H_

#include "vamcommon.h"

namespace VAM {

// CachingHeap: a heap that caches objects of a fixed size allocated from SuperHeap
template <class SuperHeap>
class CachingHeap : public SuperHeap {

public:

	CachingHeap()
		: _cached_objects(NULL),
		  _num_cached(0),
		  _target_cache_size(1) {
	}

	inline void * malloc(size_t size) {
		void * ptr;

		// allocate from the cache
		if (_num_cached > 0) {
			assert(_cached_objects != NULL);

			ptr = _cached_objects;
			_cached_objects = _cached_objects->next;
			_num_cached--;
		}
		// refill the cache
		else {
			assert(_cached_objects == NULL);

			ptr = SuperHeap::malloc(size);
			if (ptr == NULL)
				return NULL;

			if (_target_cache_size < MAX_CACHE_SIZE)
				_target_cache_size <<= 1;

			CachedObject * last_allocated = reinterpret_cast<CachedObject *>(SuperHeap::malloc(size));
			_cached_objects = last_allocated;
			while (last_allocated != NULL && _num_cached++ < _target_cache_size) {
				CachedObject * this_allocated = reinterpret_cast<CachedObject *>(SuperHeap::malloc(size));
				last_allocated->next = this_allocated;
				last_allocated = this_allocated;
			}

			if (last_allocated != NULL)
				last_allocated->next = NULL;
		}

		return ptr;
	}

	inline void free(void * ptr) {
		// free to the cache
		if (_num_cached < MAX_CACHE_SIZE) {
			CachedObject * this_freed = reinterpret_cast<CachedObject *>(ptr);
			this_freed->next = _cached_objects;
			_cached_objects = this_freed;
			_num_cached++;
		}
		// clean the cache
		else {
			SuperHeap::free(ptr);

			if (_target_cache_size > 1)
				_target_cache_size >>= 1;

			while (_num_cached > _target_cache_size) {
				CachedObject * free_one = _cached_objects;
				assert(free_one != NULL);

				_cached_objects = _cached_objects->next;
				_num_cached--;

				SuperHeap::free(free_one);
			}
		}
	}

private:

	enum {
		MAX_CACHE_SIZE = 32,
	};

	struct CachedObject {
		CachedObject * next;
	};

	CachedObject * _cached_objects;
	size_t _num_cached;
	size_t _target_cache_size;

};	// end of class CachingHeap

};	// end of namespace VAM

#endif
