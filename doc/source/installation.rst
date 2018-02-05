Installation
============

The package is built and installed as a normal autotools project
(i.e., use ``configure`` then ``make`` amd ``make install``).
However, if you have cloned the project from GitHub you
should first run ``./bootstrap.sh``, which additionally requires autotools to be installed.
The following is a list of dependencies,
where relevant arguments for ``configure`` is shown in parenthesis.

- This documentation requires `Sphinx <http://sphinx-doc.org>`__,
  preferably in the newest development version (``--enable-doc-checks``, ``--disable-doc-checks``).
- A C++ compiler with reasonable C++14 support is needed. GCC 5.1 or later should work.
- `Boost <http://boost.org>`__ dev >= 1.64 (``--with-boost=<path>``).
- `PermGroup <https://github.com/jakobandersen/perm_group>` (``--with-perm_group=<path>``).
- (optional) For the :ref:`graph-canon` program it may be useful to install the
  `argcomplete <https://pypi.python.org/pypi/argcomplete>`__ Python package.
