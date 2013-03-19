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

#if ! defined(GENERICSERIALISE_HPP)
#define GENERICSERIALISE_HPP

#include <libmaus/util/StringSerialisation.hpp>
#include <sstream>

namespace libmaus
{
	namespace util
	{
		struct GenericSerialisation
		{
                        template<typename stream_type, typename data_type>
			static void serialise(stream_type & out, data_type const & D)
			{
				std::ostringstream ostr;
				ostr << D;
				::libmaus::util::StringSerialisation::serialiseString(out,ostr.str());
			}
                        template<typename stream_type, typename data_type>
			static data_type deserialise(stream_type & in)
			{
				std::string const s = ::libmaus::util::StringSerialisation::deserialiseString(in);
				std::istringstream istr(s);
				data_type d;
				istr >> d;
				
				if ( ! istr )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failure in GenericSerialise::deserialise()" << std::endl;
					se.finish();
					throw se;
				}
				
				return d;
			}
		};
	}
}
#endif
