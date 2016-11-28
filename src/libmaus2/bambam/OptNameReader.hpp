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
#if ! defined(LIBMAUS2_BAMBAM_OPTNAMEREADER_HPP)
#define LIBMAUS2_BAMBAM_OPTNAMEREADER_HPP

#include <libmaus2/bambam/OptName.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct OptNameReader
		{
			typedef OptNameReader this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
			std::istream & in;

			libmaus2::bambam::OptName slot;
			bool slotfilled;

			OptNameReader(std::string const & name)
			: PISI(new libmaus2::aio::InputStreamInstance(name)), in(*PISI), slot(), slotfilled(false)
			{
			}

			OptNameReader(std::istream & rin)
			: in(rin)
			{

			}

			bool rawGet()
			{
				if ( in.peek() == std::istream::traits_type::eof() )
					return false;
				else
				{
					slot.deserialise(in);
					return true;
				}
			}

			bool peekNext(libmaus2::bambam::OptName & ON)
			{
				if ( ! slotfilled )
					slotfilled = rawGet();
				if ( ! slotfilled )
					return false;
				else
				{
					ON = slot;
					return true;
				}
			}

			bool getNext(libmaus2::bambam::OptName & ON)
			{
				bool const ok = peekNext(ON);

				if ( ok )
				{
					slotfilled = false;
					return true;
				}
				else
				{
					return false;
				}
			}
		};
	}
}
#endif
