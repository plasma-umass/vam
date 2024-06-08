// -*- C++ -*-

#ifndef _PARTITIONHEAP_H_
#define _PARTITIONHEAP_H_

#include "vamcommon.h"

namespace VAM {

// forward declaration
template<unsigned char PartitionTypes, size_t PartitionSize, class SubHeap>
class TheOnePartitionHeap;

// PartitionHeap: a heap that partitions the address space
template<unsigned char PartitionTypes, size_t PartitionSize, class SubHeap>
class PartitionHeap {
	friend class TheOnePartitionHeap<PartitionTypes, PartitionSize, SubHeap>;

public:

	PartitionHeap() {
		dbprintf("ParitionHeap: PartitionTypes=%u PartitionSize=%u NumPartitions=%u sizeof(_subheap_map)=%u\n", PartitionTypes, PartitionSize, NumPartitions, sizeof(_subheap_map));

		memset(_subheap_list, 0 ,sizeof(_subheap_list));
		memset(_subheap_map, 0, sizeof(_subheap_map));
		memset(_subheap_pool, 0, sizeof(_subheap_pool));

		for (size_t i = 0; i < NumPartitions; i++) {
			_type_map[i] = INVALID_TYPE;
		}

		for (unsigned char type = 0; type < PartitionTypes; type++) {
			INIT_LIST_HEAD(&_subheap_list[type].full);
			INIT_LIST_HEAD(&_subheap_list[type].avai);
		}

		_unused_subheaps = &_subheap_pool[0];
		for (size_t i = 0; i < NumPartitions - 1; i++) {
			_subheap_pool[i].next_unused = &_subheap_pool[i + 1];
		}
		assert(_subheap_pool[NumPartitions - 1].next_unused == NULL);

		sanityCheck();
	}

	inline void * malloc(size_t size, unsigned char type) {
		sanityCheck();

		void * ptr = NULL;
		SubHeapList * list = &_subheap_list[type];
		list_head * node;
		SubHeapMap * map;

		assert(type < PartitionTypes);

		// "regular" allocations
		if (size <= PartitionSize) {

			// allocate from the available list
			while (!list_empty(&list->avai)) {
				node = list->avai.next;
				map = list_entry(node, SubHeapMap, list);

				ptr = map->heap->malloc(size);
				if (ptr != NULL) {
					sanityCheck();
					return ptr;
				}

				list_del(node);
				list_add(node, &list->full);
				map->status = SUBHEAP_FULL;
			}

			// no subheap available for allocation, create one
			if (_unused_subheaps != NULL) {
				SubHeapInstance * instance = _unused_subheaps;
				SubHeap * heap = new (instance->space) SubHeap(PartitionSize, PartitionSize, size);
				assert(heap == reinterpret_cast<SubHeap *>(&instance->space));

				void * heap_address = heap->getHeapAddress();
				assert(heap_address != NULL);
				if (heap_address == NULL) {
					heap->~SubHeap();
					return NULL;
				}

				assert(heap->isEmpty());

				ptr = heap->malloc(size);
				assert(ptr != NULL);
				if (ptr == NULL) {
					heap->~SubHeap();
					return NULL;
				}
				assert(ptrToPartition(ptr) == ptrToPartition(heap_address));

				_type_map[ptrToPartition(heap_address)] = type;
				map = ptrToMap(heap_address);
				map->heap = heap;

				list_add(&map->list, &list->avai);
				map->status = SUBHEAP_AVAI;

				_unused_subheaps = _unused_subheaps->next_unused;
			}
		}
		// huge allocations that occupy more than one partition
		else {

			// create a special subheap that can only allocate one huge object
			if (_unused_subheaps != NULL) {
				SubHeapInstance * instance = _unused_subheaps;
				SubHeap * heap = new (instance->space) SubHeap(size, PAGE_SIZE, size);
				assert(heap == reinterpret_cast<SubHeap *>(&instance->space));

				void * heap_address = heap->getHeapAddress();
				assert(heap_address != NULL);
				if (heap_address == NULL) {
					heap->~SubHeap();
					return NULL;
				}

				assert(heap->isEmpty());

				ptr = heap->malloc(size);
				assert(ptr != NULL);
				if (ptr == NULL) {
					heap->~SubHeap();
					return NULL;
				}
				assert(ptrToPartition(ptr) == ptrToPartition(heap_address));

				_type_map[ptrToPartition(heap_address)] = type;
				map = ptrToMap(heap_address);
				map->heap = heap;

				// this subheap should be immediately full after the allocation
				assert(map->heap->isFull());

				list_add(&map->list, &list->full);
				map->status = SUBHEAP_FULL;

				_unused_subheaps = _unused_subheaps->next_unused;
			}
		}

		sanityCheck();

		return ptr;
	}

	inline void free(void * ptr) {
		sanityCheck();

		unsigned char type = ptrToType(ptr);
		assert(type < PartitionTypes);
		if (type == INVALID_TYPE)
			return;

		SubHeapList * list = &_subheap_list[type];
		SubHeapMap * map = ptrToMap(ptr);
		map->heap->free(ptr);

		// destroy the subheap if it's empty and not the only one left
		if (map->heap->isEmpty() && (map->list.prev != &list->avai || map->list.next != &list->avai)) {
			list_del(&map->list);

			void * heap_address = map->heap->getHeapAddress();
			assert(heap_address != NULL);
			assert(map == ptrToMap(heap_address));
			_type_map[ptrToPartition(heap_address)] = INVALID_TYPE;

			map->heap->~SubHeap();

			SubHeapInstance * instance = container_of(reinterpret_cast<const char (*) [sizeof(SubHeap)]>(map->heap), SubHeapInstance, space);

//			SubHeapInstance * instance = reinterpret_cast<SubHeapInstance *>(reinterpret_cast<size_t>(map->heap) - reinterpret_cast<size_t>(&reinterpret_cast<SubHeapInstance *>(0)->space));

			instance->next_unused = _unused_subheaps;
			_unused_subheaps = instance;

			memset(map, 0, sizeof(SubHeapMap));
		}
		// move the subheap if necessary
		else if (map->status == SUBHEAP_FULL) {
			list_move(&map->list, &list->avai);
			map->status = SUBHEAP_AVAI;
		}

		sanityCheck();
	}

	void sanityCheck() {
#ifdef DEBUG
#if 1//SANITY_CHECK
		size_t num_avai = 0;
		size_t num_full = 0;
		size_t num_unused_instances = 0;
		size_t num_unused_partitions = 0;

		for (unsigned char type = 0; type < PartitionTypes; type++) {
			SubHeapList * list = &_subheap_list[type];
			list_head * node;

			node = list->avai.next;
			while (node != &list->avai) {
				assert(node->prev->next == node && node->next->prev == node);

				SubHeapMap * map = list_entry(node, SubHeapMap, list);
				assert(map->status == SUBHEAP_AVAI);

				void * heap_address = map->heap->getHeapAddress();
				assert(ptrToType(heap_address) == type);

				node = node->next;
				num_avai++;
			}

			node = list->full.next;
			while (node != &list->full) {
				assert(node->prev->next == node && node->next->prev == node);

				SubHeapMap * map = list_entry(node, SubHeapMap, list);
				assert(map->status == SUBHEAP_FULL);
				assert(map->heap->isFull());

				void * heap_address = map->heap->getHeapAddress();
				assert(ptrToType(heap_address) == type);

				node = node->next;
				num_full++;
			}
		}

		SubHeapInstance * instance = _unused_subheaps;
		while (instance != NULL) {
			instance = instance->next_unused;
			num_unused_instances++;
		}

		for (size_t i = 0; i < NumPartitions; i++) {
			if (_subheap_map[i].status == 0) {
				assert(_type_map[i] == INVALID_TYPE);
				num_unused_partitions++;
			}
		}

		assert(num_unused_instances == num_unused_partitions);
		assert(num_avai + num_full + num_unused_partitions == NumPartitions);
#endif
#endif
	}

private:

	enum {
		NumPartitions = (1UL << 31) / PartitionSize << 1,
		SUBHEAP_FULL = 1,
		SUBHEAP_AVAI = 2,
		INVALID_TYPE = 0xFF,
	};

	struct SubHeapList {
		list_head full;
		list_head avai;
	};

	struct SubHeapMap {
		SubHeap * heap;
		unsigned int status;
		list_head list;
	};

	struct SubHeapInstance {
		char space[sizeof(SubHeap)];
		SubHeapInstance * next_unused;
	};

	SubHeapList _subheap_list[PartitionTypes];
	unsigned char _type_map[NumPartitions];
	SubHeapMap _subheap_map[NumPartitions];
	SubHeapInstance _subheap_pool[NumPartitions];
	SubHeapInstance * _unused_subheaps;

	inline size_t ptrToPartition(void * ptr) {
		return reinterpret_cast<size_t>(ptr) / PartitionSize;
	}

	inline unsigned char ptrToType(void * ptr) {
		return _type_map[ptrToPartition(ptr)];
	}

	inline SubHeapMap * ptrToMap(void * ptr) {
		return &_subheap_map[ptrToPartition(ptr)];
	}

};	// end of class PartitionHeap

// TheOnePartitionHeap: singleton of PartitionHeap
template<unsigned char PartitionTypes, size_t PartitionSize, class SubHeap>
class TheOnePartitionHeap {

public:

	TheOnePartitionHeap() {
		static PartitionHeap<PartitionTypes, PartitionSize, SubHeap> partition_heap;
		_heap = &partition_heap;
	}

	inline void * malloc(size_t size, unsigned char type = 0) {
		return _heap->malloc(size, type);
	}

	inline void free(void * ptr) {
		_heap->free(ptr);
	}

protected:

	inline unsigned char ptrToType(void * ptr) {
		return _heap->ptrToType(ptr);
	}

private:

	PartitionHeap<PartitionTypes, PartitionSize, SubHeap> * _heap;

};	// end of class TheOnePartitionHeap

};	// end of namespace VAM

#endif
