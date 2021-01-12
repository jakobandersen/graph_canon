Vertex Labelled Graphs
----------------------

For many applications the graphs have associated data,
usually called "labels", "attributes", or "properties"
depending on the community.
In this example we consider vertex-labelled graphs where
labels have a weak order defined (e.g., integers or strings).
Basically, we simply need to give a less-than predicate for
vertices which we implement to perform the label comparison.

Source file: ``vertex_labels.cpp``

.. literalinclude:: ../../../examples/vertex_labels.cpp
	:language: c++
	:linenos:
