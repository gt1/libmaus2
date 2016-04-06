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
#if !defined(LIBMAUS2_UTIL_DYNAMICLOADING_HPP)
#define LIBMAUS2_UTIL_DYNAMICLOADING_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_DLFCN_H)
#include <dlfcn.h>
#endif
#include <string>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>

namespace libmaus2
{
	namespace util
	{
		#if defined(LIBMAUS2_HAVE_DL_FUNCS)
		/**
		 * class encapsulating a dynamic library
		 **/
		struct DynamicLibrary
		{
			//! this type
			typedef DynamicLibrary this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			//! name of modules
			std::string const modname;
			//! system dl handle
			void * lib;

			/**
			 * constructor by module name; throws an exception if module is not available/cannot be loaded
			 *
			 * @param rmodname name of module to be opened
			 **/
			DynamicLibrary(std::string const & rmodname, int addflags = 0);
			/**
			 * destructor, unloads module
			 **/
			~DynamicLibrary();
		};

		/**
		 * class encapsulating a function in a dynamically loaded library
		 **/
		template<typename _func_type>
		struct DynamicLibraryFunction
		{
			//! signature type of function
			typedef _func_type func_type;
			//! this type
			typedef DynamicLibraryFunction<func_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			//! pointer to library
			DynamicLibrary::unique_ptr_type plib;
			//! reference to library
			DynamicLibrary & lib;

			//! function pointer
			func_type func;

			/**
			 * load function named funcname; throws an exception if function cannot be found
			 *
			 * @param funcname name of function
			 **/
			void init(std::string const & funcname)
			{
				void * vfunc = dlsym(lib.lib,funcname.c_str());

				if ( ! vfunc )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to dlsym(\""<<lib.modname<<"\",\"" << funcname << "\"): " << dlerror() << std::endl;
					se.finish();
					throw se;
				}

				func = reinterpret_cast<func_type>(vfunc);
			}

			/**
			 * constructor by library object and function name
			 *
			 * @param rlib library object
			 * @param funcname function name
			 **/
			DynamicLibraryFunction(DynamicLibrary & rlib, std::string const & funcname)
			: lib(rlib), func(0)
			{
				init(funcname);
			}
			/**
			 * constructor by module name and function name
			 *
			 * @param modname module name
			 * @param funcname function name
			 **/
			DynamicLibraryFunction(std::string const & modname, std::string const & funcname)
			: plib(new DynamicLibrary(modname)), lib(*plib), func(0)
			{
				init(funcname);
			}
			/**
			 * destructor, unload function
			 **/
			~DynamicLibraryFunction()
			{

			}
		};
		#endif

		/**
		 * class for calling a function taking and int and a string including string length as parameters
		 **/
		struct DynamicLoading
		{
			/**
			 * load and call function funcname from library modname with arguments arg, argstr.c_str(), argstr.size()
			 *
			 * @param modname module name
			 * @param funcname function name
			 * @param arg first argument for function
			 * @param argstr string for constructing second and third argument of function
			 * @return return code of the function
			 **/
			#if defined(LIBMAUS2_HAVE_DL_FUNCS)
			static int callFunction(std::string const & modname, std::string const & funcname, int const arg, std::string const & argstr);
			#else
			static int callFunction(std::string const & modname, std::string const & funcname, int const arg, std::string const & argstr);
			#endif
		};
	}
}
#endif
