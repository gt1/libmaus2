/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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

#if ! defined(LIBMAUS_HDF5_HDF5MUTEERRORHANDLER_HPP)
#define LIBMAUS_HDF5_HDF5MUTEERRORHANDLER_HPP

#include <libmaus/LibMausConfig.hpp>

#if defined(LIBMAUS_HAVE_HDF5)
#include <hdf5.h>
#include <hdf5_hl.h>
#include <iostream>

namespace libmaus
{
	namespace hdf5
	{
		struct HDF5MuteErrorHandler
		{
			hid_t error_stack;
			H5E_auto2_t old_func;
			void *old_client_data;
		
			HDF5MuteErrorHandler()
			{
				error_stack = H5E_DEFAULT;
				H5Eget_auto2(error_stack, &old_func, &old_client_data);
				H5Eset_auto2(error_stack, NULL, NULL);
			}
			
			~HDF5MuteErrorHandler()
			{
				H5Eset_auto2(error_stack, old_func, old_client_data);
			}
		};
	}
}
#endif

#endif
