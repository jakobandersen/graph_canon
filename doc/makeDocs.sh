#!/bin/bash

if [ "x$1" = "xclean" ]; then
	rm -rf build doctrees source/reference
	exit 0
fi

topSrcDir=${1:-..}

function outputRST {
	mkdir -p $topSrcDir/doc/source/$(dirname $1)
	cat | sed "s/	/    /g" > $topSrcDir/doc/source/$1.rst
}

function filterCPP {
	cat | awk '
BEGIN {
	inClass = 0
	lineNum = 0
	inNestedClass = 0
	nestedLineNum = 0
	lastClass = ""
	lastNested = ""
}
{
	if($0 ~ /^[\t]*\/\/ rst: .. py:class::/) {
		sub(/^[\t]*\/\/ rst: .. py:class:: /, "")
		print ""
		print "Class ``" $0 "``"
		print "--------------------------------------------------------------------------------------------------------------------------------"
		print ""
		print ".. py:class:: " $0
	} else if($0 ~ /^[\t]*\/\/ rst:/) {
		sub(/^[\t]*\/\/ rst:[ 	]?/, "")
		if(inClass)
			if(inNested)
				nestedRST[nestedLineNum++] = $0
			else
				normalRST[lineNum++] = $0
		else
			print
	} else if($0 ~ /^[\t]*\/\* rst:/) {
		sub(/^[\t]*\/\* rst: ?/, "")
		sub(/\*\/[\t]*\\?/, "")
		if(inClass)
			if(inNested)
				nestedRST[nestedLineNum++] = $0
			else
				normalRST[lineNum++] = $0
		else
			print
	} else if($0 ~ /^[\t]*\/\/ rst-class:/) {
		sub(/^[\t]*\/\/ rst-class: /, "")
		lastClass = $0
		sub(/ : .*/, "", lastClass)
		print ""
		print "Class ``" lastClass "``"
		print "--------------------------------------------------------------------------------------------------------------------------------"
		print ""
		print ".. class:: " $0
		print "    "
	} else if($0 ~ /^[\t]*\/\/ rst-class-start:/) {
		inClass	= 1
		print ""
		print "Synopsis"
		print "^^^^^^^^"
		print ""
		print ".. code-block:: c++"
		print "    "
	} else if($0 ~/^[\t]*\/\/ rst-class-end:/) {
		inClass = 0
		if(lineNum > 0) {
			print ""
			print "Details"
			print "^^^^^^^"
			print ""
			print ".. cpp:namespace:: graph_canon"
			print ""
			print ".. cpp:namespace-push:: " lastClass
			print ""
			for(i = 0; i < lineNum; i++) print normalRST[i]
			print ""
			print ".. cpp:namespace:: graph_canon"
			print ""
			lineNum = 0
		}
		if(nestedLineNum > 0) {
			for(i = 0; i < nestedLineNum; i++) print nestedRST[i]
			print ""
			nestedLineNum = 0
		}
	} else if($0 ~/^[\t]*\/\/ rst-nested:/) {
		sub(/^[\t]*\/\/ rst-nested: /, "")
		if(!inClass) {
			print "Nested class outside class!" | "cat 1>&2"
			exit 1
		}
		inNested = 1
		nestedRST[nestedLineNum++] = ""
		nestedRST[nestedLineNum++] = "Class ``" $0 "``"
		nestedRST[nestedLineNum++] = "---------------------------------------------------------------"
		nestedRST[nestedLineNum++] = ""
		nestedRST[nestedLineNum++] = ".. class:: " $0
		nestedRST[nestedLineNum++] = ""
		lastNested = $0
	} else if($0 ~/^[\t]*\/\/ rst-nested-start:/) {
		nestedRST[nestedLineNum++] = ""
	} else if($0 ~/^[\t]*\/\/ rst-nested-end:/) {
		nestedRST[nestedLineNum++] = ""
		inNested = 0
	} else if(inClass) {
		if(!($0 ~/^[\t]*$/))
			print "\t"$0
	}
}
'
}

function getHeaders {
	cd $topSrcDir/include/graph_canon
	find . -iname "*.hpp" \
		| grep -v -e "detail/" -e "config.hpp" \
		| sed -e "s/^.\///" -e "s/.hpp$//" \
		| sort
}

function makeIndex {
	function indexFiles {
		cat << "EOF"
	installation
	executables/index
	changes
	reference/index
EOF
	}
	function data {
		cat << "EOF"
################################################
GraphCanon
################################################

.. toctree::
	:maxdepth: 1
	:numbered:
	
EOF
		indexFiles
cat << "EOF"


.. _overview:

Introduction
============

This is the documentation for the GraphCanon library and its associated executables.
The library provides an algorithm framework for graph canonicalization,
graph isomorphism, and computation of automorphism groups of graphs,
using a customizable individualization-refinement algorithm.

The library was initially written as part of the paper
**A Generic Framework for Engineering Graph Canonization Algorithms**
[`DOI <http://doi.org/10.1137/1.9781611975055.13>`__ | `TR <http://arxiv.org/abs/1711.08289>`__].

The library can for example be used for

- finding a canonical order of the vertices of a given graph,
- finding the automorphism group of a given graph,
- using a canonical vertex order to create a view of a graph with canonical iteration order,
- lexicographical comparison of such permuted graph views, e.g., for isomorphism testing.

Graphs with vertices labelled with elements from a totally ordered set
are handled by a user-supplied less-predicate (as you would provide to `std::sort`).
The library also supports graphs where edges are similarly labelled,
though for obtaining efficient implementations this may require substantial work from the user.

.. attention:: In the future there will be a range of examples for using the library and extending it.

Handling of permutation groups is done using the `PermGroup <https://github.com/jakobandersen/perm_group>`__ library which is currently under heavy development.

The package additionally contains a program ``graph-canon``,
which makes many variations of the core algorithm available from the command line.
The program also facilitates extraction of additional data from the algorithm run,
e.g., visualization data for the `GraphCanon Visualizer <https://github.com/jakobandersen/graph_canon_vis>`__.



Indices and Tables
==================

* :ref:`genindex`
EOF
cat << "EOF"

Table of Contents
=================

.. toctree::
	:maxdepth: 4
	
EOF
		indexFiles
	}
	data | outputRST index
}

function makeExecutables {
	function index {
		cat << "EOF"
**************************
Executables
**************************

.. toctree::
	:maxdepth: 1

	dimacs
	graph_canon
EOF
	}
	index | outputRST executables/index
	cat $topSrcDir/bin/dimacs.cpp | filterCPP | outputRST executables/dimacs
	(
		cat $topSrcDir/bin/graph_canon_util.hpp | filterCPP
		cat $topSrcDir/bin/graph_canon_benchmark.cpp | filterCPP
		cat $topSrcDir/bin/graph_canon_test.cpp | filterCPP
	) | outputRST executables/graph_canon
}

function makeReference {
	function data {
		local f=$1
		echo ".. _cpp-$f:"
		echo ""
		echo "**********************************************************"
		echo "$f.hpp" | sed "s/^.*\///"
		echo "**********************************************************"
		echo ""
		echo "Full path: \`\`graph_canon/$f.hpp\`\`"
		echo ""
		cat << "EOF"
.. default-domain:: cpp
.. default-role:: cpp:expr

.. cpp:namespace:: graph_canon

EOF
		cat $topSrcDir/include/graph_canon/$f.hpp | filterCPP
	}
	for f in $(getHeaders); do
		data $f | outputRST reference/$f
	done
	function getFolders {
		getHeaders | grep "/" | sed "s/\/.*//" | uniq
	}
	function folderToc {
		echo $1
		cat << "EOF"
==============================================================================

.. toctree::
	:maxdepth: 2

EOF
		getHeaders | grep "^$1" | sed -e "s/^$1\///" -e "s/^/	/"
	}
	for f in $(getFolders); do
		folderToc $f | outputRST reference/$f/index	
	done

	function dataToc {
		cat << "EOF"
Library Reference
=================

.. toctree::
	:maxdepth: 2

EOF
		(
			getHeaders | grep -v "/"
			getFolders | sed 's/$/\/index/'
		) | sort | sed 's/^/	/'
	}
	dataToc | outputRST reference/index
}

makeIndex
makeExecutables
makeReference