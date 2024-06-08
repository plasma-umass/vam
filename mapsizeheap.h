// -*- C++ -*-

#ifndef _MAPSIZEHEAP_H_
#define _MAPSIZEHEAP_H_

#include <map>

#include "vamcommon.h"
#include "remembersizeheap.h"
#include "heaplayers.h"

using namespace HL;

namespace VAM {

struct PtrCmp {
	bool operator()(void * const a, void * const b) const {
		return (size_t) a < (size_t) b;
	}
};

// FIXME: 16 = size of ZoneHeap header, 8 = size of RememberSizeHeap header
class STLHeap :
    public FreelistHeap<ZoneHeap<RememberSizeHeap<PrivateMmapHeap>, PAGE_SIZE - 16 - 8> > {};

typedef pair<void * const, size_t> PtrSizePair;
typedef map<void * const, size_t, PtrCmp, STLAllocator<PtrSizePair, STLHeap> > PtrSizeMap;

// MapSizeHeap: a heap that stores sizes of allocated objects in a map and frees with the size information
template<class SuperHeap>
class MapSizeHeap : public SuperHeap {

public:

  inline void * malloc(size_t size) {
    void * ptr = SuperHeap::malloc(size);
    _map_lock.lock();
    _ptr_size_map[ptr] = size;
    _map_lock.unlock();
    return ptr;
  }
  
  inline void free(void * ptr) {
    _map_lock.lock();
    SuperHeap::free(ptr, _ptr_size_map[(void * const) ptr]);
    PtrSizeMap::iterator i;
    i = _ptr_size_map.find((void * const) ptr);
    _ptr_size_map.erase(i);
    _map_lock.unlock();
  }
  
  inline size_t getSize(void * ptr) {
    _map_lock.lock();
    size_t size = _ptr_size_map[(void * const) ptr];
    _map_lock.unlock();
    return size;
  }
  
protected:
  
  PtrSizeMap _ptr_size_map;
  SpinLockType _map_lock;
  
};	// end of class MapSizeHeap

};	// end of namespace VAM

#endif
