***************
Getting Started
***************

.. highlight:: c

Compiling and Installing Jansson
================================

The Jansson source is available at
http://www.digip.org/jansson/releases/.

Unix-like systems (including MinGW)
-----------------------------------

Unpack the source tarball and change to the source directory:

.. parsed-literal::

    bunzip2 -c jansson-|release|.tar.bz2 | tar xf -
    cd jansson-|release|

The source uses GNU Autotools (autoconf_, automake_, libtool_), so
compiling and installing is extremely simple::

    ./configure
    make
    make check
    make install

To change the destination directory (``/usr/local`` by default), use
the ``--prefix=DIR`` argument to ``./configure``. See ``./configure
--help`` for the list of all possible configuration options.

The command ``make check`` runs the test suite distributed with
Jansson. This step is not strictly necessary, but it may find possible
problems that Jansson has on your platform. If any problems are found,
please report them.

If you obtained the source from a Git repository (or any other source
control system), there's no ``./configure`` script as it's not kept in
version control. To create the script, the build system needs to be
bootstrapped. There are many ways to do this, but the easiest one is
to use ``autoreconf``::

    autoreconf -fi

This command creates the ``./configure`` script, which can then be
used as described above.

.. _autoconf: http://www.gnu.org/software/autoconf/
.. _automake: http://www.gnu.org/software/automake/
.. _libtool: http://www.gnu.org/software/libtool/


.. _build-cmake:

CMake (various platforms, including Windows)
--------------------------------------------

Jansson can be built using CMake_. Create a build directory for an
out-of-tree build, change to that directory, and run ``cmake`` (or ``ccmake``,
``cmake-gui``, or similar) to configure the project.

See the examples below for more detailed information.

.. note:: In the below examples ``..`` is used as an argument for ``cmake``.
          This is simply the path to the jansson project root directory.
          In the example it is assumed you've created a sub-directory ``build``
          and are using that. You could use any path you want.

.. _build-cmake-unix:

Unix (Make files)
^^^^^^^^^^^^^^^^^
Generating make files on unix:

.. parsed-literal::

    bunzip2 -c jansson-|release|.tar.bz2 | tar xf -
    cd jansson-|release|

    mkdir build
    cd build
    cmake .. # or ccmake .. for a GUI.

Then to build::

    make
    make check
    make install

Windows (Visual Studio)
^^^^^^^^^^^^^^^^^^^^^^^
Creating Visual Studio project files from the command line:

.. parsed-literal::

    <unpack>
    cd jansson-|release|

    md build
    cd build
    cmake -G "Visual Studio 10" ..

You will now have a *Visual Studio Solution* in your build directory.
To run the unit tests build the ``RUN_TESTS`` project.

If you prefer a GUI the ``cmake`` line in the above example can
be replaced with::

    cmake-gui ..

For command line help (including a list of available generators)
for CMake_ simply run::

    cmake

To list available CMake_ settings (and what they are currently set to)
for the project, run::

    cmake -LH ..

Mac OSX (Xcode)
^^^^^^^^^^^^^^^
If you prefer using Xcode instead of make files on OSX,
do the following. (Use the same steps as
for :ref:`Unix <build-cmake-unix>`)::

    ...
    cmake -G "Xcode" ..

Additional CMake settings
^^^^^^^^^^^^^^^^^^^^^^^^^

Shared library
""""""""""""""
By default the CMake_ project will generate build files for building the
static library. To build the shared version use::

    ...
    cmake -DJANSSON_BUILD_SHARED_LIBS=1 ..

Changing install directory (same as autoconf --prefix)
""""""""""""""""""""""""""""""""""""""""""""""""""""""
Just as with the autoconf_ project you can change the destination directory
for ``make install``. The equivalent for autoconfs ``./configure --prefix``
in CMake_ is::

    ...
    cmake -DCMAKE_INSTALL_PREFIX:PATH=/some/other/path ..
    make install

.. _CMake: http://www.cmake.org


Android
-------

Jansson can be built for Android platforms. Android.mk is in the
source root directory. The configuration header file is located in the
``android`` directory in the source distribution.


Other Systems
-------------

On non Unix-like systems, you may be unable to run the ``./configure``
script. In this case, follow these steps. All the files mentioned can
be found in the ``src/`` directory.

1. Create ``jansson_config.h`` (which has some platform-specific
   parameters that are normally filled in by the ``./configure``
   script). Edit ``jansson_config.h.in``, replacing all ``@variable@``
   placeholders, and rename the file to ``jansson_config.h``.

2. Make ``jansson.h`` and ``jansson_config.h`` available to the
   compiler, so that they can be found when compiling programs that
   use Jansson.

3. Compile all the ``.c`` files (in the ``src/`` directory) into a
   library file. Make the library available to the compiler, as in
   step 2.


Building the Documentation
--------------------------

(This subsection describes how to build the HTML documentation you are
currently reading, so it can be safely skipped.)

Documentation is in the ``doc/`` subdirectory. It's written in
reStructuredText_ with Sphinx_ annotations. To generate the HTML
documentation, invoke::

   make html

and point your browser to ``doc/_build/html/index.html``. Sphinx_ 1.0
or newer is required to generate the documentation.

.. _reStructuredText: http://docutils.sourceforge.net/rst.html
.. _Sphinx: http://sphinx.pocoo.org/


Compiling Programs that Use Jansson
===================================

Jansson involves one C header file, :file:`jansson.h`, so it's enough
to put the line

::

    #include <jansson.h>

in the beginning of every source file that uses Jansson.

There's also just one library to link with, ``libjansson``. Compile and
link the program as follows::

    cc -o prog prog.c -ljansson

Starting from version 1.2, there's also support for pkg-config_:

.. code-block:: shell

    cc -o prog prog.c `pkg-config --cflags --libs jansson`

.. _pkg-config: http://pkg-config.freedesktop.org/
