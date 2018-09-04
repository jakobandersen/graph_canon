#ifndef GRAPH_CANON_SORTING_UTILS_HPP
#define GRAPH_CANON_SORTING_UTILS_HPP

#include <boost/function_output_iterator.hpp>

#include <array>
#include <numeric>

namespace graph_canon {

// rst: .. function:: template<typename Iter, typename Pred, typename Swap> \
// rst:               Iter partition_range(Iter first, Iter last, Pred pred, Swap swapper)
// rst:
// rst:		A function template equivalent to `std::partition`,
// rst:		except that element swapping is delegated to the given `swapper`.
// rst:
// rst:		Requires (in addition to the requirements by `std::partition`),
// rst:		that for two dereferenceable iterators `a` and `b` in the given range,
// rst:		the expression `swapper(a, b)` swaps the values represented by the iterators.
// rst:		For example, `Swap` may be a predicate simply calling `std::iter_swap`.

template<typename Iter, typename Pred, typename Swap>
Iter partition_range(Iter first, Iter last, Pred pred, Swap swapper) {
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

// rst: .. class:: template<typename SizeType, SizeType Max> \
// rst:            counting_sorter
// rst:
// rst:		An object for performing counting sort of values in the range 0 to `Max`.
// rst:		Requires `Max >= 2`.
// rst:
template<typename SizeType, SizeType Max>
struct counting_sorter {
	static_assert(Max >= 2, "No point in sorting with 0 or 1 different values.");

	// rst:		.. function:: template<typename Iterator, typename ToValue, typename Callback, typename PutValue> \
	// rst:		              void operator()(const Iterator first, const Iterator last, ToValue toValue, Callback callback, PutValue putter)
	// rst:
	// rst:			Sort the range `first` to `last`.
	// rst:
	// rst:			Requires:
	// rst:
	// rst:			- `Iterator` must model a `RandomAccessIterator`.
	// rst:			- `ToValue`: for a dereferenceable iterator `iter`, the expression `toValue(*iter)`
	// rst:			  must return an integer in the range 0 to `Max`.
	// rst:			- `Callback`: for a range of integers `ends` representing the end of each bucket,
	// rst:			  as offsets from `first`, the expression `callback(ends)` must be valid.
	// rst:			- `PutValue`: for a dereferenceable iterator `iter` and an element `elem`,
	// rst:			  the expression `putter(iter, elem)` must be valid, and ensure that `*iter == elem` becomes true.

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
