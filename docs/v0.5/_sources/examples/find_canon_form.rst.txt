Computing the Canonical Form
----------------------------

Graphs can have many representations, with some being textual for
external storage (e.g., the DIMACS file format), and some being
in-memory data structures.
In the following example we

1. load a graph from a DIMACAS file into an in-memory adjacency list,
2. compute the canonical order of the vertices,
3. use that order to create an ordered view of the adjacency list,
   so we can iterate in canonical order,
4. and finally we use that in-memory canonical form to write a canonical DIMACS representation.

Note that the canonical form is specific for the algorithm configuration
we use.

Source file: ``find_canon_form.cpp``

.. literalinclude:: ../../../examples/find_canon_form.cpp
	:language: c++
	:linenos:
