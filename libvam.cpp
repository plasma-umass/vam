// -*- C++ -*-

#include <new>
#include "vam.h"

class TheCustomHeapType : public CustomAllocator {};

inline static TheCustomHeapType * getCustomHeap() {
	static char thBuf[sizeof(TheCustomHeapType)];
	static TheCustomHeapType * th = new (thBuf) TheCustomHeapType;
	return th;
}

#if defined(_WIN32)
#pragma warning(disable:4273)
#endif

#include "wrapper.cpp"
