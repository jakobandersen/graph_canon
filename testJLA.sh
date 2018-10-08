#!/bin/bash
export AS_RLIMIT=300000000
root_PWD=$(pwd)
numThreads=2
doBuild="yes"
prefix="--prefix=$HOME/test/graphCanonTestInstall"
while true; do
	case $1 in
	-j)
		if [ "x$2" = "x" ]; then
			echo "Missing argument for '$1'"
			exit 1
		fi
		numThreads=$2
		shift
		shift
		;;
	"nobuild")
		doBuild="no"
		shift
		;;
	"--prefix="*)
		prefix=$1
		shift
		;;
	"")
		break
		;;
	*)
		echo "Unknown option '$1'"
		exit 1
	esac
done

args=""
args="$args	--with-boost=$HOME/programs"
args="$args --with-perm_group=$HOME/stuff/code/perm_group/stage"
args="$args	$prefix $@"
./bootstrap.sh											\
	&& rm -rf preBuild && mkdir preBuild				\
	&& cd preBuild && ../configure $args				\
	&& make dist										\
	&& cd .. && rm -rf realBuild && mkdir realBuild		\
	&& cd realBuild										\
	&& cp ../preBuild/graph_canon-*.tar.gz ./			\
	&& tar xzf graph_canon-*.tar.gz	&& cd graph_canon-*/\
	&& mkdir realBuild && cd realBuild					\
	&& ../configure $args
res=$?
if [ $res -ne 0 ]; then
	echo "Error during configuration"
	exit $res
fi
if [ "$doBuild" = "no" ]; then
	echo "Not building due to user request"
else
	time make -j $numThreads						\
		&& rm -rf $HOME/test/graphCanonTestInstall	\
		&& make install install-doc
	res=$?
	if [ $res -ne 0 ]; then
		echo "Error during installation"
		exit $res
	fi
fi
