#ifndef GRAPH_CANON_REFINE_REFINE_HPP
#define GRAPH_CANON_REFINE_REFINE_HPP

namespace graph_canon {

// rst: .. enum-class:: RefinementResult
// rst:
// rst:		Values from this enum should be returned from the refinement function in visitors.
// rst:

enum class RefinementResult {
	// rst:		.. enumerator:: Never
	// rst:
	// rst:			If the visitor did not modify the partition, and never will no matter how much it is modified.
	Never,
	// rst:		.. enumerator:: Unchanged
	// rst:
	// rst:			If the visitor did not modify the partition, but potentially could if it gets refined more.
	Unchanged,
	// rst:		.. enumerator:: Changed
	// rst:
	// rst:			If the visitor modified the partition, and could potentially refine more if it gets refined more.
	Changed,
	// rst:		.. enumerator:: Again
	// rst:
	// rst:			If the visitor wants to be invoked again, no matter what other visitors are doing.
	// rst:			It may have modified the partition.
	Again,
	// rst:		.. enumerator:: Abort
	// rst:
	// rst:			If refinement should be aborted and the tree node pruned.
	Abort
};

//std::ostream &operator<<(std::ostream &s, RefinementResult r) {
//	switch(r) {
//	case RefinementResult::Never:
//		return s << "Never";
//	case RefinementResult::Unchanged:
//		return s << "Unchanged";
//	case RefinementResult::Changed:
//		return s << "Changed";
//	case RefinementResult::Again:
//		return s << "Again";
//	case RefinementResult::Stop:
//		return s << "Stop";
//	}
//	return s;
//}

} // namespace graph_canon

#endif /* GRAPH_CANON_REFINE_REFINE_HPP */