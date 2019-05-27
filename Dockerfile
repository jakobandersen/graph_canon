# This Dockerfile is creating a full installation of the package.
# It is assumed that it is present in the root of the repository.

FROM ubuntu:18.04
ARG j=7

# apt-utils is apparently needed for doing the dpkg path exclude/include
RUN apt-get update -qq										\
 && DEBIAN_FRONTEND=noninteractive							\
    apt-get install --no-install-recommends -y apt-utils	\
 && echo 													\
   'path-exclude /usr/share/doc/*'							\
   '\npath-include /usr/share/doc/*/copyright'				\
   '\npath-exclude /usr/share/man/*'						\
   '\npath-exclude /usr/share/groff/*'						\
   '\npath-exclude /usr/share/info/*'						\
   '\npath-exclude /usr/share/lintian/*'					\
   '\npath-exclude /usr/share/linda/*'						\
   > /etc/dpkg/dpkg.cfg.d/01_nodoc							\
 && DEBIAN_FRONTEND=noninteractive							\
    apt-get install -y										\
    git cmake build-essential								\
	libboost-graph-dev libboost-chrono-dev					\
	libboost-random-dev libboost-program-options-dev		\
	libboost-system-dev										\
 && apt-get clean											\
 && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/graph_canon
COPY .git .git
COPY external external
RUN 											\
	git reset --hard						&&	\
	git submodule update --init --recursive	&&	\
	./bootstrap.sh							&&	\
	rm -rf build && mkdir build && cd build	&&	\
	cmake ../									\
		-DCMAKE_BUILD_TYPE=Release				\
		-DBUILD_BIN_BENCHMARK=on				\
		-DBUILD_BIN_ALL_ALG=on					\
		-DBUILD_DOC=off							\
		-DBUILD_EXAMPLES=on					&&	\
  make -j $j && make install && cd .. && rm -rf *
