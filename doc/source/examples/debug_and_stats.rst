Investigating the Details of an Algorithm Configuration
-------------------------------------------------------

Two of the provided visitors are dedicated to logging events during
a canonicalization run and recording statistics
(e.g., number of tree nodes).
Those visitors can also record data for visualizing what happened
during the algorithm.
This can either be as a static picture of the search tree
(using Graphviz) or a dynamic visualization where the events of the
algorithm can be replayed
(using http://jakobandersen.github.io/graph_canon_vis/).

Source file: ``debug_and_stats.cpp``

.. literalinclude:: ../../../examples/debug_and_stats.cpp
	:language: c++
	:linenos:
