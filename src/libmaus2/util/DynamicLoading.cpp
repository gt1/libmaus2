/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <libmaus2/util/DynamicLoading.hpp>

#if defined(LIBMAUS_HAVE_DL_FUNCS)
extern "C" {
	void libmaus2_dlopen_dummy() 
	{
	
	}
}

#include <libgen.h>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <config.h>

static std::string getLibraryPath()
{
	Dl_info libinfo;
	int const r = dladdr(reinterpret_cast<void*>(libmaus2_dlopen_dummy), &libinfo);
		
	if ( ! r )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "dladdr failed: " << dlerror() << std::endl;
		se.finish();
		throw se;	
	}
	
	libmaus2::autoarray::AutoArray<char> D(strlen(libinfo.dli_fname)+1,true);
	std::copy(libinfo.dli_fname,libinfo.dli_fname + strlen(libinfo.dli_fname),D.begin());
		
	char * dn = dirname(D.begin());

	return std::string(dn);
}

#if 0
static std::string getLibraryName()
{
	Dl_info libinfo;
	int const r = dladdr(reinterpret_cast<void*>(libmaus2_dlopen_dummy), &libinfo);
		
	if ( ! r )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "dladdr failed: " << dlerror() << std::endl;
		se.finish();
		throw se;	
	}
	
	libmaus2::autoarray::AutoArray<char> D(strlen(libinfo.dli_fname)+1,true);
	std::copy(libinfo.dli_fname,libinfo.dli_fname + strlen(libinfo.dli_fname),D.begin());
		
	char * bn = basename(D.begin());

	return std::string(bn);
}
#endif

libmaus2::util::DynamicLibrary::DynamicLibrary(std::string const & rmodname)
: modname(rmodname), lib(0)
{
	// try without subdirectory (uninstalled module)
	#if defined(__linux__)
	lib = dlopen(modname.c_str(),RTLD_LAZY | RTLD_NODELETE);
	#else
	lib = dlopen(modname.c_str(),RTLD_LAZY);	
	#endif
	
	// if not found then try with directory (installed module)
	if ( ! lib )
	{	
		std::string const instmodname = getLibraryPath() + std::string("/") + std::string(PACKAGE_NAME)+std::string("/")+std::string(PACKAGE_VERSION)+std::string("/")+modname;
		#if defined(__linux__)
		lib = dlopen(instmodname.c_str(),RTLD_LAZY | RTLD_NODELETE);
		#else
		lib = dlopen(instmodname.c_str(),RTLD_LAZY);		
		#endif
	}
	
	if ( ! lib )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to dlopen(\"" << modname << "\",RTLD_LAZY): " << dlerror() << std::endl;
		se.finish();
		throw se;
	}
}
libmaus2::util::DynamicLibrary::~DynamicLibrary()
{
	dlclose(lib);
}
#endif
	
#if defined(LIBMAUS_HAVE_DL_FUNCS)
int libmaus2::util::DynamicLoading::callFunction(std::string const & modname, std::string const & funcname, int const arg, std::string const & argstr)
{
	typedef int (*func_t)(int, char const *, size_t);
	DynamicLibraryFunction<func_t> DLF(modname,funcname);				
	return DLF.func(arg,argstr.c_str(),argstr.size());					
}
#else
int libmaus2::util::DynamicLoading::callFunction(std::string const & modname, std::string const & funcname, int const arg, std::string const & argstr)
{
	::libmaus2::exception::LibMausException se;
	se.getStream() << "Cannot call function \""<<funcname<<"\" in module \""<<modname<<"\": dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
}
#endif
