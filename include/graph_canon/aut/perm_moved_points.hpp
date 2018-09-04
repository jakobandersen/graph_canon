#ifndef GRAPH_CANON_AUT_PERM_MOVED_POINTS_HPP
#define GRAPH_CANON_AUT_PERM_MOVED_POINTS_HPP

namespace graph_canon {

// rst: .. class:: template<typename Perm> wrapped_perm
// rst:
// rst:		A permutation type that explicitly keeps track of which points are moved.
// rst:
// rst:		.. todo:: To be fully documented.
// rst:

template<typename Perm>
struct wrapped_perm {
	using value_type = typename perm_group::permutation_traits<Perm>::value_type;
public:

	wrapped_perm(value_type n) : perm(n) { }

	explicit wrapped_perm(Perm p) : perm(std::move(p)) {
		value_type num_moved = 0;
		for(value_type i = 0; i != perm_group::degree(perm); ++i)
			if(perm_group::get(perm, i) != i)
				++num_moved;
		if(num_moved <= perm_group::degree(perm) / 2) {
			moved_points.reserve(num_moved);
			for(value_type i = 0; i != perm_group::degree(perm); ++i)
				if(perm_group::get(perm, i) != i)
					moved_points.push_back(i);
		}
	}
public:

	value_type get_(value_type i) const {
		return perm_group::get(perm, i);
	}

	void put_(value_type i, value_type image) {
		perm_group::put(perm, i, image);
	}

	value_type degree_() const {
		return perm_group::degree(perm);
	}
public:

	// rst:		.. function:: const Perm &get_perm() const
	// rst:
	// rst:			:returns: the wrapped permutation.

	const Perm &get_perm() const {
		return perm;
	}

	// rst:		.. function:: const std::vector<value_type> &get_moved_points() const
	// rst:
	// rst:			:returns: a list of the moved points, or an empty list.

	const std::vector<value_type> &get_moved_points() const {
		return moved_points;
	}
private:
	Perm perm;
	std::vector<value_type> moved_points;
};

} // namespace graph_canon
//namespace perm_group {
//
//template<typename Perm>
//struct permutation_traits<graph_canon::wrapped_perm<Perm> > : detail::permutation_traits<graph_canon::wrapped_perm<Perm> > {
//};
//
//} // namespace perm_group

#endif /* GRAPH_CANON_AUT_PERM_MOVED_POINTS_HPP */