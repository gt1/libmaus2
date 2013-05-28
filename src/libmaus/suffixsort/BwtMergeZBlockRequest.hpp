/**
    suds
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
#if !defined(LIBMAUS_SUFFIXSORT_BWTMERGEZBLOCKREQUEST_HPP)
#define LIBMAUS_SUFFIXSORT_BWTMERGEZBLOCKREQUEST_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <sstream>

namespace libmaus
{
	namespace suffixsort
	{
		struct BwtMergeZBlockRequest
		{
			uint64_t zabspos;
			
			BwtMergeZBlockRequest(uint64_t rzabspos = 0) : zabspos(rzabspos) {}
			BwtMergeZBlockRequest(std::istream & in) : zabspos(::libmaus::util::NumberSerialisation::deserialiseNumber(in)) {}
			
			template<typename stream_type>
			void serialise(stream_type & out) const
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(out,zabspos);
			}
			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}
			
			operator uint64_t() const
			{
				return zabspos;
			}
		};
	}
}
#endif

