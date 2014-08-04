AC_INIT(libmaus,0.0.153,[gt1@sanger.ac.uk],[libmaus],[http://www.sanger.ac.uk])
LIBRARY_VERSION=0:153:0
AC_MSG_NOTICE([Configuring for source in directory ${srcdir}])
AC_CANONICAL_SYSTEM
AC_PROG_LIBTOOL

AM_INIT_AUTOMAKE([std-options subdir-objects])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

CFLAGS_SAVE="${CFLAGS}"
CFLAGS=""
AC_PROG_CC
AM_PROG_CC_C_O
CFLAGS="${CFLAGS_SAVE}"

abs_builddir="${PWD}"

CXXFLAGS_SAVE="${CXXFLAGS}"
CXXFLAGS=""
AC_PROG_CXX
CXXFLAGS="${CXXFLAGS_SAVE}"

AC_CHECK_SIZEOF([unsigned long])
LIBMAUS_SIZEOF_UNSIGNED_LONG="#define LIBMAUS_SIZEOF_UNSIGNED_LONG $ac_cv_sizeof_unsigned_long"

LIBMAUSCPPFLAGS="${CPPFLAGS}"
LIBMAUSLDFLAGS="${LDFLAGS}"

PKG_PROG_PKG_CONFIG

LIBMAUS_HAVE_IGZIP=
AC_ARG_WITH([igzip],
	[AS_HELP_STRING([--with-igzip@<:@=PATH@:>@], [path to installed igzip library @<:@default=no@:>@])],
	[with_igzip=${withval}],
	[with_igzip=no])

IGZIPCPPFLAGS=
IGZIPLDFLAGS=
IGZIPLIBS=
if test "${with_igzip}" != "no" ; then
	IGZIPPATH=${with_igzip}
	
	IGZIPCPPFLAGS=
	IGZIPLDFLAGS=
	IGZIPLIBS="-ligzip1c"
	
	if test ! -z "${IGZIPPATH}" ; then
		IGZIPCPPFLAGS="-I${IGZIPPATH}/include"
		IGZIPLDFLAGS="-L${IGZIPPATH}/lib -Wl,-rpath,${IGZIPPATH}/lib"
	fi
	
	CPPFLAGS_SAVE="${CPPFLAGS}"
	LDFLAGS_SAVE="${LDFLAGS}"
	LIBS_SAVE="${LIBS}"
	
	LIBS="${LIBS} ${IGZIPLIBS}"
	LDFLAGS="${LDFLAGS} ${IGZIPLDFLAGS}"
	CPPFLAGS="${CPPFLAGS} ${IGZIPCPPFLAGS}"

	AC_LANG_PUSH([C++])
	AC_MSG_CHECKING([whether we can compile a igzip program])
	AC_TRY_LINK([extern "C" {
#include <igzip_lib.h>
	}
	],[
	LZ_Stream2 stream;   
	init_stream(&stream);
	fast_lz(&stream);
	],[igzip=yes],[igzip=no])
		AC_MSG_RESULT([${igzip}])
	AC_LANG_POP
	
	CPPFLAGS="${CPPFLAGS_SAVE}"
	LDFLAGS="${LDFLAGS_SAVE}"
	LIBS="${LIBS_SAVE}"
	
	if test ${igzip} = "yes" ; then
		LIBMAUS_HAVE_IGZIP="#define LIBMAUS_HAVE_IGZIP"
		LIBMAUSPKGLIBS="${LIBMAUSPKGLIBS} ${IGZIPLDFLAGS} ${IGZIPLIBS}"
		LIBMAUSPKGCPPFLAGS="${LIBMAUSPKGCPPFLAGS} ${IGZIPCPPFLAGS}"
	else
		IGZIPCPPFLAGS=
		IGZIPLDFLAGS=
		IGZIPLIBS=
		AC_MSG_ERROR([Failed to compile igzip program.]);
	fi
fi

AC_ARG_WITH([kmlocal],
	[AS_HELP_STRING([--with-kmlocal@<:@=PATH@:>@], [path to installed kmlocal library @<:@default=no@:>@])],
	[with_kmlocal=${withval}],
	[with_kmlocal=no])

KMLOCAL=
if test "${with_kmlocal}" != "no" ; then

	if test ! -z "${with_kmlocal}" ; then
		PKGCONFIGPATHSAVE="${PKG_CONFIG_PATH}"
		if test -z "${PKG_CONFIG_PATH}" ; then
			export PKG_CONFIG_PATH="${with_kmlocal}/lib/pkgconfig"
		else
			export PKG_CONFIG_PATH="${with_kmlocal}/lib/pkgconfig:${PKG_CONFIG_PATH}"
		fi
	fi

	PKG_CHECK_MODULES([kmlocal],[kmlocal >= 1.7.2],[kmlocalpkg=yes],[kmlocalpkg=no])

	if test ! -z "${with_kmlocal}" ; then
		if test ! -z "${PKGCONFIGPATHSAVE}" ; then
			export PKG_CONFIG_PATH="${PKGCONFIGPATHSAVE}"
		fi
	fi

	KMLOCAL=
	if [[ "${kmlocalpkg}" = "yes" ]] ; then
		# PKG_CHECK_MODULES([kmlocal],[kmlocal >= 1.7.2])

		AC_LANG_PUSH([C++])
		LIBSSAVE="${LIBS}"
		LIBS="${LIBS} ${kmlocal_LIBS}"
		CPPFLAGSSAVE="${CPPFLAGS}"
		CPPFLAGS="${CPPFLAGS} ${kmlocal_CFLAGS}"
		AC_MSG_CHECKING([whether we can compile a kmlocal program])
		AC_TRY_LINK([#include "KMlocal.h"
	#include <cstring>
	#include <iostream>
	],[KMterm  term(100, 0, 0, 0,              // run for 100 stages
		     0.10, 0.10, 3,             // other typical parameter values 
		     0.50, 10, 0.95);

		int         k       = 4;                    // number of centers
		int         dim     = 2;                    // dimension
		int         nPts    = 20;                   // number of data points

		KMdata dataPts(dim, nPts);                  // allocate data storage
		kmUniformPts(dataPts.getPts(), nPts, dim);  // generate random points
		dataPts.buildKcTree();                      // build filtering structure
		KMfilterCenters ctrs(k, dataPts);           // allocate centers

							// run the algorithm
		KMlocalLloyds       kmAlg(ctrs, term);      // repeated Lloyd's
		// KMlocalSwap      kmAlg(ctrs, term);      // Swap heuristic
		// KMlocalEZ_Hybrid kmAlg(ctrs, term);      // EZ-Hybrid heuristic
		// KMlocalHybrid    kmAlg(ctrs, term);      // Hybrid heuristic
		ctrs = kmAlg.execute();                     // execute
							// print number of stages
		std::cout << "Number of stages: " << kmAlg.getTotalStages() << "\n";
							// print average distortion
		std::cout << "Average distortion: " << ctrs.getDist()/nPts << "\n";
		ctrs.print();                               // print final centers

		kmExit(0);
		],[kmlocal=yes],[kmlocal=no])
		AC_MSG_RESULT([${kmlocal}])
		LIBS="${LIBSSAVE}"
		CPPFLAGS="${CPPFLAGSSAVE}"
		AC_LANG_POP

		if [[ "$kmlocal" = "no" ]] ; then
			AC_MSG_ERROR([Failed to compile kmlocal program.]);
		else
			KMLOCAL="#define LIBMAUS_HAVE_KMLOCAL"
			KMLOCALLIBS="${kmlocal_LIBS}"
			KMLOCALCPPFLAGS="${kmlocal_CFLAGS}"
		fi
	fi
fi

PKG_CHECK_MODULES([zlib],[zlib >= 0],[zlibpkg=yes],[zlibpkg=no])

if [[ "${zlibpkg}" = "yes" ]] ; then
	PKG_CHECK_MODULES([zlib],[zlib >= 0])

        AC_LANG_PUSH([C++])
	LIBSSAVE="${LIBS}"
	LIBS="${LIBS} ${zlib_LIBS}"
	CPPFLAGSSAVE="${CPPFLAGS}"
	CPPFLAGS="${CPPFLAGS} ${zlib_CFLAGS}"
        AC_MSG_CHECKING([whether we can compile a zlib program])
        AC_TRY_LINK([#include <zlib.h>
#include <cstring>
],[
	z_stream strm;
	memset ( &strm , 0, sizeof(z_stream) );
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	deflateInit(&strm,Z_DEFAULT_COMPRESSION);
	],[zlib=yes],[zlib=no])
        AC_MSG_RESULT([${zlib}])
	LIBS="${LIBSSAVE}"
	CPPFLAGS="${CPPFLAGSSAVE}"
        AC_LANG_POP

	if [[ "$zlib" = "no" ]] ; then
		AC_MSG_ERROR([Required library zlib not found.]);
	else
		ZLIBREQ=zlib
		ZLIBCPPFLAGS="${zlib_CFLAGS}"
		ZLIBLIBS="${zlib_LIBS}"
	fi
else
	AC_MSG_WARN([zlib pkgconfig file is not installed. Trying if -lz for LIBS is enough.])

        AC_LANG_PUSH([C++])
	LIBSSAVE="${LIBS}"
	LIBS="${LIBS} -lz"
        AC_MSG_CHECKING([whether we can compile a zlib program])
        AC_TRY_LINK([#include <zlib.h>
#include <cstring>
],[
	z_stream strm;
	memset ( &strm , 0, sizeof(z_stream) );
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	deflateInit(&strm,Z_DEFAULT_COMPRESSION);
	],[zlib=yes],[zlib=no])
        AC_MSG_RESULT([${zlib}])
	LIBS="${LIBSSAVE}"
        AC_LANG_POP

	if [[ "$zlib" = "no" ]] ; then
		AC_MSG_ERROR([Required library zlib not found.]);
	else
		ZLIBREQ=
		ZLIBCPPFLAGS=
		ZLIBLIBS="-lz"
		LIBMAUSPKGLIBS="${LIBMAUSPKGLIBS} -lz"
	fi
fi

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(boost/regex.hpp, [boost_regex_hpp=yes], [boost_regex_hpp=no])
AC_LANG_POP

AC_LANG_PUSH([C++])
LIBSSAVE="${LIBS}"
LIBS="${LIBS} -lboost_regex"
AC_MSG_CHECKING([whether we can compile a program using the boost regex library])
AC_TRY_LINK([#include <boost/regex.hpp>],[::boost::regex reg("hello world");
	::boost::regex_replace(std::string("in hello world replace"),reg,std::string("hu globe"));
],[boost_regex=yes],[boost_regex=no])
AC_MSG_RESULT([${boost_regex}])
LIBS="${LIBSSAVE}"
AC_LANG_POP

if [[ "${boost_regex} = yes" ]] ; then
	LIBMAUSHAVEBOOSTREGEX="#define LIBMAUS_HAVE_BOOST_REGEX"
else
	LIBMAUSHAVEBOOSTREGEX
fi

LIBMAUSHAVESNAPPY=
SNAPPYCPPFLAGS=
SNAPPYLDFLAGS=
SNAPPYLIBS=
AC_ARG_WITH([snappy],
            [AS_HELP_STRING([--with-snappy@<:@=PATH@:>@], [support for snappy compression @<:@default=no@:>@])],
            [with_snappy=${withval}],
            [with_snappy=no])

if [[ "${with_snappy}" != "no" ]] ; then
	LIBSSAVE="${LIBS}"
	LDFLAGSSAVE="${LDFLAGS}"
	CPPFLAGSSAVE="${CPPFLAGS}"

	if [[ \( ! -z "${with_snappy}" \) -a \( "${with_snappy}" != "yes" \) ]] ; then
		SNAPPYCPPFLAGS="-I${with_snappy}/include"
		SNAPPYLDFLAGS="-L${with_snappy}/lib"
	fi
	
	SNAPPYLIBS="-lsnappy"

	LIBS="${LIBS} ${SNAPPYLIBS}"
	LDFLAGS="${LDFLAGS} ${SNAPPYLDFLAGS}"
	CPPFLAGS="${CPPFLAGS} ${SNAPPYCPPFLAGS}"

	AC_LANG_PUSH([C++])

	AC_CHECK_HEADER(snappy.h, [snappy_h=yes], [snappy_h=no])

	AC_MSG_CHECKING([whether we can compile a program using snappy])
	AC_TRY_LINK([#include <snappy.h>
#include <cstring>],[char const * input = "hello snappy";
size_t const input_length = strlen(input);
std::string output;
snappy::Compress(input,input_length,&output);
;],[snappy=yes],[snappy=no])
	AC_MSG_RESULT([${snappy}])

	AC_LANG_POP

	CPPFLAGS="${CPPFLAGSSAVE}"
	LDFLAGS="${LDFLAGSSAVE}"
	LIBS="${LIBSSAVE}"

	if [[ "${snappy}" = "yes" ]] ; then
		LIBMAUSHAVESNAPPY="#define LIBMAUS_HAVE_SNAPPY"
		LIBMAUSPKGLIBS="${LIBMAUSPKGLIBS} ${SNAPPYLDFLAGS} ${SNAPPYLIBS}"
		LIBMAUSPKGCPPFLAGS="${LIBMAUSPKGCPPFLAGS} ${SNAPPYCPPFLAGS}"
	fi
fi


AC_LANG_PUSH([C++])
AC_HEADER_STDC
AC_LANG_POP

AC_ARG_ENABLE(aio,
        AS_HELP_STRING([--enable-aio],[enable asynchronous I/O classes if available (default yes)]),
        [useaio=${enableval}],[useaio=yes])

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(aio.h, [aio_h=yes], [aio_h=no])
AC_LANG_POP

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(execinfo.h, [execinfo_h=yes], [execinfo_h=no])
AC_LANG_POP

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(windows.h, [windows_h=yes], [windows_h=no])
AC_LANG_POP

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(unistd.h, [unistd_h=yes], [unistd_h=no])
AC_LANG_POP

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(features.h, [features_h=yes], [features_h=no])
AC_LANG_POP

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(stdint.h, [stdint_h=yes], [stdint_h=no])
AC_LANG_POP

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(dlfcn.h, [dlfcn_h=yes], [dlfcn_h=no])
AC_LANG_POP

HAVEDLFCNH=
if [[ "${dlfcn_h}" = "yes" ]] ; then 
	HAVEDLFCNH="#define LIBMAUS_HAVE_DLFCN_H"
fi

DLFUNCS=""
have_dynamic_loading=no
if [[ "${dlfcn_h}" = "yes" ]] ; then 
	AC_LANG_PUSH([C++])
	AC_CHECK_FUNC(dlopen,dlopen_nolib=yes,dlopen_nolib=no)
	AC_CHECK_FUNC(dlerror,dlerror_nolib=yes,dlerror_nolib=no)
	AC_CHECK_FUNC(dlsym,dlsym_nolib=yes,dlsym_nolib=no)
	AC_CHECK_FUNC(dlclose,dlclose_nolib=yes,dlclose_nolib=no)
	AC_LANG_POP

	AC_LANG_PUSH([C++])
	AC_CHECK_LIB([dl],[dlopen],[dlopen_lib_dl=yes],[dlopen_libdl=no])
	AC_CHECK_LIB([dl],[dlerror],[dlerror_lib_dl=yes],[dlerror_libdl=no])
	AC_CHECK_LIB([dl],[dlsym],[dlsym_lib_dl=yes],[dlsym_libdl=no])
	AC_CHECK_LIB([dl],[dlclose],[dlclose_lib_dl=yes],[dlclose_libdl=no])
	AC_LANG_POP	

        AC_MSG_CHECKING([whether we found the dlopen function anywhere])
	if [[ "${dlopen_nolib}" = "yes" -o "${dlopen_lib_dl}" = "yes" ]] ; then
		dlopen_any=yes
	else
		dlopen_any=no
	fi
	AC_MSG_RESULT([$dlopen_any])

        AC_MSG_CHECKING([whether we found the dlerror function anywhere])
	if [[ "${dlerror_nolib}" = "yes" -o "${dlerror_lib_dl}" = "yes" ]] ; then
		dlerror_any=yes
	else
		dlerror_any=no
	fi
	AC_MSG_RESULT([$dlerror_any])

        AC_MSG_CHECKING([whether we found the dlsym function anywhere])
	if [[ "${dlsym_nolib}" = "yes" -o "${dlsym_lib_dl}" = "yes" ]] ; then
		dlsym_any=yes
	else
		dlsym_any=no
	fi
	AC_MSG_RESULT([$dlsym_any])

        AC_MSG_CHECKING([whether we found the dlclose function anywhere])
	if [[ "${dlclose_nolib}" = "yes" -o "${dlclose_lib_dl}" = "yes" ]] ; then
		dlclose_any=yes
	else
		dlclose_any=no
	fi
	AC_MSG_RESULT([$dlclose_any])

        AC_MSG_CHECKING([whether we found all the functions required for dynamic modules somewhere])
	if [[ "${dlopen_any}" = "yes" -a "${dlerror_any}" = "yes" -a "${dlsym_any}" = "yes" -a "${dlclose_any}" = "yes" ]] ; then
		dlall_any=yes
	else
		dlall_any=no
	fi
	AC_MSG_RESULT([$dlall_any])

	if [[ "$dlall_any" = "yes" ]] ; then
	        AC_MSG_CHECKING([whether we need the dl library for any of the dl functions])
		if [[ "${dlopen_nolib}" = "no" -o "${dlerror_nolib}" = "no" -o "${dlsym_nolib}" = "no" -o "${dlclose_nolib}" = "no" ]] ; then
			dlneedlibdl=yes
			DLLIB="-ldl"
			LIBMAUSPKGLIBS="${LIBMAUSPKGLIBS} ${DLLIB}"
		else
			dlneedlibdl=no
			DLLIB=""
		fi
		AC_MSG_RESULT([$dlneedlibdl])

		DLFUNCS="#define LIBMAUS_HAVE_DL_FUNCS"
		have_dynamic_loading=yes
	fi
	
	# if [[ "${backtrace}" = "yes" ]] ; then
	#	HAVEBACKTRACE="#define LIBMAUS_HAVE_BACKTRACE"
	# fi
fi


HAVEUNISTDH=
if [[ "${unistd_h}" = "yes" ]] ; then 
	HAVEUNISTDH="#define LIBMAUS_HAVE_UNISTD_H"
fi

HAVEFEATURESH=
if [[ "${features_h}" = "yes" ]] ; then
        HAVEHAVEFEATURESH="#define LIBMAUS_HAVE_FEATURES_H"
fi

HAVESTDINTH=
if [[ "${stdint_h}" = "yes" ]] ; then 
	HAVESTDINTH="#define LIBMAUS_HAVE_STDINT_H"
fi

HAVEAIO=
if [[ \( "${aio_h}" = "yes" \) -a \( "${useaio}" = "yes" \) ]] ; then 
	AC_LANG_PUSH([C++])
	AC_CHECK_LIB([rt],[aio_suspend],[aiolib=yes],[aiolib=no])
	AC_LANG_POP
	
	if [[ "${aiolib}" = "yes" ]] ; then
		HAVEAIO="#define LIBMAUS_HAVE_AIO"
		LIBMAUSLIBS="${LIBMAUSLIBS} -lrt"
	fi
fi

if [[ "${execinfo_h}" = "yes" ]] ; then 
	AC_LANG_PUSH([C++])
	AC_CHECK_FUNC(backtrace,backtrace=yes,backtrace=no)
	AC_LANG_POP
	
	if [[ "${backtrace}" = "yes" ]] ; then
		HAVEBACKTRACE="#define LIBMAUS_HAVE_BACKTRACE"
	fi
fi

if [[ "${windows_h}" = "yes" ]] ; then 
	HAVEWINDOWSH="#define LIBMAUS_HAVE_WINDOWS_H"
fi

HAVEEXECINFO=
if [[ "${execinfo_h}" = "yes" ]] ; then 
	LIBMAUS_HAVE_EXECINFO_H="#define LIBMAUS_HAVE_EXECINFO_H"
fi

AC_LANG_PUSH([C++])
AC_CHECK_FUNC(setrlimit,setrlimit=yes,setrlimit=no)
AC_LANG_POP

if [[ "${setrlimit}" = "yes" ]] ; then 
	HAVESETRLIMIT="#define LIBMAUS_HAVE_SETRLIMIT"
fi

LIBMAUSLIBS="${LIBMAUSLIBS} ${ZLIBLIBS}"

AC_LANG_PUSH([C++])
CXXFLAGS_SAVE="${CXXFLAGS}"
CXXFLAGS="-std=gnu++0x"
AC_MSG_CHECKING([whether the C++ compiler supports the -std=gnu++0x flag])
AC_TRY_COMPILE([#include <cstdint>],[],[nullx=yes],[nullx=no])
AC_MSG_RESULT([${nullx}])
CXXFLAGS="${CXXFLAGS_SAVE}"
AC_LANG_POP

if test "${nullx}" = "yes" ; then
	CXXFLAGS_SAVE="${CXXFLAGS}"
	CXXFLAGS="-std=gnu++0x"
	AC_LANG_PUSH([C++])
	AC_CHECK_HEADER(cstdint, [cstdint=yes], [cstdint=no])
	AC_LANG_POP
	CXXFLAGS="${CXXFLAGS_SAVE}"

	LIBMAUSCXXFLAGS="${LIBMAUSCXXFLAGS} -std=gnu++0x"
	LIBMAUSPKGCXXFLAGS="${LIBMAUSPKGCXXFLAGS} -std=gnu++0x"
fi

AC_LANG_PUSH([C++])
CXXFLAGS_SAVE="${CXXFLAGS}"
CXXFLAGS="-msse4"
AC_MSG_CHECKING([whether the C++ compiler supports the -msse4 flag])
AC_TRY_COMPILE([],[__builtin_popcount(4ul)],[sse4flag=yes],[sse4flag=no])
AC_MSG_RESULT([${sse4flag}])
CXXFLAGS="${CXXFLAGS_SAVE}"
AC_LANG_POP

WARNCPPFLAGS=

# effc++ strict-null-sentinel no-non-template-friend old-style-cast overloaded-virtual sign-promo non-virtual-dtor abi extra

for flag in "" all abi non-virtual-dtor ; do
	AC_LANG_PUSH([C++])
	CPPFLAGS_SAVE="${CPPFLAGS}"
	CPPFLAGS="-W${flag}"
	AC_MSG_CHECKING([whether the C++ compiler supports the -W${flag} flag])
	AC_TRY_COMPILE([],[int c = 0;],[wflag=yes],[wflag=no])
	AC_MSG_RESULT([${wflag}])
	CPPFLAGS="${CPPFLAGS_SAVE}"
	AC_LANG_POP

	if test "${wflag}" = "yes" ; then
		WARNCPPFLAGS="${WARNCPPFLAGS} -W${flag}"
	fi
done

if test "${CXX}" = "cl.exe" ; then
	CXXFLAGS="${CXXFLAGS} -EHsc -Gr"
	CPPFLAGS="${CPPFLAGS} -W1" # -Wall
fi

AC_ARG_ENABLE(optimization,
        AS_HELP_STRING([--enable-optimization],[use compiler optimization (default yes)]),
        [optimization=${enableval}],[optimization=yes])
AC_ARG_ENABLE(debug,
        AS_HELP_STRING([--enable-debug],[use compiler debug flags (default no)]),
        [debug=${enableval}],[debug=no])
AC_ARG_ENABLE(werror,
        AS_HELP_STRING([--enable-werror],[treat compiler warnings as errors (default no)]),
        [werror=${enableval}],[werror=no])
AC_ARG_ENABLE(profile,
        AS_HELP_STRING([--enable-profile],[use compiler profiling flags (default no)]),
        [profile=${enableval}],[profile=no])
AC_ARG_ENABLE(fast,
        AS_HELP_STRING([--enable-fast],[disable evaluation of assertions (default no)]),
        [fast=${enableval}],[fast=no])
AC_ARG_ENABLE(sse4,
        AS_HELP_STRING([--enable-sse4],[enable sse4 operations (default no)]),
        [sse4=${enableval}],[sse4=no])
AC_ARG_ENABLE(ssse3,
        AS_HELP_STRING([--enable-ssse3],[enable ssse3 operations (default no)]),
        [ssse3=${enableval}],[ssse3=no])
AC_ARG_ENABLE(asm,
        AS_HELP_STRING([--enable-asm],[use inline assembly code (default yes)]),
        [assembly=${enableval}],[assembly=yes])
AC_ARG_ENABLE(shared-libmaus,
        AS_HELP_STRING([--enable-shared-libmaus],[enable compiling shared libmaus.so and libmauslsf.so]),
        [sharedlibmaus=${enableval}],[sharedlibmaus=yes])

LIBMAUSSTATIC=
if test "${sharedlibmaus}" = "no" ; then
	LIBMAUSSTATIC="-static"
fi

if test "${fast}" = "yes" ; then
	CPPFLAGS="${CPPFLAGS} -DNDEBUG"
fi

if test "${werror}" = "yes" ; then
	CXXFLAGS="${CXXFLAGS} -Werror"
	CFLAGS="${CFLAGS} -Werror"
fi

AC_ARG_ENABLE(openmp,
        AS_HELP_STRING([--enable-openmp],[use OpenMP (default yes)]),
        [openmp=${enableval}],[openmp=yes])

if test "${openmp}" = "yes" ; then
	AC_LANG_PUSH([C++])
	AX_OPENMP(have_openmp=yes,have_openmp=no)
	AC_LANG_POP
fi

if test "${debug}" = "yes" ; then
        CXXFLAGS="${CXXFLAGS} -g -O0 -rdynamic"
        CFLAGS="${CFLAGS} -g -O0 -rdynamic"
	openmp="no"
else
	if test "${profile}" = "yes" ; then
	        CXXFLAGS="${CXXFLAGS} -g -pg -rdynamic"
	        CFLAGS="${CFLAGS} -g -pg -rdynamic"
	else
		if test "${optimization}" = "yes" ; then
			case ${CXX} in
				g++)
					CXXFLAGS="${CXXFLAGS} -O3 -rdynamic"
					CFLAGS="${CFLAGS} -O3 -rdynamic"
					;;
				*-mingw32msvc-g++)
					CXXFLAGS="${CXXFLAGS} -O3 -rdynamic"
					CFLAGS="${CFLAGS} -O3 -rdynamic"
					;;
				cl.exe)
					CXXFLAGS="${CXXFLAGS} -O2 -Ob2 -Ot -Oy"
					;;
			esac
		fi

	fi
fi



AC_LANG_PUSH([C++])
CPPFLAGS_SAVE="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS}"
if test "${nullx}" = "yes" ; then
	CPPFLAGS="${CPPFLAGS} -std=gnu++0x"
fi
LIBS_SAVE="${LIBS}"
LIBS="${LIBS}"

AC_MSG_CHECKING([whether std::unique_ptr exists])
                AC_LINK_IFELSE([AC_LANG_SOURCE([#include <memory>

        int main() {
                std::unique_ptr<int> U;
                return 0;
        }])],
                have_std_unique_ptr=yes,
                have_std_unique_ptr=no
        )
AC_MSG_RESULT($have_std_unique_ptr)
CPPFLAGS="${CPPFLAGS_SAVE}"
LIBS="${LIBS_SAVE}"
AC_LANG_POP

AC_LANG_PUSH([C++])
CPPFLAGS_SAVE="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS}"
if test "${nullx}" = "yes" ; then
	CPPFLAGS="${CPPFLAGS} -std=gnu++0x"
fi
LIBS_SAVE="${LIBS}"
LIBS="${LIBS}"

AC_MSG_CHECKING([whether std::shared_ptr exists])
                AC_LINK_IFELSE([AC_LANG_SOURCE([#include <memory>

        int main() {
                std::shared_ptr<int> U;
                return 0;
        }])],
                have_std_shared_ptr=yes,
                have_std_shared_ptr=no
        )
AC_MSG_RESULT($have_std_shared_ptr)
CPPFLAGS="${CPPFLAGS_SAVE}"
LIBS="${LIBS_SAVE}"
AC_LANG_POP

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(boost/interprocess/smart_ptr/unique_ptr.hpp,[have_boost_unique_include=yes],[have_boost_unique_input=no])
AC_LANG_POP

if test "${have_boost_unique_include}" = "yes" ; then
    AC_LANG_PUSH([C++])
    CPPFLAGS_SAVE="${CPPFLAGS}"
    LIBS_SAVE="${LIBS}"
    AC_MSG_CHECKING([whether boost::interprocess::unique_ptr exists])
                    AC_LINK_IFELSE([AC_LANG_SOURCE([#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

            template<typename T>
	    struct Delete
            {
		void operator()(T * t) const
		{
			delete t;
		}
            };

            int main() {
                    boost::interprocess::unique_ptr<int, Delete<int> > U;
                    return 0;
            }])],
                    have_boost_unique_ptr=yes,
                    have_boost_unique_ptr=no
            )
    AC_MSG_RESULT($have_boost_unique_ptr)
    CPPFLAGS="${CPPFLAGS_SAVE}"
    LIBS="${LIBS_SAVE}"
    AC_LANG_POP
else
    have_boost_unique_ptr=no
    # AC_MSG_ERROR([Required header boost/interprocess/smart_ptr/unique_ptr.hpp not found])
fi

UNIQUEPTR=

if test "${have_std_unique_ptr}" = "yes" ; then
	UNIQUEPTR="#define LIBMAUS_USE_STD_UNIQUE_PTR"
elif test "${have_boost_unique_ptr}" = "yes" ; then
	UNIQUEPTR="#define LIBMAUS_USE_BOOST_UNIQUE_PTR"
else
	AC_MSG_ERROR([Required unique_ptr class not found (not in standard and not in boost)])
fi

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(boost/shared_ptr.hpp,[have_boost_shared_include=yes],[have_boost_shared_input=no])
AC_LANG_POP

if test "${have_boost_shared_include}" = "yes" ; then
    AC_LANG_PUSH([C++])
    CPPFLAGS_SAVE="${CPPFLAGS}"
    LIBS_SAVE="${LIBS}"
    AC_MSG_CHECKING([whether boost::shared_ptr exists])
                    AC_LINK_IFELSE([AC_LANG_SOURCE([#include <boost/shared_ptr.hpp>

            int main() {
		    boost::shared_ptr<int> U;
                    return 0;
            }])],
                    have_boost_shared_ptr=yes,
                    have_boost_shared_ptr=no
            )
    AC_MSG_RESULT($have_boost_shared_ptr)
    CPPFLAGS="${CPPFLAGS_SAVE}"
    LIBS="${LIBS_SAVE}"
    AC_LANG_POP
else
    have_boost_shared_ptr=no
fi

if test "${have_std_shared_ptr}" = "yes" ; then
	SHAREDPTR="#define LIBMAUS_USE_STD_SHARED_PTR"
elif test "${have_boost_shared_ptr}" = "yes" ; then
	SHAREDPTR="#define LIBMAUS_USE_BOOST_SHARED_PTR"
else
	AC_MSG_ERROR([Required shared_ptr class not found (not in standard and not in boost)])
fi

AC_LANG_PUSH([C++])
CPPFLAGS_SAVE="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS}"
if test "${nullx}" = "yes" ; then
	CPPFLAGS="${CPPFLAGS} -std=gnu++0x"
fi
LIBS_SAVE="${LIBS}"
LIBS="${LIBS}"

AC_MSG_CHECKING([whether std::unordered_map exists])
                AC_LINK_IFELSE([AC_LANG_SOURCE([#include <unordered_map>

        int main() {
                std::unordered_map<int,int> U;
                return 0;
        }])],
                have_std_unordered_map=yes,
                have_std_unordered_map=no
        )
AC_MSG_RESULT($have_std_unordered_map)
CPPFLAGS="${CPPFLAGS_SAVE}"
LIBS="${LIBS_SAVE}"
AC_LANG_POP

AC_LANG_PUSH([C++])
CPPFLAGS_SAVE="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS}"
if test "${nullx}" = "yes" ; then
	CPPFLAGS="${CPPFLAGS} -std=gnu++0x"
fi
LIBS_SAVE="${LIBS}"
LIBS="${LIBS}"

AC_MSG_CHECKING([whether std::unordered_set exists])
                AC_LINK_IFELSE([AC_LANG_SOURCE([#include <unordered_set>

        int main() {
                std::unordered_set<int> U;
                return 0;
        }])],
                have_std_unordered_set=yes,
                have_std_unordered_set=no
        )
AC_MSG_RESULT($have_std_unordered_set)
CPPFLAGS="${CPPFLAGS_SAVE}"
LIBS="${LIBS_SAVE}"
AC_LANG_POP

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(boost/unordered_map.hpp,[have_boost_unordered_map=yes],[have_boost_unordered_map=no])
AC_LANG_POP

if test "${have_boost_unordered_map}" = "yes" ; then
    AC_LANG_PUSH([C++])
    CPPFLAGS_SAVE="${CPPFLAGS}"
    LIBS_SAVE="${LIBS}"
    AC_MSG_CHECKING([whether boost::unordered_map exists])
                    AC_LINK_IFELSE([AC_LANG_SOURCE([#include <boost/unordered_map.hpp>

            int main() {
		    boost::unordered_map<int,int> U;
                    return 0;
            }])],
                    have_boost_unordered_map=yes,
                    have_boost_unordered_map=no
            )
    AC_MSG_RESULT($have_boost_unordered_map)
    CPPFLAGS="${CPPFLAGS_SAVE}"
    LIBS="${LIBS_SAVE}"
    AC_LANG_POP
else
    have_boost_unordered_map=no
fi

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(boost/unordered_set.hpp,[have_boost_unordered_set=yes],[have_boost_unordered_set=no])
AC_LANG_POP

if test "${have_boost_unordered_set}" = "yes" ; then
    AC_LANG_PUSH([C++])
    CPPFLAGS_SAVE="${CPPFLAGS}"
    LIBS_SAVE="${LIBS}"
    AC_MSG_CHECKING([whether boost::unordered_set exists])
                    AC_LINK_IFELSE([AC_LANG_SOURCE([#include <boost/unordered_set.hpp>

            int main() {
		    boost::unordered_set<int> U;
                    return 0;
            }])],
                    have_boost_unordered_set=yes,
                    have_boost_unordered_set=no
            )
    AC_MSG_RESULT($have_boost_unordered_set)
    CPPFLAGS="${CPPFLAGS_SAVE}"
    LIBS="${LIBS_SAVE}"
    AC_LANG_POP
else
    have_boost_unordered_set=no
fi


if test "${have_std_unordered_set}" = "yes" ; then
	UNORDEREDSET="#define LIBMAUS_USE_STD_UNORDERED_SET"
elif test "${have_boost_unordered_set}" = "yes" ; then
	UNORDEREDSET="#define LIBMAUS_USE_BOOST_UNORDERED_SET"
else
	AC_MSG_ERROR([Required unordered_set class not found (not in standard and not in boost)])
fi

if test "${have_std_unordered_map}" = "yes" ; then
	UNORDEREDMAP="#define LIBMAUS_USE_STD_UNORDERED_MAP"
elif test "${have_boost_unordered_map}" = "yes" ; then
	UNORDEREDMAP="#define LIBMAUS_USE_BOOST_UNORDERED_MAP"
else
	AC_MSG_ERROR([Required unordered_map class not found (not in standard and not in boost)])
fi

AC_LANG_PUSH([C++])
CPPFLAGS_SAVE="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS}"
if test "${nullx}" = "yes" ; then
	CPPFLAGS="${CPPFLAGS} -std=gnu++0x"
fi
LIBS_SAVE="${LIBS}"
LIBS="${LIBS}"

AC_MSG_CHECKING([whether std::move exists])
                AC_LINK_IFELSE([AC_LANG_SOURCE([#include <tuple>

        int main() {
		std::move(0);
                return 0;
        }])],
                have_std_move=yes,
                have_std_move=no
        )
AC_MSG_RESULT($have_std_move)
CPPFLAGS="${CPPFLAGS_SAVE}"
LIBS="${LIBS_SAVE}"
AC_LANG_POP

AC_LANG_PUSH([C++])
CPPFLAGS_SAVE="${CPPFLAGS}"
CPPFLAGS="${CPPFLAGS}"
if test "${nullx}" = "yes" ; then
	CPPFLAGS="${CPPFLAGS} -std=gnu++0x"
fi
LIBS_SAVE="${LIBS}"
LIBS="${LIBS}"

AC_MSG_CHECKING([whether boost::move exists])
                AC_LINK_IFELSE([AC_LANG_SOURCE([#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/move/move.hpp>

            template<typename T>
	    struct Delete
            {
		void operator()(T * t) const
		{
			delete t;
		}
            };

            int main() {
                    boost::interprocess::unique_ptr<int, Delete<int> > U(new int);
                    boost::interprocess::unique_ptr<int, Delete<int> > V = ::boost::move(U);
                    return 0;
            }])],
                have_boost_move=yes,
                have_boost_move=no
        )
AC_MSG_RESULT($have_boost_move)
CPPFLAGS="${CPPFLAGS_SAVE}"
LIBS="${LIBS_SAVE}"
AC_LANG_POP

if test "${have_std_move}" = "yes" ; then
	MOVEFUNC="#define LIBMAUS_USE_STD_MOVE"
elif test "${have_boost_move}" = "yes" ; then
	MOVEFUNC="#define LIBMAUS_USE_BOOST_MOVE"
else
	AC_MSG_ERROR([Required move function for unique_ptr not found (not in standard and not in boost)])
fi

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(sys/types.h, [sys_types=yes], [sys_types=no])
AC_CHECK_HEADER(sys/vfs.h, [sys_vfs_h=yes], [sys_vfs_h=no])
AC_CHECK_HEADER(windows.h, [windows_h=yes], [windows_h=no])
AC_CHECK_HEADER(sys/time.h, [sys_time_h=yes], [sys_time_h=no])
AC_CHECK_HEADER(regex.h, [regex_h=yes], [regex_h=no])
AC_LANG_POP

SEQANCPPFLAGS=
SEQANLDFLAGS=
SEQANLIBS=
AC_ARG_WITH([seqan],
            [AS_HELP_STRING([--with-seqan@<:@=PATH@:>@], [support for SeqAn library @<:@default=no@:>@])],
            [with_seqan=${withval}],
            [with_seqan=no])

if [[ "${with_seqan}" != "no" ]] ; then

	if [[ \( ! -z "${with_snappy}" \) -a \( "${with_snappy}" != "yes" \) ]] ; then
		SEQANCPPFLAGS="-I${with_seqan}/include"
		SEQANLDFLAGS="-L${with_seqan}/lib"
	fi

	CPPFLAGSSAVE="${CPPFLAGS}"
	LDFLAGSSAVE="${LDFLAGS}"
	CPPFLAGS="${CPPFLAGS} ${SEQANCPPFLAGS}"
	LDFLAGS="${LDFLAGS} ${SEQANLDFLAGS}"

	AC_LANG_PUSH([C++])
	AC_CHECK_HEADER(seqan/align.h, [seqan_align_h=yes], [seqan_align_h=no])
	AC_CHECK_HEADER(seqan/graph_msa.h, [seqan_graph_msa_h=yes], [seqan_graph_msa_h=no])
	AC_CHECK_HEADER(seqan/score.h, [seqan_score_h=yes], [seqan_score_h=no])
	AC_LANG_POP

	CPPFLAGS="${CPPFLAGSSAVE}"
	LDFLAGS="${LDFLAGSSAVE}"

	if test "x$seqan_align_h" = "xyes" -a "x$seqan_graph_msa_h" = "xyes" -a "x$seqan_score_h" = "xyes" ; then
		CPPFLAGSSAVE="${CPPFLAGS}"
		LDFLAGSSAVE="${LDFLAGS}"
		CPPFLAGS="${CPPFLAGS} ${SEQANCPPFLAGS}"
		LDFLAGS="${LDFLAGS} ${SEQANLDFLAGS}"

		AC_LANG_PUSH([C++])
		AC_MSG_CHECKING([whether we can compile a SeqAN program])
				AC_LINK_IFELSE([AC_LANG_SOURCE([#include <iostream>
	#include <vector>
	#include <cassert>
	#include <seqan/align.h>
	#include <seqan/graph_msa.h>
	#include <seqan/score.h>

	struct ScoringMatrix : public seqan::Score < int, seqan::ScoreMatrix<seqan::Dna5, seqan::Default> >
	{
		typedef ScoringMatrix this_type;
		typedef seqan::Dna5 TSequenceValue;
		typedef seqan::Score < int, seqan::ScoreMatrix<TSequenceValue, seqan::Default> > base_type;

		// gap penalties
		static int const gap_open_penalty = -4;
		static int const gap_extend_penalty = -3;

		// Print a scoring scheme matrix to stdout.
		void showMatrix()
		{
			// Print top row.
			for (unsigned i = 0; i < seqan::ValueSize<TSequenceValue>::VALUE; ++i)
				std::cout << "\t" << TSequenceValue(i);
			std::cout << std::endl;
			
			// Print each row.
			for (unsigned i = 0; i < seqan::ValueSize<TSequenceValue>::VALUE; ++i) {
				std::cout << TSequenceValue(i);
				for (unsigned j = 0; j < seqan::ValueSize<TSequenceValue>::VALUE; ++j) {
					std::cout << "\t" << score(*this, TSequenceValue(i), TSequenceValue(j));
				}
				std::cout << std::endl;
			}
		}

		ScoringMatrix() : base_type(gap_open_penalty,gap_extend_penalty)
		{
			/* keeping base: good(1), changing base: bad(-1) */
			seqan::setScore(*this,'A','A',1);
			seqan::setScore(*this,'A','C',-1);
			seqan::setScore(*this,'A','G',-1);
			seqan::setScore(*this,'A','T',-1);

			seqan::setScore(*this,'C','A',-1);
			seqan::setScore(*this,'C','C', 1);
			seqan::setScore(*this,'C','G',-1);
			seqan::setScore(*this,'C','T',-1);

			seqan::setScore(*this,'G','A',-1);
			seqan::setScore(*this,'G','C',-1);
			seqan::setScore(*this,'G','G', 1);
			seqan::setScore(*this,'G','T',-1);

			seqan::setScore(*this,'T','A',-1);
			seqan::setScore(*this,'T','C',-1);
			seqan::setScore(*this,'T','G',-1);
			seqan::setScore(*this,'T','T', 1);

			/* changing N to base: ok(0) */
			seqan::setScore(*this,'N','A', 0);
			seqan::setScore(*this,'N','C', 0);
			seqan::setScore(*this,'N','G', 0);
			seqan::setScore(*this,'N','T', 0);

			/* changing base to N: bad(-1) */
			seqan::setScore(*this,'A','N', -1);
			seqan::setScore(*this,'C','N', -1);
			seqan::setScore(*this,'G','N', -1);
			seqan::setScore(*this,'T','N', -1);
			seqan::setScore(*this,'N','N',  0);
		}
	};

	struct ConsensusComputation
	{
		ScoringMatrix const mscoring;
		ScoringMatrix::base_type const & scoring;

		static char mapChar(char const c)
		{
			switch ( c )
			{
				case 'A': case 'a': return 0;
				case 'C': case 'c': return 1;
				case 'G': case 'g': return 2;
				case 'T': case 't': return 3;
				case 'N': case 'n': return 4;
				default:  return 5;
			}
		}

		static char remapChar(char const c)
		{
			static char const * m = "ACGTN-";
			return m[[static_cast<int>(c)]];
		}

		ConsensusComputation() : mscoring(), scoring(mscoring)
		{
			// mscoring.showMatrix();
		}
		
		std::string computeConsensus(std::vector<std::string> const & V, bool const verbose = false) const
		{
			seqan::Align< seqan::String<seqan::Dna5> > align;
			seqan::resize(rows(align), V.size());
		
			for ( uint64_t i = 0; i < V.size(); ++i )
				seqan::assignSource(row(align, i),V[[i]]);	

			// compute multiple alignment
			seqan::globalMsaAlignment(align, scoring);

			unsigned int const rowlen = length(rows(align)) ? length(rows(align)[[0]]) : 0;
			
			// compute frequencies per column	
			std::vector < int > cons(6*rowlen);

			for ( unsigned int r = 0; r < length(rows(align)); ++r )
			{
				seqan::Gaps< seqan::String<seqan::Dna5>, seqan::ArrayGaps > const & arow = rows(align)[[r]];

				if ( verbose )
					std::cerr << arow << "\t";
				
				unsigned int exp = 0;
				for ( unsigned int i = 0; i < length(source(arow)); ++i )
				{
					unsigned int const viewpos = toViewPosition(arow, i) ;
					
					while ( exp < viewpos )
					{
						cons[[(exp * 6) + mapChar('-')]] ++;
						exp++;
					}
				
					if ( verbose )
						std::cerr << viewpos
							<< "(" << source(arow)[[i]] << ")"
							<< (((i+1)<length(source(arow)))?",":"")
						;
					
					cons[[(exp*6) + mapChar(source(arow)[i])]]++;
					
					exp++;
				}
				
				while ( exp < rowlen )
				{		
					cons[[(exp * 6) + mapChar('-')]] ++;
					exp++;
				}
				
				if ( verbose )
					std::cerr << std::endl;
			}
			
			// compute pre consensus by majority vote
			// (use lex. smallest character with maximum freq per column)
			std::string preconsensus(rowlen,'-');
			unsigned int conslen = 0;
			for ( unsigned int c = 0; c < rowlen; ++c )
			{
				int sum = 0;
				int maxval = -1;
				int maxchar = -1;
				
				for ( unsigned int i = 0; i < 6; ++i )
				{
					if ( cons[[c*6+i]] )
					{
						if ( verbose )
							std::cerr << remapChar(i) << ":" << cons[[c*6+i]] << ";";
						
						if ( cons[[c*6+i]] > maxval )
						{
							maxval = cons[[c*6+i]];
							maxchar = i;
						}
					}
						
					sum += cons[[c*6+i]];
				}
				
				assert ( sum == static_cast<int>(length(rows(align))) );
				
				preconsensus[[c]] = remapChar(maxchar);
				
				if ( maxchar >= 0 && maxchar <= 4 )
					conslen++;
				
				if ( verbose )
					std::cerr << std::endl;
			}
			
			// build final consensus from pre consensus by dropping - columns
			std::string consensus(conslen,'\0');
			
			for ( unsigned int i = 0, j = 0; i < preconsensus.size(); ++i )
				if ( preconsensus[[i]] != '-' )
					consensus[[j++]] = preconsensus[[i]];

			return consensus;
		}
	};

	int main()
	{
		ConsensusComputation CC;
		
		std::vector<std::string> V;
		V.push_back("AAAA");
		V.push_back("ATAA");
		V.push_back("AATA");
		V.push_back("GAAA");
		V.push_back("AANAA");
		V.push_back("AAAAT");
		
		std::string const consensus = CC.computeConsensus(V,true);
				
		std::cerr << "consensus: " << consensus << std::endl;
			   
		return 0;
	}])],
				have_seqan=yes,
				have_seqan=no
			)
		AC_MSG_RESULT($have_seqan)
		AC_LANG_POP

		CPPFLAGS="${CPPFLAGSSAVE}"
		LDFLAGS="${LDFLAGSSAVE}"

	else
		have_seqan="no"
	fi

	if [[ "${have_seqan}" = "yes" ]] ; then 
		SEQAN="#define LIBMAUS_HAVE_SEQAN"
	else
		SEQAN=
	fi
fi

AC_C_BIGENDIAN
if test "x$ac_cv_c_bigendian" = "xyes"; then
	ENDIANESS="#define LIBMAUS_BYTE_ORDER_BIG_ENDIAN"
else
	ENDIANESS="#define LIBMAUS_BYTE_ORDER_LITTLE_ENDIAN"
fi

if [[ "${sys_types}" = "yes" ]] ; then 
	SYS_TYPES_H="#define LIBMAUS_HAVE_SYS_TYPES_H"
else
	SYS_TYPES_H=
fi
if [[ "${sys_vfs_h}" = "yes" ]] ; then 
	HAVESYSVFSH="#define LIBMAUS_HAVE_SYS_VFS_H"
else
	HAVESYSVFSH=
fi
if [[ "${cstdint}" = "yes" ]] ; then 
	CSTDINT="#define LIBMAUS_HAVE_CSTDINT"
else
	CSTDINT=
fi
if [[ "${windows_h}" = "yes" ]] ; then 
	WINDOWSH="#define LIBMAUS_HAVE_WINDOWS_H"
else
	WINDOWSH=
fi
if [[ "${sys_time_h}" = "yes" ]] ; then 
	SYSTIMEH="#define LIBMAUS_HAVE_SYS_TIME_H"
else
	SYSTIMEH=
fi

if [[ "${regex_h}" = "yes" ]] ; then 

AC_MSG_CHECKING([whether we can compile a program using posix regex])
                AC_LINK_IFELSE([AC_LANG_SOURCE([#include <sys/types.h>
#include <regex.h>
#include <string.h>

            int main() {
		regex_t preg;
		regcomp(&preg,"hello",REG_EXTENDED);

		regmatch_t pmatch[[1]];
		memset(&pmatch,0,sizeof(pmatch));
		regexec(&preg,"test hello world hello world", sizeof(pmatch)/sizeof(pmatch[[0]]), &pmatch[[0]], 0);

		regfree(&preg);
		return 0;
            }])],
                have_posix_regex=yes,
                have_posix_regex=no
        )
AC_MSG_RESULT([$have_posix_regex])

	if test "${have_posix_regex}" = "yes" ; then
		REGEXH="#define LIBMAUS_HAVE_REGEX_H"
	else
		REGEXH=""
	fi
else
	REGEXH=
fi

if [[ "${sys_vfs_h}" = "yes" ]] ; then 
	AC_MSG_CHECKING([whether we can compile a program using statfs])
                AC_LINK_IFELSE([AC_LANG_SOURCE([#include <sys/vfs.h>

            int main() {
		struct statfs stfs;
		statfs("file",&stfs);
		return 0;
            }])],
                have_statfs=yes,
                have_statfs=no
        )
AC_MSG_RESULT([$have_statfs])

	if test "${have_statfs}" = "yes" ; then
		LIBMAUS_HAVE_STATFS="#define LIBMAUS_HAVE_STATFS"
	else
		LIBMAUS_HAVE_STATFS=""
	fi
else
	LIBMAUS_HAVE_STATFS=
fi

HAVESSE4=

if test "${sse4}" = "yes" ; then
	HAVESSE4="#define HAVE_SSE4"

	if test "${sse4flag}" = "yes" ; then
		LIBMAUSCXXFLAGS="${LIBMAUSCXXFLAGS} -msse4"
		LIBMAUSCFLAGS="${LIBMAUSCFLAGS} -msse4"
		LIBMAUSCPPFLAGS="${LIBMAUSCPPFLAGS}"
	fi
fi

if test "${ssse3}" = "yes" ; then
	HAVESSSE3="#define LIBMAUS_HAVE_SSSE3"
fi

if test "${assembly}" = "yes" ; then
	USEASSEMBLY="#define LIBMAUS_USE_ASSEMBLY"
fi

AC_LANG_PUSH([C++])
CXXFLAGS_SAVE="${CXXFLAGS}"
CXXFLAGS="-pthread"
AC_MSG_CHECKING([whether the C++ compiler supports the -pthread flag])
AC_TRY_COMPILE([#include <pthread.h>
pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;],[
	pthread_mutex_lock(&fastmutex);
	pthread_mutex_unlock(&fastmutex);
],[pthread=yes],[pthread=no])
AC_MSG_RESULT([${pthread}])
CXXFLAGS="${CXXFLAGS_SAVE}"
AC_LANG_POP

POSIXSPINLOCKS=
if test "${pthread}" = "yes" ; then
	AC_LANG_PUSH([C++])
	CXXFLAGS_SAVE="${CXXFLAGS}"
	CXXFLAGS="-pthread"
	AC_MSG_CHECKING([for pthread spin lock support])
	AC_TRY_COMPILE([#include <pthread.h>],[pthread_spinlock_t spinlock;
		pthread_spin_init(&spinlock,0);
		pthread_spin_lock(&spinlock);
		pthread_spin_unlock(&spinlock);
		pthread_spin_destroy(&spinlock);
	],[pthreadspinlocks=yes],[pthreadspinlocks=no])
	AC_MSG_RESULT([${pthreadspinlocks}])
	CXXFLAGS="${CXXFLAGS_SAVE}"
	AC_LANG_POP

	if test "${pthreadspinlocks}" = "yes" ; then
		POSIXSPINLOCKS="#define LIBMAUS_HAVE_POSIX_SPINLOCKS"
	fi
fi

if test "${pthread}" = "yes" ; then
	CXXFLAGS_SAVE="${CXXFLAGS}"
	CXXFLAGS="${CXXFLAGS} -pthread"

	AC_LANG_PUSH([C++])
	AC_CHECK_FUNC(sem_post,sem_post=yes,sem_post=no)
	AC_LANG_POP

	AC_LANG_PUSH([C++])
	AC_CHECK_FUNC(sem_wait,sem_wait=yes,sem_wait=no)
	AC_LANG_POP

	AC_LANG_PUSH([C++])
	AC_CHECK_FUNC(sem_init,sem_init=yes,sem_init=no)
	AC_LANG_POP

	AC_LANG_PUSH([C++])
	AC_CHECK_FUNC(sem_destroy,sem_destroy=yes,sem_destroy=no)
	AC_LANG_POP

	AC_LANG_PUSH([C++])
	AC_CHECK_FUNC(sem_timedwait,sem_timedwait=yes,sem_timedwait=no)
	AC_LANG_POP

	AC_LANG_PUSH([C++])
	AC_CHECK_FUNC(pthread_setname_np,pthread_setname_np_func=yes,pthread_setname_np_func=no)
	AC_CHECK_FUNC(pthread_set_name_np,pthread_set_name_np_func=yes,pthread_set_name_np_func=no)
	AC_CHECK_FUNC(pthread_getname_np,pthread_getname_np_func=yes,pthread_getname_np_func=no)
	AC_CHECK_FUNC(pthread_get_name_np,pthread_get_name_np_func=yes,pthread_get_name_np_func=no)
	AC_CHECK_FUNC(pthread_setaffinity_np,pthread_setaffinity_np_func=yes,pthread_setaffinity_np_func=no)
	AC_CHECK_FUNC(prctl,prctl_func=yes,prctl_func=no)
	AC_LANG_POP


	CXXFLAGS="${CXXFLAGS_SAVE}"
fi

HAVESEMPOST=
if [[ "${ac_cv_func_sem_post}" = "yes" ]] ; then
	HAVESEMPOST="#define HAVE_SEM_POST"
else
	pthread="no"
fi

HAVESEMWAIT=
if [[ "${ac_cv_func_sem_wait}" = "yes" ]] ; then
	HAVESEMWAIT="#define LIBMAUS_HAVE_SEM_WAIT"
else
	pthread="no"
fi

HAVESEMINIT=
if [[ "${ac_cv_func_sem_init}" = "yes" ]] ; then
	HAVESEMINIT="#define LIBMAUS_HAVE_SEM_INIT"
else
	pthread="no"
fi

HAVESEMDESTROY=
if [[ "${ac_cv_func_sem_destroy}" = "yes" ]] ; then
	HAVESEMDESTROY="#define LIBMAUS_HAVE_SEM_DESTROY"
else
	pthread="no"
fi

HAVESEMTIMEDWWAIT=
if [[ "${ac_cv_func_sem_timedwait}" = "yes" ]] ; then
	HAVESEMPOST="#define LIBMAUS_HAVE_SEM_TIMEDWAIT"
fi

HAVEPTHREADSETNAME_NP=
if [[ "${ac_cv_pthread_setname_np_func}" = "yes" ]] ; then
	HAVEPTHREADSETNAME_NP="#define LIBMAUS_HAVE_HAVEPTHREADSETNAME_NP"
fi

HAVEPTHREADSET_NAME_NP=
if [[ "${ac_cv_pthread_set_name_np_func}" = "yes" ]] ; then
	HAVEPTHREADSET_NAME_NP="#define LIBMAUS_HAVE_HAVEPTHREADSET_NAME_NP"
fi

HAVEPTHREADGETNAME_NP=
if [[ "${ac_cv_pthread_getname_np_func}" = "yes" ]] ; then
	HAVEPTHREADGETNAME_NP="#define LIBMAUS_HAVE_HAVEPTHREADGETNAME_NP"
fi

HAVEPTHREADGET_NAME_NP=
if [[ "${ac_cv_pthread_get_name_np_func}" = "yes" ]] ; then
	HAVEPTHREADGET_NAME_NP="#define LIBMAUS_HAVE_HAVEPTHREADGET_NAME_NP"
fi

HAVEPRCTL=
if [[ "${ac_cv_func_prctl}" = "yes" ]] ; then
	HAVEPRCTL="#define LIBMAUS_HAVE_PRCTL"
fi

HAVEPTHREADSETAFFINITYNP=
if [[ "${ac_cv_func_pthread_setaffinity_np}" = "yes" ]] ; then
	HAVEPTHREADSETAFFINITYNP="#define LIBMAUS_HAVE_PTHREAD_SETAFFINITY_NP"
fi


HAVEPTHREADS=
if test "${pthread}" = "yes" ; then
	LIBMAUSCXXFLAGS="${LIBMAUSCXXFLAGS} -pthread"
	LIBMAUSCFLAGS="${LIBMAUSCFLAGS} -pthread"
	LIBMAUSCPPFLAGS="${LIBMAUSCPPFLAGS}"
	HAVEPTHREADS="#define LIBMAUS_HAVE_PTHREADS"

	if [[ "${have_openmp}" = "yes" ]] ; then
		LIBMAUSPKGCPPFLAGS="${LIBMAUSPKGCPPFLAGS} -fopenmp"
		LIBMAUSPKGLIBS="${LIBMAUSPKGLIBS} -fopenmp"
		LIBMAUSCXXFLAGS="${LIBMAUSCXXFLAGS} -fopenmp"
		LIBMAUSCFLAGS="${LIBMAUSCFLAGS} -fopenmp"
	fi
else
	AC_MSG_NOTICE([Required thread functionality not found, disabling multi threading.])
fi

AC_ARG_WITH([lsf],
	[AS_HELP_STRING([--with-lsf@<:@=PATH@:>@], [path to installed LSF @<:@default=no@:>@])],
	[with_lsf=${withval}],
	[with_lsf=no])

if test "${with_lsf}" != "no" ; then

    if test \( "${with_lsf}" != "no" \) -a \( ! -z "${with_lsf}" \) ; then
	LSFCPPFLAGS="-I${with_lsf}/include -I${with_lsf}/../include"
        LSFLIBDIR="${with_lsf}/lib"
	LSFLDFLAGS="-L${LSFLIBDIR}"
        LSFLIBS="${LSFLIBDIR}/libbat.a ${LSFLIBDIR}/liblsf.a -lm -lnsl -ldl"
        # LSFLIBS="-lbat -llsf -lm -lnsl -ldl -lstdc++"
    else
	LSFCPPFLAGS=
	LSFLDFLAGS=
        LSFLIBDIR=
        LSFLIBS=
    fi

    CPPFLAGS_SAVE="${CPPFLAGS}"
    CPPFLAGS="${CPPFLAGS} ${LSFCPPFLAGS}"
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADER(lsf/lsbatch.h,[have_lsf_lsbatch_h=yes],[have_lsf_lsbatch_h=no])
    AC_LANG_POP
    CPPFLAGS="${CPPFLAGS_SAVE}"

    if test "${have_lsf_lsbatch_h}" = "yes" ; then

        AC_LANG_PUSH([C++])
        CPPFLAGS_SAVE="${CPPFLAGS}"
        CPPFLAGS="${CPPFLAGS} ${LSFCPPFLAGS}"
        LIBS_SAVE="${LIBS}"
        LIBS="${LIBS} ${LSFLIBS}"
        LDFLAGS_SAVE="${LDFLAGS}"
        LDFLAGS="${LDFLAGS} ${LSFLDFLAGS}"
        AC_MSG_CHECKING([whether we can compile a program using LSF])
                        AC_LINK_IFELSE([AC_LANG_SOURCE([#include <lsf/lsbatch.h>
                    #include <cstdlib>

                    int main(int, char *argv[[]]) 
                    {
                            if (lsb_init(argv[[0]]) < 0) 
                            {
                                    char buf[[]] = "lsb_init";
                                    lsb_perror(buf);
                                    exit(-1);
                            }

                            struct submit sm;
                            struct submitReply sr;
                            lsb_submit(&sm,&sr);
                            
                            return 0;
                    }])],
                        have_lsf=yes,
                        have_lsf=no
                )
        AC_MSG_RESULT($have_lsf)
        CPPFLAGS="${CPPFLAGS_SAVE}"
        LIBS="${LIBS_SAVE}"
        LDFLAGS="${LDFLAGS_SAVE}"
        AC_LANG_POP

        if test "${have_lsf}" = "yes" ; then
		LSFCPPFLAGS="-DHAVE_LSF ${LSFCPPFLAGS}"
		LSFLDFLAGS="${LSFLDFLAGS}"
                LSFLIBDIR="${LSFLIBDIR}"
		LSFLIBS="${LIBMAUSLIBS} ${LSFLIBS}"
        else
		LSFCPPFLAGS=""
                LSFLDFLAGS=""
		LSFLIBS=""
                LSFLIBDIR=""
        fi
    fi

fi

AC_ARG_WITH([cuda],
	[AS_HELP_STRING([--with-cuda@<:@=PATH@:>@], [path to installed CUDA tools @<:@default=/usr/local/cuda@:>@])],
	[with_cuda=${withval}],
	[with_cuda=/usr/local/cuda])

CUDADIR=
if test "${with_cuda}" != "no" ; then
	AC_MSG_CHECKING([whether ${with_cuda}/bin/nvcc exists])

	if test -x "${with_cuda}/bin/nvcc" ; then
		CUDANVCC="${with_cuda}/bin/nvcc"
		AC_MSG_RESULT([yes])
	else
		CUDANVCC="no"
		AC_MSG_RESULT([no])
	fi

	if test "${CUDANVCC}" != "no" ; then
		AC_MSG_CHECKING([whether we can compile a program using ${with_cuda}/bin/nvcc])	

		CXXSAVE="${CXX}"
		CXX="${CUDANVCC}"
		CXXFLAGSSAVE="${CXXFLAGS}"
		CXXFLAGS=""
		CPPFLAGSSAVE="${CPPFLAGS}"
		CPPFLAGS=""

	        AC_LANG_PUSH([C++])

		AC_TRY_COMPILE([#include <cuda.h>],[cuInit(0);],[nvcc_compile=yes],[nvcc_compile=no])
		AC_MSG_RESULT([${nvcc_compile}])

		AC_LANG_POP

		CXX="${CXXSAVE}"
		CXXFLAGS="${CXXFLAGSSAVE}"
		CPPFLAGS="${CPPFLAGSSAVE}"

	fi
fi

case $target_cpu in
	i386|i486|i586|i686)
		HAVEI386="#define LIBMAUS_HAVE_i386"
		;;
	x86_64|amd64)
		HAVEI386="#define LIBMAUS_HAVE_i386"
		HAVEX8664="#define LIBMAUS_HAVE_x86_64"
		;;
esac

AC_ARG_ENABLE(syncops,
        AS_HELP_STRING([--enable-syncops],[use atomic operation primitives (default yes)]),
        [syncops=${enableval}],[syncops=yes])

if test "${syncops}" = "yes" ; then
	HAVESYNCOPS=""
	case "${target_cpu}" in
		i386|i486|i586|i686)
			AC_LANG_PUSH([C++])
			AC_MSG_CHECKING([whether the C++ compiler supports synchronous add operations])
			AC_TRY_LINK([],[unsigned long long v = 0; __sync_fetch_and_add(&v,1)],[syncops=yes],[syncops=no])
			AC_MSG_RESULT([${syncops}])
			AC_LANG_POP

			if test "${syncops}" = "yes" ; then
				HAVESYNCOPS="#define LIBMAUS_HAVE_SYNC_OPS"
			fi

			if test "${syncops}" = "no" ; then
				AC_LANG_PUSH([C++])
				CXXFLAGS_SAVE="${CXXFLAGS}"
				CXXFLAGS="-march=i586"
				AC_MSG_CHECKING([whether the C++ compiler supports the -march=i586 option and synchronous add operations])
				AC_TRY_LINK([],[unsigned long long v = 0; __sync_fetch_and_add(&v,1)],[syncops=yes],[syncops=no])
				AC_MSG_RESULT([${syncops}])
				CXXFLAGS="${CXXFLAGS_SAVE}"
				AC_LANG_POP

				if test "${syncops}" = "yes" ; then
					LIBMAUSCXXFLAGS="${LIBMAUSCXXFLAGS} -march=i586"
					LIBMAUSCFLAGS="${LIBMAUSCFLAGS} -march=i586"
					LIBMAUSARCHCFLAGS="${LIBMAUSARCHCFLAGS} -march=i586"
					HAVESYNCOPS="#define LIBMAUS_HAVE_SYNC_OPS"
				fi
			fi
			;;
		x86_64)
			AC_LANG_PUSH([C++])
			AC_MSG_CHECKING([whether the C++ compiler supports synchronous add operations])
			AC_TRY_COMPILE([],[unsigned long long v = 0; __sync_fetch_and_add(&v,1)],[syncops=yes],[syncops=no])
			AC_MSG_RESULT([${syncops}])
			AC_LANG_POP

			if test "${syncops}" = "yes" ; then
				HAVESYNCOPS="#define LIBMAUS_HAVE_SYNC_OPS"
			fi
			;;
		*)
			;;
	esac
fi

AC_ARG_ENABLE(synclock,
        AS_HELP_STRING([--enable-synclock],[use atomic lock primitives (default yes)]),
        [synclock=${enableval}],[synclock=yes])

if test "${synclock}" = "yes" ; then
	LIBMAUS_HAVE_SYNC_LOCK=""
	case "${target_cpu}" in
		i386|i486|i586|i686)
			AC_LANG_PUSH([C++])
			AC_MSG_CHECKING([whether the C++ compiler supports synchronous lock operations])
			AC_TRY_LINK([],[unsigned int v = 0; __sync_lock_test_and_set(&v,1)],[synclock=yes],[synclock=no])
			AC_MSG_RESULT([${synclock}])
			AC_LANG_POP

			if test "${synclock}" = "yes" ; then
				HAVESYNCLOCK="#define LIBMAUS_HAVE_SYNC_LOCK"
			fi

			if test "${synclock}" = "no" ; then
				AC_LANG_PUSH([C++])
				CXXFLAGS_SAVE="${CXXFLAGS}"
				CXXFLAGS="-march=i586"
				AC_MSG_CHECKING([whether the C++ compiler supports the -march=i586 option and synchronous lock operations])
				AC_TRY_LINK([],[unsigned int v = 0; __sync_lock_test_and_set(&v,1)],[synclock=yes],[synclock=no])
				AC_MSG_RESULT([${synclock}])
				CXXFLAGS="${CXXFLAGS_SAVE}"
				AC_LANG_POP

				if test "${synclock}" = "yes" ; then
					LIBMAUSCXXFLAGS="${LIBMAUSCXXFLAGS} -march=i586"
					LIBMAUSCFLAGS="${LIBMAUSCFLAGS} -march=i586"
					LIBMAUSARCHCFLAGS="${LIBMAUSARCHCFLAGS} -march=i586"
					HAVESYNCLOCK="#define LIBMAUS_HAVE_SYNC_LOCK"
				fi
			fi
			;;
		x86_64)
			AC_LANG_PUSH([C++])
			AC_MSG_CHECKING([whether the C++ compiler supports synchronous lock operations])
			AC_TRY_COMPILE([],[unsigned int v = 0; __sync_lock_test_and_set(&v,1)],[synclock=yes],[synclock=no])
			AC_MSG_RESULT([${synclock}])
			AC_LANG_POP

			if test "${synclock}" = "yes" ; then
				HAVESYNCLOCK="#define LIBMAUS_HAVE_SYNC_LOCK"
			fi
			;;
		*)
			;;
	esac
fi

AC_LANG_PUSH([C++])
AC_MSG_CHECKING([whether posix_memalign is present])
AC_TRY_COMPILE([#include <cstdlib>],[void * ptr = 0; posix_memalign(&ptr,64,128);],[posixmemalign=yes],[posixmemalign=no])
AC_MSG_RESULT([${posixmemalign}])
AC_LANG_POP

AC_LANG_PUSH([C++])
AC_MSG_CHECKING([whether getpagesize is present])
AC_TRY_COMPILE([#include <unistd.h>],[unsigned int pagesize = getpagesize();],[getpagesize=yes],[getpagesize=no])
AC_MSG_RESULT([${getpagesize}])
AC_LANG_POP

HAVEPOSIXMEMALIGN=""
if test "${posixmemalign}" = "yes" ; then
	HAVEPOSIXMEMALIGN="#define LIBMAUS_HAVE_POSIX_MEMALIGN"
fi

HAVEGETPAGESIZE=""
if test "${getpagesize}" = "yes" ; then
	HAVEGETPAGESIZE="#define LIBMAUS_HAVE_GETPAGESIZE"
fi

IOLIBCPPFLAGS=
IOLIBLDFLAGS=
IOLIBLIBS=
IOLIBDEFINE=
AC_ARG_WITH([io_lib],
            [AS_HELP_STRING([--with-io_lib@<:@=PATH@:>@], [support for io_lib library @<:@default=no@:>@])],
            [with_io_lib=${withval}],
            [with_io_lib=no])

if [[ "${have_dynamic_loading}" = "yes" -a "${getpagesize}" = "yes" -a "${posixmemalign}" = "yes" -a "${with_io_lib}" != "no" -a "${sharedlibmaus}" != "no" ]] ; then
	if [[ \( ! -z "${with_io_lib}" \) -a \( "${with_io_lib}" != "yes" \) ]] ; then
		IOLIBCPPFLAGS="-I${with_io_lib}/include"
		IOLIBLDFLAGS="-L${with_io_lib}/lib"
	fi

	IOLIBLIBS="-lstaden-read"

	CPPFLAGSSAVE="${CPPFLAGS}"
	LDFLAGSSAVE="${LDFLAGS}"
	LIBSSAVE="${LIBS}"
	CPPFLAGS="${CPPFLAGS} ${IOLIBCPPFLAGS}"
	LDFLAGS="${LDFLAGS} ${IOLIBLDFLAGS}"
	LIBS="${LIBS} ${IOLIBLIBS} ${ZLIBLIBS} -lm"

	AC_LANG_PUSH([C])
	AC_CHECK_HEADER(io_lib/scram.h, [io_lib_scram_h=yes], [io_lib_scram_h=no])
	AC_CHECK_FUNC(scram_open,scram_open=yes,scram_open=no)
	AC_CHECK_FUNC(scram_close,scram_close=yes,scram_close=no)
	AC_CHECK_FUNC(scram_get_header,scram_get_header=yes,scram_get_header=no)
	AC_CHECK_FUNC(scram_get_seq,scram_get_seq=yes,scram_get_seq=no)
	AC_CHECK_FUNC(scram_set_option,scram_set_option=yes,scram_set_option=no)
	AC_LANG_POP

	if [[ "${io_lib_scram_h}" != "yes" ]] ; then
		AC_MSG_ERROR([--with-io_lib is set to ${with_io_lib}, but io_lib headers are not available])
	fi
	if [[ "${scram_open}" != "yes" ]] ; then
		AC_MSG_ERROR([--with-io_lib is set to ${with_io_lib}, but io_lib library (libstaden-read) is not available])
	fi
	if [[ "${scram_close}" != "yes" ]] ; then
		AC_MSG_ERROR([--with-io_lib is set to ${with_io_lib}, but io_lib library (libstaden-read) is not available])
	fi
	if [[ "${scram_get_header}" != "yes" ]] ; then
		AC_MSG_ERROR([--with-io_lib is set to ${with_io_lib}, but io_lib library (libstaden-read) is not available])
	fi
	if [[ "${scram_get_seq}" != "yes" ]] ; then
		AC_MSG_ERROR([--with-io_lib is set to ${with_io_lib}, but io_lib library (libstaden-read) is not available])
	fi
	if [[ "${scram_set_option}" != "yes" ]] ; then
		AC_MSG_ERROR([--with-io_lib is set to ${with_io_lib}, but io_lib library (libstaden-read) is not available])
	fi

        AC_LANG_PUSH([C])
        AC_MSG_CHECKING([whether we can compile a program using io_lib])
                        AC_LINK_IFELSE([AC_LANG_SOURCE([#include <io_lib/scram.h>
                    #include <stdlib.h>

                    int main() 
                    {
			scram_fd * cfd = scram_open("filename","C");
			SAM_hdr * header = 0;
			bam_seq_t * s = 0;

			if ( ! cfd )
				return -1;

			if ( ! (header = scram_get_header(cfd)) )
			{
				scram_close(cfd);
				return -1;
			}

			while (scram_get_seq(cfd, &s) >= 0) 
			{
			}

			if ( ! scram_eof(cfd) )
			{
				scram_close(cfd);
				return -1;
			}

			scram_close(cfd);
			return 0;
                    }])],
                        have_io_lib=yes,
                        have_io_lib=no
                )
        AC_MSG_RESULT($have_io_lib)
        AC_LANG_POP

	CPPFLAGS="${CPPFLAGSSAVE}"
	LDFLAGS="${LDFLAGSSAVE}"
	LIBS="${LIBSSAVE}"

	IOLIBDEFINE="#define LIBMAUS_HAVE_IO_LIB"
fi

AC_MSG_CHECKING([whether MSG_CMSG_CLOEXEC is defined])	

AC_LANG_PUSH([C])

AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>],[const int flag = MSG_CMSG_CLOEXEC;],[flag_MSG_CMSG_CLOEXEC=yes],[flag__MSG_CMSG_CLOEXEC=no])
AC_MSG_RESULT([${flag_MSG_CMSG_CLOEXEC}])

AC_LANG_POP

if [[ "${flag_MSG_CMSG_CLOEXEC}" = "yes" ]] ; then
	HAVEMSGCMSGCLOEXEC="#define LIBMAUS_HAVE_MSG_CMSG_CLOEXEC"	
fi

AC_LANG_PUSH([C++])
AC_CHECK_HEADER(lzma.h, [have_lzma_h=yes], [have_lzma_h=no])
AC_LANG_POP

if [[ "${have_lzma_h}" = "yes" ]] ; then
	LIBS_SAVE="${LIBS}"
	LIBS="${LIBS} -llzma"

	AC_MSG_CHECKING([whether we can compile a program using the lzma library])	
	AC_LANG_PUSH([C++])
        AC_TRY_LINK([#include <lzma.h>
#include <cstring>
],[
        /* init LZMA decoder */
        lzma_stream lstr = LZMA_STREAM_INIT;

        #if LZMA_VERSION <= UINT32_C(49990030)
        int ret = lzma_auto_decoder(&lstr, NULL, NULL);
        #else
        int ret = lzma_auto_decoder(&lstr, UINT64_MAX, 0);
        #endif

	lzma_end(&lstr);
	],[have_lzma=yes],[have_lzma=no])
        AC_MSG_RESULT([${have_lzma}])
	LIBS="${LIBSSAVE}"
	CPPFLAGS="${CPPFLAGSSAVE}"
        AC_LANG_POP
	
	LIBS="${LIBS_SAVE}"
fi

AC_ARG_WITH([libcurl],
	[AS_HELP_STRING([--with-libcurl@<:@=PATH@:>@], [path to installed libcurl library @<:@default=no@:>@])],
	[with_libcurl=${withval}],
	[with_libcurl=no])

LIBCURL=
if test "${with_libcurl}" != "no" ; then

	if test ! -z "${with_libcurl}" ; then
		PKGCONFIGPATHSAVE="${PKG_CONFIG_PATH}"
		if test -z "${PKG_CONFIG_PATH}" ; then
			export PKG_CONFIG_PATH="${with_libcurl}/lib/pkgconfig"
		else
			export PKG_CONFIG_PATH="${with_libcurl}/lib/pkgconfig:${PKG_CONFIG_PATH}"
		fi
	fi

	PKG_CHECK_MODULES([libcurl],[libcurl >= 1.7.2],[libcurlpkg=yes],[libcurlpkg=no])

	if test ! -z "${with_libcurl}" ; then
		if test ! -z "${PKGCONFIGPATHSAVE}" ; then
			export PKG_CONFIG_PATH="${PKGCONFIGPATHSAVE}"
		fi
	fi

	LIBCURL=
	if [[ "${libcurlpkg}" = "yes" ]] ; then
		AC_LANG_PUSH([C++])
		LIBSSAVE="${LIBS}"
		LIBS="${LIBS} ${libcurl_LIBS}"
		CPPFLAGSSAVE="${CPPFLAGS}"
		CPPFLAGS="${CPPFLAGS} ${libcurl_CFLAGS}"
		AC_MSG_CHECKING([whether we can compile a libcurl program])
		AC_TRY_LINK([#include <curl/curl.h>
	],[curl_global_init(CURL_GLOBAL_ALL);],[libcurl=yes],[libcurl=no])
		AC_MSG_RESULT([${libcurl}])
		LIBS="${LIBSSAVE}"
		CPPFLAGS="${CPPFLAGSSAVE}"
		AC_LANG_POP

		if [[ "$libcurl" = "no" ]] ; then
			AC_MSG_ERROR([Failed to compile libcurl program.]);
		else
			LIBCURL="#define LIBMAUS_HAVE_LIBCURL"
			LIBCURLLIBS="${libcurl_LIBS}"
			LIBCURLCPPFLAGS="${libcurl_CFLAGS}"
		fi
	fi

fi

AC_ARG_WITH([openssl],
	[AS_HELP_STRING([--with-openssl@<:@=PATH@:>@], [path to installed openssl library @<:@default=no@:>@])],
	[with_openssl=${withval}],
	[with_openssl=no])

OPENSSL=
if test "${with_openssl}" != "no" ; then

	if test ! -z "${with_openssl}" ; then
		PKGCONFIGPATHSAVE="${PKG_CONFIG_PATH}"
		if test -z "${PKG_CONFIG_PATH}" ; then
			export PKG_CONFIG_PATH="${with_openssl}/lib/pkgconfig"
		else
			export PKG_CONFIG_PATH="${with_openssl}/lib/pkgconfig:${PKG_CONFIG_PATH}"
		fi
	fi

	PKG_CHECK_MODULES([openssl],[openssl >= 1.0.1],[opensslpkg=yes],[opensslpkg=no])

	if test ! -z "${with_openssl}" ; then
		if test ! -z "${PKGCONFIGPATHSAVE}" ; then
			export PKG_CONFIG_PATH="${PKGCONFIGPATHSAVE}"
		fi
	fi

	OPENSSL=
	if [[ "${opensslpkg}" = "yes" ]] ; then
		AC_LANG_PUSH([C++])
		LIBSSAVE="${LIBS}"
		LIBS="${LIBS} ${openssl_LIBS}"
		CPPFLAGSSAVE="${CPPFLAGS}"
		CPPFLAGS="${CPPFLAGS} ${openssl_CFLAGS}"
		AC_MSG_CHECKING([whether we can compile a openssl program])
		AC_TRY_LINK([#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>],[SSL_library_init();
SSL_load_error_strings();
ERR_load_BIO_strings();
],[openssl=yes],[openssl=no])
		AC_MSG_RESULT([${openssl}])
		LIBS="${LIBSSAVE}"
		CPPFLAGS="${CPPFLAGSSAVE}"
		AC_LANG_POP

		if [[ "$openssl" = "no" ]] ; then
			AC_MSG_ERROR([Failed to compile openssl program.]);
		else
			OPENSSL="#define LIBMAUS_HAVE_OPENSSL"
			OPENSSLLIBS="${openssl_LIBS}"
			OPENSSLCPPFLAGS="${openssl_CFLAGS}"
		fi
	fi
fi

AC_ARG_WITH([gnutls],
	[AS_HELP_STRING([--with-gnutls@<:@=PATH@:>@], [path to installed gnutls library @<:@default=no@:>@])],
	[with_gnutls=${withval}],
	[with_gnutls=no])

GNUTLS=
GNUTLSREQ=
if test "${with_gnutls}" != "no" ; then

	if test ! -z "${with_gnutls}" ; then
		PKGCONFIGPATHSAVE="${PKG_CONFIG_PATH}"
		if test -z "${PKG_CONFIG_PATH}" ; then
			export PKG_CONFIG_PATH="${with_gnutls}/lib/pkgconfig"
		else
			export PKG_CONFIG_PATH="${with_gnutls}/lib/pkgconfig:${PKG_CONFIG_PATH}"
		fi
	fi

	PKG_CHECK_MODULES([gnutls],[gnutls >= 2.12.14],[gnutlspkg=yes],[gnutlspkg=no])

	if test ! -z "${with_gnutls}" ; then
		if test ! -z "${PKGCONFIGPATHSAVE}" ; then
			export PKG_CONFIG_PATH="${PKGCONFIGPATHSAVE}"
		fi
	fi

	GNUTLS=
	if [[ "${gnutlspkg}" = "yes" ]] ; then
		AC_LANG_PUSH([C++])
		LIBSSAVE="${LIBS}"
		LIBS="${LIBS} ${gnutls_LIBS}"
		CPPFLAGSSAVE="${CPPFLAGS}"
		
		CPPFLAGS="${CPPFLAGS} ${gnutls_CFLAGS}"
		AC_MSG_CHECKING([whether we can compile a gnutls program])
		AC_TRY_LINK([#include <gnutls/gnutls.h>
#include <gnutls/x509.h>		
	],[;],[gnutls=yes],[gnutls=no])
		AC_MSG_RESULT([${gnutls}])
		LIBS="${LIBSSAVE}"
		CPPFLAGS="${CPPFLAGSSAVE}"
		
		AC_LANG_POP

		if [[ "$gnutls" = "no" ]] ; then
			AC_MSG_ERROR([Failed to compile gnutls program.]);
		else
			GNUTLS="#define LIBMAUS_HAVE_GNUTLS"
			GNUTLSLDFLAGS=""
			GNUTLSLIBS="${gnutls_LIBS}"
			GNUTLSCPPFLAGS="${gnutls_CFLAGS}"
			GNUTLSREQ="gnutls"
		fi
	else
		AC_MSG_ERROR([GNU TLS not found in required version])
	fi

fi


AC_LANG_PUSH([C++])
AC_CHECK_HEADER(libkern/OSAtomic.h, [libkern_osatomic_h=yes], [libkern_osatomic_h=no])
AC_LANG_POP

HAVEDARWINSPINLOCKS=
if test "${libkern_osatomic_h}" = "yes" ; then
	AC_MSG_CHECKING([for Darwin type spin locks])
	AC_LANG_PUSH([C++])
	AC_TRY_LINK([#include <libkern/OSAtomic.h>],[
	OSSpinLock lock = OS_SPINLOCK_INIT;

	OSSpinLockLock(&lock);
	OSSpinLockUnlock(&lock);
	
	if ( OSSpinLockTry(&lock) )
		OSSpinLockUnlock(&lock);

	return 0;
],[have_darwin_spinlocks=yes],[have_darwin_spinlocks=no])
		AC_MSG_RESULT([${have_darwin_spinlocks}])
		AC_LANG_POP

		if test "$have_darwin_spinlocks" = "yes" ; then
			HAVEDARWINSPINLOCKS="#define LIBMAUS_HAVE_DARWIN_SPINLOCKS"
		fi
fi

AC_MSG_CHECKING([for unsigned __int128])
AC_LANG_PUSH([C++])
	AC_TRY_LINK([#include <iostream>],[
	unsigned __int128 i = 5;
	std::cout << static_cast<int>(i) << std::endl;
	return 0;
],[have_unsigned_int128=yes],[have_unsigned_int128=no])
AC_MSG_RESULT([${have_unsigned_int128}])
AC_LANG_POP

if test "$have_unsigned_int128" = "yes" ; then
	HAVEUNSIGNEDINT128="#define LIBMAUS_HAVE_UNSIGNED_INT128"
fi

case $host_os in
  *mingw32*)
    LIBMAUSLDFLAGS="${LIBMAUSLDFLAGS} -no-undefined" # -avoid-version
    ;;
  *)
    LIBMAUSLDFLAGS="${LIBMAUSLDFLAGS}"
    ;;
esac

case $host_os in
  freebsd*)
    LIBMAUSLIBS="${LIBMAUSLIBS} -lkvm"
    LIBMAUSPKGLIBS="${LIBMAUSPKGLIBS} -lkvm"
    ;;
  *)
    LIBMAUSLIBS="${LIBMAUSLIBS}"
    LIBMAUSPKGLIBS="${LIBMAUSPKGLIBS}"
    ;;
esac

CPPFLAGS="${CPPFLAGS} ${LIBMAUSCPPFLAGS}"
CFLAGS="${CFLAGS} ${LIBMAUSCFLAGS}"
CXXFLAGS="${CXXFLAGS} ${LIBMAUSCXXFLAGS}"
LIBS="${LIBS} ${LIBMAUSLIBS}"

PACKAGE_NAME=${PACKAGE}
PACKAGE_VERSION=${VERSION}

AC_SUBST([PACKAGE_NAME])
AC_SUBST([PACKAGE_VERSION])
AC_SUBST([SYS_TYPES_H])
AC_SUBST([UNIQUEPTR])
AC_SUBST([SHAREDPTR])
AC_SUBST([UNORDEREDSET])
AC_SUBST([UNORDEREDMAP])
AC_SUBST([CSTDINT])
AC_SUBST([ENDIANESS])
AC_SUBST([HAVESSE4])
AC_SUBST([HAVEAIO])
AC_SUBST([HAVESEMINIT])
AC_SUBST([HAVESEMDESTROY])
AC_SUBST([HAVESEMWAIT])
AC_SUBST([HAVESEMPOST])
AC_SUBST([HAVESEMTIMEDWAIT])
AC_SUBST([HAVEPTHREADS])
AC_SUBST([HAVESYNCOPS])
AC_SUBST([HAVEPOSIXMEMALIGN])
AC_SUBST([HAVELSF])
AC_SUBST([MOVEFUNC])
AC_SUBST([HAVEI386])
AC_SUBST([HAVEX8664])
AC_SUBST([HAVESSSE3])
AC_SUBST([WINDOWSH])
AC_SUBST([SYSTIMEH])
AC_SUBST([LIBMAUSCPPFLAGS])
AC_SUBST([LIBMAUSCXXFLAGS])
AC_SUBST([LIBMAUSLDFLAGS])
AC_SUBST([LIBMAUSLIBS])
AC_SUBST([LIBMAUSLIBLSFSRC])
AC_SUBST([LIBMAUSARCHCFLAGS])
AC_SUBST([LSFCPPFLAGS])
AC_SUBST([LSFCXXFLAGS])
AC_SUBST([LSFLIBS])
AC_SUBST([LSFLDFLAGS])
AC_SUBST([LSFLIBDIR])
AC_SUBST([COMPUTEHASHFREQUENCIESLSF])
AC_SUBST([COMPUTERELATIONMATRIXLSF])
AC_SUBST([FILTERRELATIONMATRIX])
AC_SUBST([LIBMAUSPKGCPPFLAGS])
AC_SUBST([LIBMAUSPKGCXXFLAGS])
AC_SUBST([LIBMAUSPKGLIBS])
AC_SUBST([LIBRARY_VERSION])
AC_SUBST([LIBMAUS_HAVE_EXECINFO_H])
AC_SUBST([HAVEBACKTRACE])
AC_SUBST([HAVEWINDOWSH])
AC_SUBST([HAVESETRLIMIT])
AC_SUBST([ZLIBREQ])
AC_SUBST([SDSLCPPFLAGS])
AC_SUBST([SDSLCXXFLAGS])
AC_SUBST([SDSLLIBS])
AC_SUBST([USEASSEMBLY])
AC_SUBST([HAVEUNISTDH])
AC_SUBST([HAVESTDINTH])
AC_SUBST([REGEXH])
AC_SUBST([SEQAN])
AC_SUBST([KMLOCAL])
AC_SUBST([LIBMAUSHAVEBOOSTREGEX])
AC_SUBST([LIBMAUSHAVESNAPPY])
AC_SUBST([HAVEDLFCNH])
AC_SUBST([abs_builddir])
AC_SUBST([DLFUNCS])
AC_SUBST([DLLIB])
AC_SUBST([KMLOCALLDFLAGS])
AC_SUBST([KMLOCALLIBS])
AC_SUBST([KMLOCALCPPFLAGS])
AC_SUBST([KMLOCALCXXFLAGS])
AC_SUBST([SNAPPYCPPFLAGS])
AC_SUBST([SNAPPYCXXFLAGS])
AC_SUBST([SNAPPYLDFLAGS])
AC_SUBST([SNAPPYLIBS])
AC_SUBST([SEQANCPPFLAGS])
AC_SUBST([SEQANCXXFLAGS])
AC_SUBST([SEQANLDFLAGS])
AC_SUBST([SEQANLIBS])
AC_SUBST([ZLIBCPPFLAGS])
AC_SUBST([ZLIBCXXFLAGS])
AC_SUBST([ZLIBLIBS])
AC_SUBST([LIBMAUSSTATIC])
AC_SUBST([HAVEPTHREADSETNAME_NP])
AC_SUBST([HAVEPTHREADSET_NAME_NP])
AC_SUBST([HAVEPTHREADGETNAME_NP])
AC_SUBST([HAVEPTHREADGET_NAME_NP])
AC_SUBST([HAVEPRCTL])
AC_SUBST([LIBMAUS_SIZEOF_UNSIGNED_LONG])
AC_SUBST([IOLIBDEFINE])
AC_SUBST([IOLIBCPPFLAGS])
AC_SUBST([IOLIBLDFLAGS])
AC_SUBST([IOLIBLIBS])
AC_SUBST([HAVEGETPAGESIZE])
AC_SUBST([HAVEMSGCMSGCLOEXEC])
AC_SUBST([WARNCPPFLAGS])
AC_SUBST([LIBCURL])
AC_SUBST([LIBCURLCPPFLAGS])
AC_SUBST([LIBCURLLIBS])
AC_SUBST([OPENSSL])
AC_SUBST([OPENSSLCPPFLAGS])
AC_SUBST([OPENSSLLIBS])
AC_SUBST([POSIXSPINLOCKS])
AC_SUBST([HAVESYNCLOCK])
AC_SUBST([HAVEDARWINSPINLOCKS])
AC_SUBST([HAVESYSVFSH])
AC_SUBST([LIBMAUS_HAVE_STATFS])
AC_SUBST([HAVEUNSIGNEDINT128])
AC_SUBST([HAVEHAVEFEATURESH])
AC_SUBST([HAVEPTHREADSETAFFINITYNP])
AC_SUBST([LIBMAUS_HAVE_IGZIP])
AC_SUBST([IGZIPCPPFLAGS])
AC_SUBST([IGZIPLDFLAGS])
AC_SUBST([IGZIPLIBS])
AC_SUBST([GNUTLSLIBS])
AC_SUBST([GNUTLSCPPFLAGS])
AC_SUBST([GNUTLSLDFLAGS])
AC_SUBST([GNUTLS])
AC_SUBST([GNUTLSREQ])
AC_OUTPUT(Makefile src/Makefile libmaus.pc libmauslsf.pc ubuntu.sh src/libmaus/LibMausConfig.hpp)
