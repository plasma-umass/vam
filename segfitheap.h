// -*- C++ -*-

#ifndef _SEGFITHEAP_H_
#define _SEGFITHEAP_H_

#include "objectheader.h"

namespace VAM {

// SegFitHeap: a heap that allocates objects with headers and performs splitting and coalescing
template<size_t MaxDedicatedSize>
class SegFitHeap {

public:

	SegFitHeap() {
		assert((NUM_DEDICATED_SIZES & (NUM_DEDICATED_SIZES - 1)) == 0);
		assert(sizeof(ObjectHeader) == sizeof(double));

		// initially all freelists are empty
		for (int i = 0; i < NUM_DEDICATED_SIZES; i++) {
			INIT_LIST_HEAD(&_dedicated_size_list[i]);
		}
		INIT_LIST_HEAD(&_large_size_list);

		memset(_dedicated_size_counter, 0, sizeof(_dedicated_size_counter));
		memset(_dedicated_size_bitmap, 0, sizeof(_dedicated_size_bitmap));

		sanityCheck();
	}

	inline void * malloc(size_t size) {
		sanityCheck();

		void * ptr = NULL;
		list_head * node;
		size_t index = SIZE_TO_INDEX(size);

		// first try to find a best fit in freelists for dedicated sizes
		if (index < NUM_DEDICATED_SIZES) {
			// fast path, hopefully
			if (_dedicated_size_counter[index] > 0) {

				// update the bitmap
				if (--_dedicated_size_counter[index] == 0) {
					assert(_dedicated_size_bitmap[index / SIZE_T_BIT] & (1 << (index % SIZE_T_BIT)));
					_dedicated_size_bitmap[index / SIZE_T_BIT] ^= 1 << (index % SIZE_T_BIT);
				}
			}
			// use the bitmap to find a best fit
			else {
				assert((_dedicated_size_bitmap[index / SIZE_T_BIT] & (1 << (index % SIZE_T_BIT))) == 0);

				size_t * bm = &_dedicated_size_bitmap[index / SIZE_T_BIT];

				size_t offset = index % SIZE_T_BIT;
				size_t mask;

				// check if there is a fit in the same cluster
				if (*bm & (~0UL << offset)) {
					mask = 1 << ++offset;
					while (!(*bm & mask)) {
						mask <<= 1;
						offset++;
						assert(offset < SIZE_T_BIT);
					}
				}
				// look for a fit in higher entries
				else {
					// find the first non-zero word
					for (bm++; !*bm && bm < &_dedicated_size_bitmap[NUM_DEDICATED_SIZES / SIZE_T_BIT]; bm++);

					if (bm == &_dedicated_size_bitmap[NUM_DEDICATED_SIZES / SIZE_T_BIT])
						goto try_large;

					// find the bit that indicates the first non-empty freelist
					mask = 1;
					offset = 0;
					while (!(*bm & mask)) {
						mask <<= 1;
						offset++;
					}
				}

				index = (bm - &_dedicated_size_bitmap[0]) * SIZE_T_BIT;
				index += offset;
				assert(index < NUM_DEDICATED_SIZES);
				assert(_dedicated_size_counter[index] > 0);
				assert(*bm & mask);

				if (--_dedicated_size_counter[index] == 0)
					*bm ^= mask;
			}

			assert(!list_empty(&_dedicated_size_list[index]));
			node = _dedicated_size_list[index].next;
			list_del(node);

			ptr = node;
		}

	  try_large:
		// then try to find a first fit in the freelist for large sizes
		if (ptr == NULL) {
			if (!list_empty(&_large_size_list)) {
				node = _large_size_list.next;
				while (node != &_large_size_list) {
					if (ObjectHeader::getHeader(node)->_size >= size) {
						list_del(node);

						ptr = node;
						break;
					}
					node = node->next;
				}
			}
		}

		sanityCheck();
		return ptr;
	}

	inline void free(void * ptr) {
		sanityCheck();

		ObjectHeader * header = ObjectHeader::getHeader(ptr);
		assert(header->isFree());
		assert(header->getPrevHeader()->getNextHeader() == header);
		assert(header->getNextHeader()->getPrevHeader() == header);

		size_t index = SIZE_TO_INDEX(header->_size);
		list_head * node = reinterpret_cast<list_head *>(ptr);

		if (index < NUM_DEDICATED_SIZES) {
			// we need to update our counter and bitmap
			if (_dedicated_size_counter[index]++ == 0) {
				assert(list_empty(&_dedicated_size_list[index]));

				assert((_dedicated_size_bitmap[index / SIZE_T_BIT] & (1 << (index % SIZE_T_BIT))) == 0);
				_dedicated_size_bitmap[index / SIZE_T_BIT] |= 1 << (index % SIZE_T_BIT);
			}

			list_add(node, &_dedicated_size_list[index]);
		}
		else {
			list_add(node, &_large_size_list);
		}

		sanityCheck();
	}

	inline void remove(void * ptr) {
//		sanityCheck();

		ObjectHeader * header = ObjectHeader::getHeader(ptr);

		// we need to update our counter and bitmap
		size_t index = SIZE_TO_INDEX(header->_size);
		if (index < NUM_DEDICATED_SIZES && --_dedicated_size_counter[index] == 0) {
			assert(!list_empty(&_dedicated_size_list[index]));
			assert(_dedicated_size_list[index].prev == _dedicated_size_list[index].next);

			assert(_dedicated_size_bitmap[index / SIZE_T_BIT] & (1 << (index % SIZE_T_BIT)));
			_dedicated_size_bitmap[index / SIZE_T_BIT] ^= 1 << (index % SIZE_T_BIT);
		}

		list_head * node = reinterpret_cast<list_head *>(ptr);
		list_del(node);

		sanityCheck();
	}

	void sanityCheck() {
#ifdef DEBUG
#if SANITY_CHECK
		for (size_t index = 0; index < NUM_DEDICATED_SIZES; index++) {
			if (!list_empty(&_dedicated_size_list[index])) {
				list_head * node = _dedicated_size_list[index].next;
				size_t size = INDEX_TO_SIZE(index);
				size_t count = 0;

				while (node != &_dedicated_size_list[index]) {
					ObjectHeader * header = ObjectHeader::getHeader(node);
					ObjectHeader * prev_header = header->getPrevHeader();
					ObjectHeader * next_header = header->getNextHeader();

					assert(header->_size == next_header->_prev_size);
					assert(header->_prev_size == prev_header->_size);

					assert(header->isFree());
					assert(!prev_header->isFree() && !next_header->isFree());

					assert(header->_size == size);

					node = node->next;
					count++;
				}

				assert(_dedicated_size_counter[index] == count);
				assert(_dedicated_size_bitmap[index / SIZE_T_BIT] & (1 << (index % SIZE_T_BIT)));
			}
			else {
				assert(_dedicated_size_counter[index] == 0);
				assert((_dedicated_size_bitmap[index / SIZE_T_BIT] & (1 << (index % SIZE_T_BIT))) == 0);
			}
		}

		if (list_empty(&_large_size_list)) {
			list_head * node = _large_size_list.next;

			while (node != &_large_size_list) {
				ObjectHeader * header = ObjectHeader::getHeader(node);
				ObjectHeader * prev_header = header->getPrevHeader();
				ObjectHeader * next_header = header->getNextHeader();

				assert(header->_size == next_header->_prev_size);
				assert(header->_prev_size == prev_header->_size);

				assert(header->isFree());
				assert(!prev_header->isFree() && !next_header->isFree());

				assert(header->_size > MaxDedicatedSize);

				node = node->next;
			}
		}
#endif
#endif
	}

private:

	enum {
		NUM_DEDICATED_SIZES = SIZE_TO_INDEX(MaxDedicatedSize) + 1,
	};

	// freelists for objects of different sizes
	list_head _dedicated_size_list[NUM_DEDICATED_SIZES];
	list_head _large_size_list;

	// counter and bitmap for dedicated sizes
	size_t _dedicated_size_counter[NUM_DEDICATED_SIZES];
	size_t _dedicated_size_bitmap[NUM_DEDICATED_SIZES / SIZE_T_BIT];

};	// end of class SegFitHeap

};	// end of namespace VAM

#endif
