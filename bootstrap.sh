#!/usr/bin/env bash

function indent {
	cat | sort | sed 's/^/	"/' | sed 's/$/"/'
}

function indentAndDir {
	cat | sort | sed 's/^/	"${CMAKE_CURRENT_LIST_DIR}\//' | sed 's/$/"/'
}

function gen_file_lists {
	echo "# Auto-generated by bootstrap.sh"
	echo "set(graph_canon_INCLUDE_FILES"
	find include -iname "*.hpp" | indentAndDir
	echo ")"
	echo ""
	echo "set(graph_canon_TEST_FILES"
	find test -iname "*.cpp" | grep -v cmake | sed "s/^test\/\(.*\)\.cpp$/\1/" | indent
	echo ")"
	echo ""
	echo "set(graph_canon_EXAMPLE_FILES"
	find examples -iname "*.cpp" | sed "s/^examples\/\(.*\)\.cpp$/\1/" | indent
	echo ")"
	echo ""
}

echo "VERSION"
git describe --tags --always | sed -e "s/^v//" -e "s/-g.*$//" -e "s/-/.0./" > VERSION
cat VERSION
echo "CMakeFiles.txt"
gen_file_lists > CMakeFiles.txt
echo "Docs"
doc/makeDocs.sh .

echo "Recursing in external/perm_group"
echo "--------------------------------"
cd external/perm_group
./bootstrap.sh
