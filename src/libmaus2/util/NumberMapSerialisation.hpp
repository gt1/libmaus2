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
#if ! defined(LIBMAUS2_UTIL_NUMBERMAPSERIALISATION_HPP)
#define LIBMAUS2_UTIL_NUMBERMAPSERIALISATION_HPP

#include <map>
#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace util
	{
		struct NumberMapSerialisation
		{
			template<typename stream_type, typename key_type, typename value_type>
			static void serialiseMap(stream_type & stream, std::map<key_type,value_type> const & M)
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,M.size());

				for ( typename std::map<key_type,value_type>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
				{
					::libmaus2::util::NumberSerialisation::serialiseNumber(stream,ita->first);
					::libmaus2::util::NumberSerialisation::serialiseNumber(stream,ita->second);
				}
			}

			template<typename key_type, typename value_type>
			static std::string serialiseMap(std::map<key_type,value_type> const & M)
			{
				std::ostringstream ostr;
				serialiseMap(ostr,M);
				return ostr.str();
			}


			template<typename stream_type, typename key_type, typename value_type>
			static std::map<key_type,value_type> deserialiseMap(stream_type & stream)
			{
				uint64_t const n = ::libmaus2::util::NumberSerialisation::deserialiseNumber(stream);
				std::map<key_type,value_type> M;

				for ( uint64_t i = 0; i < n; ++i )
				{
					uint64_t const k = ::libmaus2::util::NumberSerialisation::deserialiseNumber(stream);
					uint64_t const v = ::libmaus2::util::NumberSerialisation::deserialiseNumber(stream);
					M[k] = v;
				}

				return M;
			}

			template<typename key_type, typename value_type>
			static std::map<key_type,value_type> deserialiseMap(std::string const & ser)
			{
				std::istringstream istr(ser);
				return deserialiseMap<std::istringstream,key_type,value_type>(istr);
			}
		};
	}
}
#endif
