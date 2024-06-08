// -*- C++ -*-

#ifndef _FREQUENCYHEAP_H_
#define _FREQUENCYHEAP_H_

namespace VAM {

// FrequencyHeap: a heap that segregates objects by their allocation frequency
template<size_t MaxFreqSize, bool (*highFreqReached)(size_t size, size_t count), class LowFreqHeap, class HighFreqHeap>
class FrequencyHeap : public HighFreqHeap {

  public:
	FrequencyHeap() {
		dbprintf("FrequencyHeap: sizeof(_frequent_sizes)=%u sizeof(_size_counts)=%u\n", sizeof(_frequent_sizes), sizeof(_size_counts));
		memset(_frequent_sizes, 0, sizeof(_frequent_sizes));
		memset(_size_counts, 0, sizeof(_size_counts));
	}

	inline void * malloc(size_t size) {
		void * ptr = NULL;
#if 1
		size_t index = SIZE_TO_INDEX(size);

		if (size <= MaxFreqSize) {
			if (_frequent_sizes[index]) {
				ptr = HighFreqHeap::malloc(size);
				assert(HighFreqHeap::ptrToType(ptr) != LOW_FREQ_TYPE);
			}
			else if (highFreqReached(size, ++_size_counts[index])) {
				_frequent_sizes[index] = true;
				ptr = HighFreqHeap::malloc(size);
				assert(HighFreqHeap::ptrToType(ptr) != LOW_FREQ_TYPE);
			}
		}

		if (ptr == NULL) {
			ptr = _low_freq_heap.malloc(size);
			assert(HighFreqHeap::ptrToType(ptr) == LOW_FREQ_TYPE);
		}
#else
		if (size <= MaxFreqSize) {
			ptr = HighFreqHeap::malloc(size);
			assert(HighFreqHeap::ptrToType(ptr) != LOW_FREQ_TYPE);
		}
		else {
			ptr = _low_freq_heap.malloc(size);
			assert(HighFreqHeap::ptrToType(ptr) == LOW_FREQ_TYPE);
		}
#endif
		return ptr;
	}

	inline void free(void * ptr) {
		unsigned char type = HighFreqHeap::ptrToType(ptr);

		if (type == LOW_FREQ_TYPE) {
			_low_freq_heap.free(ptr);
		}
		else {
			HighFreqHeap::free(ptr);
		}
	}

	inline size_t getSize(void * ptr) {
		unsigned char type = HighFreqHeap::ptrToType(ptr);

		if (type == LOW_FREQ_TYPE) {
			return _low_freq_heap.getSize(ptr);
		}
		else {
			return HighFreqHeap::getSize(ptr);
		}
	}

  private:

	bool _frequent_sizes[SIZE_TO_INDEX(MaxFreqSize) + 1];
	size_t _size_counts[SIZE_TO_INDEX(MaxFreqSize) + 1];

	LowFreqHeap _low_freq_heap;

};	// end of class FrequencyHeap

};	// end of namespace VAM

#endif
