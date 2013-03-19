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
#if !defined(LIBMAUS_UTIL_DYNAMICLOADING_HPP)
#define LIBMAUS_UTIL_DYNAMICLOADING_HPP

#include <libmaus/LibMausConfig.hpp>
#if defined(LIBMAUS_HAVE_DLFCN_H)
#include <dlfcn.h>
#endif
#include <string>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>

namespace libmaus
{
	namespace util
	{
		#if defined(LIBMAUS_HAVE_DL_FUNCS)
		struct DynamicLibrary
		{
			typedef DynamicLibrary this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			std::string const modname;
			void * lib;
			
			DynamicLibrary(std::string const & rmodname)
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
			~DynamicLibrary()
			{
				dlclose(lib);
			}
		};
		
		template<typename _func_type>
		struct DynamicLibraryFunction
		{
			typedef _func_type func_type;
			typedef DynamicLibraryFunction<func_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			DynamicLibrary::unique_ptr_type plib;
			DynamicLibrary & lib;
			
			func_type func;
			
			void init(std::string const & funcname)
			{
				void * vfunc = dlsym(lib.lib,funcname.c_str());
				
				if ( ! vfunc )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to dlsym(\""<<lib.modname<<"\",\"" << funcname << "\"): " << dlerror() << std::endl;
					se.finish();
					throw se;
				}
				
				func = reinterpret_cast<func_type>(vfunc);				
			}
			
			DynamicLibraryFunction(DynamicLibrary & rlib, std::string const & funcname)
			: lib(rlib), func(0)
			{
				init(funcname);
			}
			DynamicLibraryFunction(std::string const & modname, std::string const & funcname)
			: plib(new DynamicLibrary(modname)), lib(*plib), func(0)
			{
				init(funcname);
			}
			~DynamicLibraryFunction()
			{
				
			}
		};
		#endif
	
		struct DynamicLoading
		{
			#if defined(LIBMAUS_HAVE_DL_FUNCS)
			static int callFunction(std::string const & modname, std::string const & funcname, int const arg, std::string const & argstr)
			{
				typedef int (*func_t)(int, char const *, size_t);
				DynamicLibraryFunction<func_t> DLF(modname,funcname);				
				return DLF.func(arg,argstr.c_str(),argstr.size());					
			}
			#else
			static int callFunction(std::string const & modname, std::string const & funcname, int const arg, std::string const & argstr)
			{
				::libmaus::exception::LibMausException se;
				se.getStream() << "Cannot call function \""<<funcname<<"\" in module \""<<modname<<"\": dynamic loading is not supported." << std::endl;
				se.finish();
				throw se;
			}
			#endif
		};
	}	
}
#endif
