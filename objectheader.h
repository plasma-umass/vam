// -*- C++ -*-

#ifndef _OBJECTHEADER_H_
#define _OBJECTHEADER_H_

namespace VAM {

// ObjectHeader: object header
struct ObjectHeader {

	unsigned _prev_free : 1;
	size_t _prev_size : 31;
	size_t _size;

	inline static ObjectHeader * getHeader(void * ptr) {
		return reinterpret_cast<ObjectHeader *>(ptr) - 1;
	}

	inline void * getObject() {
		return this + 1;
	}

	inline ObjectHeader * getPrevHeader() {
		return reinterpret_cast<ObjectHeader *>(reinterpret_cast<size_t>(this) - _prev_size - sizeof(ObjectHeader));
	}

	inline ObjectHeader * getNextHeader() {
		return reinterpret_cast<ObjectHeader *>(reinterpret_cast<size_t>(this) + sizeof(ObjectHeader) + _size);
	}

	inline unsigned isFree() {
		return getNextHeader()->_prev_free == 1;
	}

	inline void setFree(unsigned prev_free) {
		getNextHeader()->_prev_free = prev_free;
	}

};	// end of struct ObjectHeader

};	// end of namespace VAM

#endif
