#ifndef GRAPH_CANON_SORTING_UTILS_HPP
#define GRAPH_CANON_SORTING_UTILS_HPP

#include <boost/function_output_iterator.hpp>

#include <array>
#include <numeric>

namespace graph_canon {

template<typename Iter, typename Pred, typename Swap>
Iter partition_range(Iter first, Iter last, Pred pred, Swap swapper) {
	assert(first != last);
	assert(first + 1 != last);
	while(true) {
		while(true) {
			if(first == last)
				return first;
			else if(pred(*first))
				++first;
			else
				break;
		}
		--last;
		while(true) {
			if(first == last)
				return first;
			else if(!pred(*last))
				--last;
			else
				break;
		}
		swapper(first, last);
		++first;
	}
}

template<typename SizeType, SizeType Max>
struct counting_sorter {
	static_assert(Max >= 2, "No point in sorting with 0 or 1 different values.");

	// Iterator first, last: the range to sort.
	// ToValue toValue: for a dereferenceable Iterator iter, toValue(*iter) in the range [0, bFirsts.size()[
	// Callback callback: called with bucket begin iterators in ascending order,
	//                    from the second bucket (the first is always == 'first'),
	//                    until (including) the first bucket iterator == 'last'.

	template<typename Iterator, typename ToValue, typename Callback, typename PutValue>
	void operator()(const Iterator first, const Iterator last, ToValue toValue, Callback callback, PutValue putter) {
		end.fill(0);
		for(auto iter = first; iter != last; ++iter) {
			const SizeType val = toValue(*iter);
			assert(val < end.size());
			++end[val];
		}
		std::partial_sum(end.begin(), end.end(), end.begin());
		start[0] = 0;
		std::copy(end.begin(), end.end() - 1, start.begin() + 1);
		callback(end);
		for(SizeType bucket = 0; bucket < Max; ++bucket) {
			auto bFirst = first + start[bucket];
			const auto bLast = first + end[bucket];
			for(; bFirst != bLast; ++bFirst) {
				auto elem = *bFirst;
				auto elemBucket = toValue(elem);
				if(elemBucket == bucket) continue;
				do { // shift a cycle in place
					auto nextLoc = first + start[elemBucket];
					auto nextElem = *nextLoc;
					putter(nextLoc, elem);
					++start[elemBucket];
					elem = nextElem;
					elemBucket = toValue(elem);
				} while(elemBucket != bucket);
				putter(bFirst, elem);
			}
		}
	}
private:
	std::array<SizeType, Max> start;
	std::array<SizeType, Max> end;
};

} // namespace graph_canon

#endif // GRAPH_CANON_SORTING_UTILS_HPP
