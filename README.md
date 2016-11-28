libmaus2
========

libmaus2 is a collection of data structures and algorithms. It contains

 - I/O classes (single byte and UTF-8)
 - bitio classes (input, output and various forms of bit level manipulation)
 - text indexing classes (suffix and LCP array, fulltext and minute (FM), ...)
 - BAM sequence alignment files input/output (simple and collating)

and many lower level support classes.

-------------------------------------------------------------------------------

The main development branch of libmaus2 is hosted by github at

https://github.com/gt1/libmaus2

Release packages can be found at

https://github.com/gt1/libmaus2/releases

Please make sure to choose a package containing the word "release" in it's name if you
intend to compile libmaus2 for production (i.e. non development) use.

-------------------------------------------------------------------------------

Compilation
-----------

libmaus2 uses the GNU autoconf/automake tool set. It can be compiled on Linux
using:

	libtoolize
	aclocal
	autoreconf -i -f
	./configure
	make

Running autoreconf requires a complete set of tools including autoconf, automake,
autoheader, aclocal and libtool.

The release packages come with a configure script, so running libtoolize, aclocal etc should not be necessary, i.e.

	./configure
	make

should be sufficient to compile libmaus2.

A full list of configuration parameters can be obtained by calling

	./configure --help

libmaus2 can use functionality from several other code bases. This includes:

 - snappy [http://google.github.io/snappy/] : a fast and lightweight Lempel-Ziv
   type compression/decompression library. This can be used for the compression
   of temporary files (as used in name collating BAM input for instance).
 - io_lib [http://sourceforge.net/p/staden/code/HEAD/tree/io_lib/] : This is
   part of the Staden package. libmaus2 can use this library for SAM and CRAM
   file input.
 - SeqAN [http://www.seqan.de/] : libmaus2 contains a wrapper class for
   consensus computation. The actual consensus computation is done by SeqAN.
   libmaus2 only offers a somewhat simplified interface.

The compilation on Darwin (MacOS X) may require the installation of the 
following packages:

 - pkg-config [http://www.freedesktop.org/wiki/Software/pkg-config/]
 - boost [http://www.boost.org/]

pkg-config will be installed on most Linux systems. The boost libraries
are required if libmaus2 is to be compiled using non recent versions of the
GNU compiler, as they contain some classes offering functionality that
has meanwhile been added to the C++ standard (shared_ptr, unique_ptr,
unordered_map, ...).

Compilation of boost for libmaus2 on Darwin
------------------------------------------

The following lines called in an unpacked boost source tree should be 
sufficient to compile and install a version of the boost libraries usable
by libmaus2:

	INSTPREFIX=${HOME}/libs/boost
	./bootstrap.sh --prefix=${INSTPREFIX}
	./b2 --prefix=${INSTPREFIX} --build-type=minimal \
		--layout=system toolset=darwin variant=release link=static \
		address-model=32_64 threading=multi cxxflags="-arch i386 -arch x86_64"
	./b2 --prefix=${INSTPREFIX} --build-type=minimal \
		--layout=system toolset=darwin variant=release link=static \
		address-model=32_64 threading=multi cxxflags="-arch i386 -arch x86_64" install

Compilation of libmaus2 on Darwin
--------------------------------

After installing boost, libmaus2 can be compiled and installed in ${HOME}/libmaus2 using:

	export CPPFLAGS="-I${HOME}/libs/boost/include/"
	export CFLAGS="-arch i386 -arch x86_64 -O2"
	export CXXFLAGS="-arch i386 -arch x86_64 -O2"
	export LDFLAGS="-L${HOME}/libs/boost/lib/"
	export LIBS="-arch i386 -arch x86_64"
	export CC="/usr/bin/gcc"
	export CXX="/usr/bin/g++"
	export CPP="/usr/bin/cpp"
	export CXXCPP="/usr/bin/cpp"
	bash configure --prefix=${HOME}/libmaus2 --disable-shared-libmaus2 --disable-asm \
		--disable-dependency-tracking
	make install
