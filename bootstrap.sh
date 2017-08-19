subdirs="include test"

function indentAndSlash {
	cat | sort | \
		sed "s/^/	/" |	\
		sed "s/$/ \\\\/" |	\
		sed "\$s/\\\\//"
}

function gen_root {
	echo "SUBDIRS = $subdirs"
	
	echo "dist_bin_SCRIPTS = \\"
	echo "	bin/graph-canon"
	echo ""
	echo "bin_PROGRAMS = dimacs"
	echo "dimacs_SOURCES = src/dimacs.cpp"
	echo "dimacs_LDADD = @BOOST_LDLIBS@"

	for mode in "test" "benchmark"; do
		#for eLabel in "no-el" "el"; do
		for eLabel in "no-el"; do
			for refine in "WL-1"; do
			for degreeOne in "false" "true"; do
				for autPruner in "none" "basic"; do
				for autImplicit in "false" "true"; do
					for partialLeaf in "false" "true"; do
					for trace in "false" "true"; do
					for quotient in "false" "true"; do
						name="graph-canon-${mode}_${eLabel}_${refine}"
						if [ "$autPruner" != "none" ]; then
							name="${name}_${autPruner}"
						fi
						if [ "$autImplicit" == "true" ]; then
							name="${name}_aut-implicit"
						fi
						if [ "$partialLeaf" == "true" ]; then
							name="${name}_partial-leaf"
						fi
						if [ "$trace" == "true" ]; then
							name="${name}_trace"
						fi
						if [ "$quotient" == "true" ]; then
							name="${name}_quotient"
						fi
						if [ "$degreeOne" == "true" ]; then
							name="${name}_d1"
						fi
						amName=$(echo "$name" | tr '-' '_')
						refineId=$(echo $refine | tr '-' '_')
						autPrunerId=$(echo $autPRuner | tr '-' '_')
						echo ""
						echo "bin_PROGRAMS += $name"
						echo "${amName}_SOURCES = \\"
						echo "	src/graph_canon_${mode}.cpp"
						echo "${amName}_LDADD = @BOOST_LDLIBS@"
						echo "${amName}_CPPFLAGS = @AM_CPPFLAGS@"
						if [ "$eLabel" = "el" ]; then
							echo "${amName}_CPPFLAGS += -DGRAPH_CANON_EDGE_LABELS"
						fi
						echo "${amName}_CPPFLAGS += -DGRAPH_CANON_REFINE=$refineId"
						if [ "$autPruner" != "none" ]; then
							echo "${amName}_CPPFLAGS += -DGRAPH_CANON_AUT_PRUNER=$autPruner"
						fi
						if [ "$autImplicit" == "true" ]; then
							echo "${amName}_CPPFLAGS += -DGRAPH_CANON_AUT_IMPLICIT"
						fi
						if [ "$partialLeaf" == "true" ]; then
							echo "${amName}_CPPFLAGS += -DGRAPH_CANON_PARTIAL_LEAF"
						fi
						if [ "$trace" == "true" ]; then
							echo "${amName}_CPPFLAGS += -DGRAPH_CANON_TRACE"
						fi
						if [ "$quotient" == "true" ]; then
							echo "${amName}_CPPFLAGS += -DGRAPH_CANON_QUOTIENT"
						fi
						if [ "$degreeOne" == "true" ]; then
							echo "${amName}_CPPFLAGS += -DGRAPH_CANON_DEGREE_1"
						fi
						if [ "$mode" = "test" ]; then
							echo "${amName}_CXXFLAGS = -g @AM_CXXFLAGS@"
						fi
					done # quotient
					done # trace
					done # partialLeaf
				done # autImplicit
				done # autPruner
			done # degreeOne
			done # refine
		done # eLabel
	done # mode

	echo ""
	echo "EXTRA_DIST = \\"
	(
		find src -type f
		find test -type f
	) | indentAndSlash
}

function gen_include {
	echo "nobase_include_HEADERS = \\"
	find graph_canon -type f | indentAndSlash
}

function gen_test {
	echo "check_PROGRAMS = \\"
	ls *.cpp | sed "s/.cpp$//" | indentAndSlash
	echo "TESTS = \$(check_PROGRAMS)"
	for f in $(ls *.cpp); do
		e=${f%.cpp}
		echo "${e}_SOURCES = $f"
		echo "${e}_LDADD = -lboost_program_options -lboost_random -lboost_chrono -lboost_system"
	done
}

cs=$(git rev-parse --short HEAD)

echo "VERSION"
git describe --tags --always > VERSION
echo "Makefile.am"
gen_root > Makefile.am
for d in $subdirs; do
	echo $d/Makefile.am
	cd $d 
	$(echo "gen_$d") > Makefile.am
	cd ..
done
autoreconf -fi
