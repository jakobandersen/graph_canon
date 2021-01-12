.. _installation:

Installation
============


From a Git Repository
---------------------

After a checkout of the desired version, do::

	git submodule update --init --recursive
	./bootstrap.sh

This is needed to first fetch certain dependencies and second
to generate build files, extract the API documentation,
and create the file ``VERSION`` based on the current commit.

See :ref:`source-build` on how to then build the package.


As Dependency of Another CMake Project
--------------------------------------

GraphCanon supports use via ``add_subdirectory`` in CMake.
The target ``GraphCanon::graph_canon`` is exported,
which can be used with ``target_link_libraries``.
The version is variable in the variable ``GraphCanon_VERSION``.
Note that running ``./bootstrap.sh`` is still needed if the
source is a repository clone (e.g., a Git submodule).


.. _source-build:

From a Source Archive
---------------------

The package is build and installed from source as a CMake project.
Generally that means something like::

	mkdir build
	cd build
	cmake ../ <options>
	make -j <n>
	make install

A source archive can also be created with ``make dist``.

The following is a list of commonly used options for ``cmake``,
and additional options specific for GraphCanon.

- ``-DCMAKE_INSTALL_PREFIX=<prefix>``, set a non-standard installation directory.
  Note also that the
  `GNUInstallDirs <https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html>`__
  module is used.
- ``-DCMAKE_BUILD_TYPE=<build type>``, set a non-standard build type.
  The default is `RelWithDebInfo <https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html?highlight=build_type#variable:CMAKE_BUILD_TYPE>`__.
  An additional build type ``OptDebug`` is available which adds the compilation flags ``-g -O3``.
- ``-DBUILD_BIN=on``, whether to build any command line executables or not.
  This is forced to ``off`` when used via ``add_subdirectory``.
  See also :ref:`graph-canon`.
- ``-DBUILD_BIN_BENCHMARK=off``, whether to build the benchmark version of command line executables or not.
  This only has effect if ``BUILD_BIN=on``.
- ``-DBUILD_BIN_ALL_ALG=off``, whether to build all algorithm variations for the command line executables or not.
  This only has effect if ``BUILD_BIN=on``.
- ``-DBUILD_DOC=on``, whether to bulid documentation or not.
  This is forced to ``off`` when used via ``add_subdirectory``.
- ``-DBUILD_EXAMPLES=off``, whether to allow building of the example programs or not.
  When ``on`` the examples can be build with ``make examples``,
  and the resulting programs will then be present in the ``examples/`` subfolder in your build folder.
  This is forced to ``off`` when used via ``add_subdirectory``.
- ``-DBUILD_TESTING=off``, whether to allow test building or not.
  This is forced to ``off`` when used via ``add_subdirectory``.
  When ``on`` the tests can be build with ``make tests`` and run with ``ctest``.
- ``-DBUILD_TESTING_SANITIZERS=on``, whether to compile tests with sanitizers or not.
  This has no effect with code coverage is enabled.
- ``-DUSE_NESTED_PERM_GROUP=on``, whether to use the dependency PermGroup from the Git submodule or not.


Dependencies
------------

- This documentation requires a supported version of `Sphinx <http://sphinx-doc.org>`__
  (``-DBUILD_DOC=on``).
- A C++ compiler with reasonable C++17 support is needed.
- `Boost <http://boost.org>`__ dev >= 1.67
  (use ``-DBOOST_ROOT=<path>`` for non-standard locations).
- `PermGroup <https://github.com/jakobandersen/perm_group>`__ >= 0.5.
  This is fulfilled via a Git submodule (make sure to do ``git submodule update --init --recursive``),
  but if another source is needed, set ``-DUSE_NESTED_PERM_GROUP=off``.
- (optional) For the :ref:`graph-canon` program it may be useful to install the
  `argcomplete <https://pypi.python.org/pypi/argcomplete>`__ Python package.
