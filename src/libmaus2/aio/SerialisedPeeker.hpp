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
#if ! defined(LIBMAUS2_AIO_SERIALISEDPEEKER_HPP)
#define LIBMAUS2_AIO_SERIALISEDPEEKER_HPP

#include <libmaus2/aio/InputStreamInstance.hpp>

namespace libmaus2
{
	namespace aio
	{
		template<typename _data_type>
		struct SerialisedPeeker
		{
			typedef _data_type data_type;
			typedef SerialisedPeeker<data_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
			std::istream & ISI;
			
			bool rawGet(data_type & D)
			{
				if ( ISI.peek() == std::istream::traits_type::eof() )
					return false;
				else
				{
					D.deserialise(ISI);
					return true;
				}
			}
			
			data_type slot;
			bool slotfilled;
			
			SerialisedPeeker(std::istream & rISI) : ISI(rISI), slotfilled(false) {}
			SerialisedPeeker(std::string const & fn) : PISI(new libmaus2::aio::InputStreamInstance(fn)), ISI(*PISI), slotfilled(false) {}
			
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
