#ifndef GRAPH_CANON_DETAIL_FIXED_VECTOR_HPP
#define GRAPH_CANON_DETAIL_FIXED_VECTOR_HPP

#include <memory>

namespace graph_canon {
namespace detail {

template<typename T>
struct fixed_vector {
	fixed_vector() = default;
	fixed_vector(const fixed_vector&) = delete;
	fixed_vector &operator=(const fixed_vector&) = delete;
	fixed_vector(fixed_vector&&) = delete;
	fixed_vector &operator=(fixed_vector&&) = delete;

	void reset(std::size_t n) {
		data.reset(new T[n]);
		last = data.get();
	}

	void clear() {
		last = data.get();
	}

	T *begin() {
		return data.get();
	}

	T *end() {
		return last;
	}

	template<typename ...Args>
	void emplace_back(Args&&... args) {
		*last = T(std::forward<Args>(args)...);
		++last;
	}
private:
	std::unique_ptr<T[] > data;
	T *last = nullptr;
};

} // namespace detail
} // namespace graph_canon

#endif /* GRAPH_CANON_DETAIL_FIXED_VECTOR_HPP */