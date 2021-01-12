Deciding Graph Ismorphism and Total Ordering of Graphs
------------------------------------------------------

Once we have the canonical form of multiple graphs (using the same
algorithm configuration) we can simply perform a lexicographical
comparison to decide if the graphs are isomorphic.
Similarly we can define a total order among graphs based on the
lexicographic comparison.

Source file: ``check_isomohpsim.cpp``

.. literalinclude:: ../../../examples/check_isomorphism.cpp
	:language: c++
	:linenos:
