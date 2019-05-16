.. cpp:namespace:: graph_canon
.. default-role:: cpp:expr

Changes
#######

Develop
==========================

- #1: Add missing license (GPL).
- Change to CMake as build system.
  See :ref:`installation` for how to build the package,
  or use it as a submodule in a nother CMake project.
- Require version v0.4 PermGroup, though it is available as a Git submodule.
- Add the program :ref:`graph_canon_run` for easier invocation
  of canonicalization programs on batches of graphs.
- Add script for downloading various graph collections:
  :ref:`download_graph_collections`.
- Add the scripts :ref:`graph_canon_dreadnaut` and :ref:`graph_canon_bliss`
  for providing a common interface between the programs in this package,
  Bliss, nauty, and Traces.


Release v0.3 (2018-09-04)
==========================

- Require version v0.3 PermGroup.
- Add Schreier-Sims-based automorphism pruning.
- Documentation polishing.
- The interface of `aut_pruner_base` has been changed.
- `Visitor::canon_new_best` has been changed.
- Running the algorithm on an empty graph now works.


Release v0.2 (2018-02-05)
==========================

Second public version.
