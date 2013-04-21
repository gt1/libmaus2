/**
    libmaus
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
**/
#include <libmaus/util/DynamicLoading.hpp>

#if defined(LIBMAUS_HAVE_DL_FUNCS)
libmaus::util::DynamicLibrary::DynamicLibrary(std::string const & rmodname)
: modname(rmodname), lib(0)
{
	lib = dlopen(modname.c_str(),RTLD_LAZY);

	if ( ! lib )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "Failed to dlopen(\"" << modname << "\",RTLD_LAZY): " << dlerror() << std::endl;
		se.finish();
		throw se;
	}
}
libmaus::util::DynamicLibrary::~DynamicLibrary()
{
	dlclose(lib);
}
#endif
	
#if defined(LIBMAUS_HAVE_DL_FUNCS)
int libmaus::util::DynamicLoading::callFunction(std::string const & modname, std::string const & funcname, int const arg, std::string const & argstr)
{
	typedef int (*func_t)(int, char const *, size_t);
	DynamicLibraryFunction<func_t> DLF(modname,funcname);				
	return DLF.func(arg,argstr.c_str(),argstr.size());					
}
#else
int libmaus::util::DynamicLoading::callFunction(std::string const & modname, std::string const & funcname, int const arg, std::string const & argstr)
{
	::libmaus::exception::LibMausException se;
	se.getStream() << "Cannot call function \""<<funcname<<"\" in module \""<<modname<<"\": dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
}
#endif
