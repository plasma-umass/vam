// -*- C++ -*-

#ifndef _PAGECLUSTERHEAP_H_
#define _PAGECLUSTERHEAP_H_

#include "vamcommon.h"

namespace VAM {

// PageClusterHeap: a page-oriented heap that allocates memory in fixed page cluster size
template<class SuperHeap>
class PageClusterHeap : public SuperHeap {

public:

	PageClusterHeap(size_t heap_size, size_t heap_alignment, size_t cluster_size)
		: _heap_size(heap_size),
		  _heap_alignment(heap_alignment),
		  _cluster_size(cluster_size),
		  _num_clusters(heap_size / cluster_size),
		  _num_free(_num_clusters),
		  _num_discarded(_num_clusters),
		  _num_pages(heap_size >> PAGE_SHIFT) {

		assert(heap_size != 0 && (heap_size & ~PAGE_MASK) == 0);
		assert(heap_alignment != 0 && (heap_alignment & ~PAGE_MASK) == 0 && (heap_alignment & (heap_alignment - 1)) == 0);
		assert(cluster_size != 0 && (cluster_size & ~PAGE_MASK) == 0 && heap_size % cluster_size == 0);
		assert(_num_clusters > 0);
		abort_on(heap_size == 0 || (heap_size & ~PAGE_MASK) != 0);
		abort_on(heap_alignment == 0 || (heap_alignment & ~PAGE_MASK) != 0 || (heap_alignment & (heap_alignment - 1)) != 0);
		abort_on(cluster_size == 0 || (cluster_size & ~PAGE_MASK) != 0 || heap_size % cluster_size != 0);
		abort_on(_num_clusters == 0);

		// allocate heap space
		_heap_space = SuperHeap::malloc(_heap_size, _heap_alignment);
		dbprintf("PageClusterHeap: _heap_size=%x _heap_alignment=%x _cluster_size=%x _heap_space=%p\n", _heap_size, _heap_alignment, _cluster_size, _heap_space);
		assert(_heap_space != NULL && (reinterpret_cast<size_t>(_heap_space) & ~PAGE_MASK) == 0);
		assert(reinterpret_cast<size_t>(_heap_space) % _heap_alignment == 0);
		abort_on(_heap_space == NULL);

		// allocate map space and initialize it
		size_t cluster_map_size = _num_clusters * sizeof(ClusterMap);
		cluster_map_size = (cluster_map_size + PAGE_SIZE - 1) & PAGE_MASK;
		_cluster_map = reinterpret_cast<ClusterMap *>(SuperHeap::malloc(cluster_map_size));
		dbprintf("PageClusterHeap: cluster_map_size=%d _cluster_map=%p\n", cluster_map_size, _cluster_map);
		assert(_cluster_map != NULL);
		abort_on(_cluster_map == NULL);
		memset(_cluster_map, 0, cluster_map_size);

		size_t page_map_size = _num_pages * sizeof(unsigned char);
		page_map_size = (page_map_size + PAGE_SIZE - 1) & PAGE_MASK;
		_page_map = reinterpret_cast<unsigned char *>(SuperHeap::malloc(page_map_size));
		dbprintf("PageClusterHeap: page_map_size=%d _page_map=%p\n", page_map_size, _page_map);
		assert(_page_map != NULL);
		abort_on(_page_map == NULL);
		memset(_page_map, 0, page_map_size);

		// initially all page clusters are free and discarded (because the PTEs are empty at this time)
		INIT_LIST_HEAD(&_free_list);
		assert(list_empty(&_free_list));
		for (int i = 0; i < _num_clusters; i++) {
			_cluster_map[i].setFlags(CLUSTER_FREE | CLUSTER_DISCARDED);
			list_add_tail(&_cluster_map[i].list, &_free_list);
		}

		sanityCheck();
	}

	~PageClusterHeap() {
		if (_heap_space != NULL)
			SuperHeap::free(_heap_space);

		if (_cluster_map != NULL)
			SuperHeap::free(_cluster_map);

		if (_page_map != NULL)
			SuperHeap::free(_page_map);
	}

	// allocate a page cluster
	inline void * malloc(size_t size) {
		sanityCheck();

		void * ptr = NULL;
		assert(size == _cluster_size);
		abort_on(size != _cluster_size);

		if (_num_free > 0) {
			assert(!list_empty(&_free_list));

			list_head * node = _free_list.next;
			list_del(node);
			_num_free--;

			ClusterMap * map = list_entry(node, ClusterMap, list);
			assert(map->flagsOn(CLUSTER_FREE));
			map->clearFlags(CLUSTER_FREE);
			if (map->flagsOn(CLUSTER_DISCARDED))
				_num_discarded--;

			ptr = clusterMapToPtr(map);
		}
		else {
			assert(list_empty(&_free_list));
		}

		sanityCheck();

		return ptr;
	}

	// free a page cluster
	inline void free(void * ptr) {
		sanityCheck();

		assert(reinterpret_cast<size_t>(ptr) >= reinterpret_cast<size_t>(_heap_space));
		assert(reinterpret_cast<size_t>(ptr) < reinterpret_cast<size_t>(_heap_space) + _heap_size);
		assert(((reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(_heap_space)) % _cluster_size) == 0);

		ClusterMap * map = ptrToClusterMap(ptr);
		list_add(&map->list, &_free_list);
		_num_free++;

		assert(map->flagsOff(CLUSTER_FREE));
		map->setFlags(CLUSTER_FREE);

#ifdef AGGRESSIVE_DISCARD
		int rc = madvise(ptr, _cluster_size, MADV_DONTNEED);
		dbprintf("PageClusterHeap: madvise(%p, %u, MADV_DONTNEED) returned %d\n", ptr, _cluster_size, rc);

		map->setFlags(CLUSTER_DISCARDED);
		_num_discarded++;
#else
		map->clearFlags(CLUSTER_DISCARDED);
#endif

		sanityCheck();
	}

	inline int isDiscarded(void * addr) {
		ClusterMap * page = ptrToClusterMap(addr);
		return page->flagsOn(CLUSTER_DISCARDED);
	}

	inline size_t getHeapSize() {
		return _heap_size;
	}

	inline void * getHeapAddress() {
		return _heap_space;
	}

	inline bool isEmpty() {
		return _num_free == _num_clusters;
	}

	inline bool isFull() {
		return _num_free == 0;
	}

	void sanityCheck() {
#ifdef DEBUG
#if SANITY_CHECK
		size_t num_free = 0;
		size_t num_discarded = 0;

		list_head * node = _free_list.next;
		while (node != &_free_list) {
			assert(node->prev->next == node && node->next->prev == node);

			ClusterMap * map = list_entry(node, ClusterMap, list);
			void * ptr = clusterMapToPtr(map);
			assert(((reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(_heap_space)) % _cluster_size) == 0);
			assert(map->flagsOn(CLUSTER_FREE));
			assert(num_discarded == 0 || map->flagsOn(CLUSTER_DISCARDED));

			num_free++;
			if (map->flagsOn(CLUSTER_DISCARDED))
				num_discarded++;

			node = node->next;
		}

		assert(num_free == _num_free);
		assert(num_discarded == _num_discarded);
#endif
#endif
	}

	inline int flagsOn(void * ptr, unsigned f) {
		ClusterMap * page = ptrToClusterMap(ptr);
		return page->flagsOn(f);
	}

	inline int flagsOff(void * ptr, unsigned f) {
		ClusterMap * page = ptrToClusterMap(ptr);
		return page->flagsOff(f);
	}

private:

	enum {
		CLUSTER_FREE		= 0x00000001,	// is this page cluster free?
		CLUSTER_DISCARDED	= 0x00000002,	// has this page cluster been discarded?
	};

	struct ClusterMap {
		unsigned int flags;			// page cluster status
		list_head list;				// doubly-linked list of free page clusters

		inline void setFlags(unsigned int f) {
			flags |= f;
		}

		inline void clearFlags(unsigned int f) {
			flags &= ~f;
		}

		inline int flagsOn(unsigned int f) {
			return (flags & f) == f;
		}

		inline int flagsOff(unsigned int f) {
			return (flags & f) == 0;
		}

	};	// end of struct ClusterMap

	size_t _heap_size;
	size_t _heap_alignment;
	size_t _cluster_size;

	size_t _num_clusters;
	size_t _num_free;				// number of free page clusters
	size_t _num_discarded;			// number of free page clusters that have been discarded
	size_t _num_pages;

	void * _heap_space;
	list_head _free_list;
	ClusterMap * _cluster_map;
	unsigned char * _page_map;

	inline void * clusterMapToPtr(ClusterMap * p) {
		//dbprintf("getPagePtr(p=%p): _cluster_map=%p _heap_space=%p\n", p, _cluster_map, _heap_space);
		assert(p - _cluster_map >= 0 && p - _cluster_map < _num_clusters);
		return reinterpret_cast<void *>(reinterpret_cast<size_t>(_heap_space) + (p - _cluster_map) * _cluster_size);
	}

	inline ClusterMap * ptrToClusterMap(void * ptr) {
		assert((reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(_heap_space)) % _cluster_size == 0);
		size_t index = (reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(_heap_space)) / _cluster_size;
		assert(index < _num_clusters);
		return &_cluster_map[index];
	}

};	// end of class PageClusterHeap

};	// end of namespace VAM

#endif
