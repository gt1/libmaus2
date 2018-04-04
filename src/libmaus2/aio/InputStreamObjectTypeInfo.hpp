/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if !defined(LIBMAUS2_AIO_INPUTSTREAMOBJECTTYPEINFO_HPP)
#define LIBMAUS2_AIO_INPUTSTREAMOBJECTTYPEINFO_HPP

#include <libmaus2/aio/InputStreamObject.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct InputStreamObjectTypeInfo
		{
			typedef InputStreamObjectTypeInfo this_type;

			typedef InputStreamObject::shared_ptr_type pointer_type;

			static pointer_type getNullPointer()
			{
				pointer_type p;
				return p;
			}

			static pointer_type deallocate(pointer_type /* p */)
			{
				return getNullPointer();
			}
		};
	}
}
#endif
