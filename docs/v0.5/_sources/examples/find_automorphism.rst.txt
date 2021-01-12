Finding the Automorphism Group
------------------------------

Usually an algorithm configuration includes a visitor for pruning the
search tree based on discovered autormphisms.
In fact, the algorithm will incidentially compute a generating set
for the complete automorphism group of the graph.
We can get this generating set out from the result, along with the
canonical order.

Source file: ``find_automorphisms.cpp``

.. literalinclude:: ../../../examples/find_automorphisms.cpp
	:language: c++
	:linenos:
