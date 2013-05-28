/*
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
*/
#if ! defined(LIBMAUS_SUFFIXSORT_BWTMERGEZBLOCK_HPP)
#define LIBMAUS_SUFFIXSORT_BWTMERGEZBLOCK_HPP

#include <sstream>
#include <libmaus/types/types.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		struct BwtMergeZBlock
		{
			uint64_t zabspos;
			uint64_t zrank;
			
			BwtMergeZBlock()
			: zabspos(0), zrank(0)
			{
			
			}
			
			BwtMergeZBlock(uint64_t const rzabspos, uint64_t const rzrank)
			: zabspos(rzabspos), zrank(rzrank) {}
			
			BwtMergeZBlock(std::istream & stream)
			:
				zabspos(::libmaus::util::NumberSerialisation::deserialiseNumber(stream)),
				zrank(::libmaus::util::NumberSerialisation::deserialiseNumber(stream))
			{
			
			}
			
			static BwtMergeZBlock load(std::string const & s)
			{
				std::istringstream istr(s);
				return BwtMergeZBlock(istr);
			}
			
			template<typename stream_type>
			void serialise(stream_type & stream) const
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,zabspos);
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,zrank);
			}
			
			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}
		};

		inline std::ostream & operator<<(std::ostream & out, BwtMergeZBlock const & zblock)
		{
			out << "BwtMergeZBlock(abspos=" << zblock.zabspos << ",rank=" << zblock.zrank << ")";
			return out;
		}
	}
}
#endif
