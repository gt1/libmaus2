/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_AIO_GENERICPEEKER_HPP)
#define LIBMAUS2_AIO_GENERICPEEKER_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>

namespace libmaus2
{
	namespace aio
	{
		template<typename _reader_type>
		struct GenericPeeker
		{
			typedef _reader_type reader_type;
			typedef typename reader_type::data_type data_type;
			typedef GenericPeeker<reader_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			reader_type & reader;

			bool rawGet(data_type & D)
			{
				return reader.getNext(D);
			}

			data_type slot;
			bool slotfilled;

			GenericPeeker(reader_type & rreader) : reader(rreader) {}

			bool peekNext(data_type & D)
			{
				slotfilled = slotfilled || rawGet(slot);

				if ( slotfilled )
					D = slot;

				return slotfilled;
			}

			bool getNext(data_type & D)
			{
				if ( slotfilled )
				{
					D = slot;
					slotfilled = false;
					return true;
				}
				else
				{
					return rawGet(D);
				}
			}
		};
	}
}
#endif
