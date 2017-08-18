#ifndef GRAPH_CANON_DETAIL_PARTITION_HPP
#define GRAPH_CANON_DETAIL_PARTITION_HPP

// An ordered partition is a list of vertices v_i, 0 <= i < num_vertices
// and a array of indices indicating the starting point of each cell in the partition.
// Let A be this array of indices, then 0 is the index of the first vertex in the first cell.
// A[0] is the index of the first vertex of the second cell, A[A[0]], the index the first vertex of the third cell, and so on.
// All A[i] = 0 for i not being the index of the first vertex of any cell.
// A[i] = num_vertices for i being the index of the first vertex in the last cell.

#include <graph_canon/config.hpp>

#include <algorithm>
#include <cassert>
#include <vector>

namespace graph_canon {
namespace detail {

template<typename SizeType>
struct partition {

	partition(SizeType n) // create unit partition
	: n(n), elements(new SizeType[n]), inverse(new SizeType[n]),
	next_cell_begin(new SizeType[n]), element_to_cell(new SizeType[n]), num_cells(1) {
#ifdef BOOST_GRAPH_CANON_CHECK_PARTITION
		assert(n > 0);
#endif
		for(SizeType i = 0; i < n; i++)
			put_element_on_index(i, i);
		std::fill(next_cell_begin.get(), next_cell_begin.get() + n, 0);
		std::fill(element_to_cell.get(), element_to_cell.get() + n, 0);
		next_cell_begin[0] = n;
	}

	partition(partition&&) = default;
	partition &operator=(partition&&) = default;

	partition(const partition &other)
	: n(other.n), elements(new SizeType[n]), inverse(new SizeType[n]),
	next_cell_begin(new SizeType[n]), element_to_cell(new SizeType[n]), num_cells(other.num_cells) {
		std::copy(other.elements.get(), other.elements.get() + n, elements.get());
		std::copy(other.inverse.get(), other.inverse.get() + n, inverse.get());
		std::copy(other.next_cell_begin.get(), other.next_cell_begin.get() + n, next_cell_begin.get());
		std::copy(other.element_to_cell.get(), other.element_to_cell.get() + n, element_to_cell.get());
	}

	SizeType *begin() {
		return elements.get();
	}

	const SizeType *begin() const {
		return elements.get();
	}

	SizeType *end() {
		return begin() + n;
	}

	const SizeType *end() const {
		return begin() + n;
	}

	SizeType get(SizeType i) const {
		return begin()[i];
	}

public: // the inverse is generally not valid during refinement

	const SizeType *begin_inverse() const {
		return inverse.get();
	}

	const SizeType *end_inverse() const {
		return begin_inverse() + n;
	}

	SizeType get_inverse(SizeType i) const {
		return begin_inverse()[i];
	}

	void reset_inverse(SizeType begin, SizeType end) {
		SizeType i = begin;
		auto iter = this->begin() + i;
		for(SizeType i = begin; i != end; ++i, ++iter)
			inverse[*iter] = i;
	}
public:
	// it only updates elements and the index map, the user must take care that no duplicates are introduced
	// during the use of the partition

	void put_element_on_index(SizeType value, SizeType element_idx) {
		elements[element_idx] = value;
		inverse[value] = element_idx;
	}
public:

	SizeType get_num_cells() const {
		return num_cells;
	}

	SizeType get_cell_end(SizeType cell_begin) const {
#ifdef BOOST_GRAPH_CANON_CHECK_PARTITION
		assert(cell_begin < elements.size());
		assert(next_cell_begin[cell_begin] != 0);
		assert(next_cell_begin[cell_begin] != cell_begin);
#endif
		return next_cell_begin[cell_begin];
	}

	SizeType get_cell_size(SizeType cell_begin) const {
		return get_cell_end(cell_begin) - cell_begin;
	}
public:

	void define_cell(SizeType cell_begin, SizeType cell_end) {
		// TODO: get rid of this, and use the new proxy thingy instead
		if(next_cell_begin[cell_begin] == 0) ++num_cells;
		next_cell_begin[cell_begin] = cell_end;
	}

	class cell_splitter {
		friend class partition;

		cell_splitter(partition<SizeType> &pi, SizeType cell_begin, SizeType cell_end)
		: pi(pi), cell_begin(cell_begin), cell_end(cell_end) {
			assert(cell_begin < cell_end);
		}

	public:

		void add_split(SizeType new_cell) {
			assert(new_cell > cell_begin);
			assert(new_cell < cell_end);
			assert(pi.next_cell_begin[new_cell] == 0);
			pi.next_cell_begin[cell_begin] = new_cell;
			cell_begin = new_cell;
			++pi.num_cells;
		}

		~cell_splitter() {
			pi.next_cell_begin[cell_begin] = cell_end;
		}
	private:
		partition<SizeType> &pi;
		SizeType cell_begin;
		const SizeType cell_end;
	};
	friend class cell_splitter;

	cell_splitter split_cell(SizeType cell_begin) {
		return cell_splitter(*this, cell_begin, get_cell_end(cell_begin));
	}

	SizeType get_cell_from_element(SizeType pos) const {
		return element_to_cell[pos];
	}

	void set_element_to_cell(SizeType cell) {
		const auto cell_end = get_cell_end(cell);
		assert(cell_end != 0);
		for(SizeType i = cell; i != cell_end; ++i)
			element_to_cell[i] = cell;
	}
public:

	void sanityCheck() const {
		std::vector<bool> checkA(n, false);
		std::vector<bool> checkB(n, false);
		for(std::size_t i = 0; i < checkA.size(); i++) {
			auto a = elements[i];
			auto b = inverse[i];
			assert(!checkA[a]);
			assert(!checkB[b]);
			checkA[a] = true;
			checkB[b] = true;
		}
		for(std::size_t i = 0; i < checkA.size(); i++) {
			assert(checkA[i]);
			assert(checkB[i]);
		}
	}
private:
	SizeType n;
	std::unique_ptr<SizeType[] > elements;
	std::unique_ptr<SizeType[] > inverse;
	std::unique_ptr<SizeType[] > next_cell_begin;
	std::unique_ptr<SizeType[] > element_to_cell;
	SizeType num_cells;
};

} // namespace detail
} // namespace graph_canon

#endif // GRAPH_CANON_DETAIL_PARTITION_HPP