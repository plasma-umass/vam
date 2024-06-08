// -*- C++ -*-

#ifndef _REAPBASE_H_
#define _REAPBASE_H_

#include "vamcommon.h"

namespace VAM {

// ReapBase: base of reaps that allocate objects of a fixed size by pointer bumping
class ReapBase {

public:

	inline void * malloc() {
		void * ptr = NULL;
		if (_num_bumped < _num_total) {
			ptr = reinterpret_cast<void *>(_bump_ptr);
			_bump_ptr += _object_size;
			_num_bumped++;
			_num_free--;
		}
		return ptr;
	}

	inline size_t getObjectSize() {
		return _object_size;
	}

	inline size_t getNumTotal() {
		return _num_total;
	}

	inline size_t getNumFree() {
		return _num_free;
	}

protected:

	size_t _object_size;
	size_t _num_total;
	size_t _num_free;
	size_t _base_ptr;

	inline void initReapBase(size_t object_size, size_t num_total, size_t num_free, size_t base_ptr) {
		_object_size = object_size;
		_num_total = num_total;
		_num_free = num_free;
		_base_ptr = base_ptr;

		_num_bumped = 0;
		_bump_ptr = base_ptr;
	}

private:
	size_t _num_bumped;
	size_t _bump_ptr;

};	// end of class ReapBase

};	// end of namespace VAM

#endif
