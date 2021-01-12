.. cpp:namespace:: graph_canon
.. default-role:: cpp:expr

Changes
#######

v0.5 (2020-01-12)
=================

Incompatible Chagnes
--------------------

- Bump Boost requirement to 1.67.
- Require C++17.
- Bump PermGroup requirement to 0.5.

New Features
------------

- Added convenience type aliasses `ordered_graph::Graph` and
  `ordered_graph::IndexMap`.

Bugs Fixed
----------

- Fix extranous dereference in `ordered_graph::get_graph`.
  Thanks to Gregor Kälin.
- Use `idx` when setting up initial partition.
  Thanks to Gregor Kälin.
- Use ``get(idx, v)`` instead of ``idx[v]`` consistently everywhere.
- Add missing concept checks in `canonicalize` for the graph type.
- Add missing inline on `make_default_visitor`.
  Thanks to Gregor Kälin.
- Doc, fix indendation issues.

Other
-----

- Update documentation for `EdgeHandler` and `canonicalize`.


v0.4 (2019-05-27)
=================

Incompatible Changes
--------------------

- Change to CMake as build system.
  See :ref:`installation` for how to build the package,
  or use it as a submodule in a nother CMake project.
- Require version v0.4 PermGroup, though it is available as a Git submodule.
- Change :cpp:func:`make_default_visitor` to include many more visitors,
  to provide a quick way to get a reasonable algorithm configuration.
- Change the style of search tree depictions from :cpp:class:`stats_visitor`
  to use rounded rectangles as node shapes.

New Features
------------

- Add the program :ref:`graph_canon_run` for easier invocation
  of canonicalization programs on batches of graphs.
- Add script for downloading various graph collections:
  :ref:`download_graph_collections`.
- Add the scripts :ref:`graph_canon_dreadnaut` and :ref:`graph_canon_bliss`
  for providing a common interface between the programs in this package,
  Bliss, nauty, and Traces.
- Add ``num_edges`` support for :cpp:class:`ordered_graph`.
- Add :cpp:func:`as_range` helper function for converting Boost.Graph
  iterator pairs to ranges.

Other
-----

- #1: Add missing license (GPL).
- Add and update examples.
- Lots of documentation improvements.


v0.3 (2018-09-04)
=================

- Require version v0.3 PermGroup.
- Add Schreier-Sims-based automorphism pruning.
- Documentation polishing.
- The interface of `aut_pruner_base` has been changed.
- `Visitor::canon_new_best` has been changed.
- Running the algorithm on an empty graph now works.


v0.2 (2018-02-05)
=================

Second public version.
