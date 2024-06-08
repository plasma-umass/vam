// -*- C++ -*-

#ifndef _VAM_H_
#define _VAM_H_

#define SANITY_CHECK		1
#define DB_PRINT_TO_FILE	1

namespace VAM {
	enum {
		LOW_FREQ_TYPE = 0,
		USE_HEADER_TYPE = 0,
	};
};

#include "vamcommon.h"

#include "alignedmmapheap.h"
#include "bitmapcachingreap.h"
#include "bitmapreap.h"
#include "bytemapreap.h"
#include "cachingheap.h"
#include "freelistreap.h"
#include "frequencyheap.h"
#include "onesizeheap.h"
#include "pageclusterheap.h"
#include "partitionheap.h"
#include "reapbase.h"
#include "segfitheap.h"
#include "segsizeheap.h"
#include "splitcoalesceheap.h"
#include "twoheap.h"

#include "heaplayers.h"

using namespace VAM;
using namespace HL;

#ifdef DEBUG
#if SANITY_CHECK
template<class SuperHeap>
class SCHeap : public SanityCheckHeap<SuperHeap> {};
#else
template<class SuperHeap>
class SCHeap : public SuperHeap {};
#endif
#else
template<class SuperHeap>
class SCHeap : public SuperHeap {};
#endif

// some tunable parameters

#define PARTITION_SIZE		(8 * 1024 * 1024)
#define MAX_DEDICATED_SIZE	1024
#define MAX_PAGE_ORDER		5

#define WORKHORSE_HEAP		BitmapCachingReap
//#define WORKHORSE_HEAP		BitmapReap
//#define WORKHORSE_HEAP		BytemapReap
//#define WORKHORSE_HEAP		FreelistReap

#ifdef THREAD_SAFE
template<class SuperHeap>
class ThreadSafeHeap : public LockedHeap<SpinLockType, SuperHeap> {};
#else
template<class SuperHeap>
class ThreadSafeHeap : public SuperHeap {};
#endif

inline bool high_freq_reached(size_t size, size_t count) {
	return size * count > PAGE_SIZE;
}

// here is how our Vam allocator is composed

typedef TheOnePartitionHeap<MAX_PAGE_ORDER + 1, PARTITION_SIZE, PageClusterHeap<TheOneAlignedMmapHeap> > PageSourceHeap;

typedef SplitCoalesceHeap<SegFitHeap<MAX_DEDICATED_SIZE * 2>, PageSourceHeap, PARTITION_SIZE> RegularSizeHeap;

typedef ThreadSafeHeap<TwoHeap<RegularSizeHeap, PageSourceHeap, PARTITION_SIZE> > LowFreqHeap;

//typedef SegSizeHeap<MAX_DEDICATED_SIZE, ThreadSafeHeap<CachingHeap<OneSizeHeap<MAX_PAGE_ORDER, WORKHORSE_HEAP, PageSourceHeap> > > > HighFreqHeap;
typedef SegSizeHeap<MAX_DEDICATED_SIZE, ThreadSafeHeap<OneSizeHeap<MAX_PAGE_ORDER, WORKHORSE_HEAP, PageSourceHeap> > > HighFreqHeap;

typedef FrequencyHeap<MAX_DEDICATED_SIZE, high_freq_reached, LowFreqHeap, HighFreqHeap> VamHeap;


#ifdef MEMORY_TRACE

#define MEMTRACE_IN_MEMLIB
#include "tools/memtrace.h"
template<class SuperHeap>
class DenyDlsymHeap : public SuperHeap {
public:
	void * malloc(size_t size) {
		if (in_dlsym)
			return NULL;
		else
			return SuperHeap::malloc(size);
	}
};

class CustomAllocator : public ANSIWrapper<DenyDlsymHeap<VamHeap> > {};

#else
class CustomAllocator : public ANSIWrapper<VamHeap> {};
#endif

volatile int anyThreadCreated = 0;


#endif
