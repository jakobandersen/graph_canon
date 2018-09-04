#ifndef GRAPH_CANON_EXPLICIT_AUTOMORPHISM_HPP
#define GRAPH_CANON_EXPLICIT_AUTOMORPHISM_HPP

namespace graph_canon {
namespace detail {

template<typename State>
struct explicit_automorphism {
	using TreeNode = typename State::TreeNode;
public:

	explicit_automorphism(State &state, TreeNode &t) : state(state), t(t) { }
public: // PermutationConcept
	using value_type = typename State::SizeType;

	value_type get_(value_type i) const {
		const auto &piOld = state.get_canon_leaf()->pi;
		const auto &piNew = t.pi;
		return piNew.get(piOld.get_inverse(i));
	}
public: // SizeAwarePermutationConcept

	std::size_t degree_() const {
		return state.n;
	}
private:
	State &state;
	TreeNode &t;
};

} // namespace detail
} // namespace graph_canon

#endif /* GRAPH_CANON_EXPLICIT_AUTOMORPHISM_HPP */