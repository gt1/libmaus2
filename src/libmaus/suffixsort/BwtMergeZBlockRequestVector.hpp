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
#if ! defined(LIBMAUS_SUFFIXSORT_BWTMERGEZBLOCKREQUESTVECTOR_HPP)
#define LIBMAUS_SUFFIXSORT_BWTMERGEZBLOCKREQUESTVECTOR_HPP

#include <libmaus/suffixsort/BwtMergeZBlockRequest.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		struct BwtMergeZBlockRequestVector
		{
			private:
			std::vector < ::libmaus::suffixsort::BwtMergeZBlockRequest > requests;
			
			public:
			BwtMergeZBlockRequestVector()
			{
			
			}
			
			BwtMergeZBlockRequestVector(std::istream & in)
			{
				uint64_t const siz = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
				for ( uint64_t i = 0; i < siz; ++i )
					push_back ( ::libmaus::suffixsort::BwtMergeZBlockRequest(in) );
			}
			
			template<typename stream_type>
			void serialise(stream_type & stream) const
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,size());
				for ( uint64_t i = 0; i < size(); ++i )
					(*this)[i].serialise(stream);
			}
			
			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}
			
			void push_back(::libmaus::suffixsort::BwtMergeZBlockRequest const & req)
			{
				requests.push_back(req);
			}
			
			uint64_t size() const
			{
				return requests.size();
			}
			
			::libmaus::suffixsort::BwtMergeZBlockRequest & operator[](uint64_t const i)
			{
				return requests.at(i);
			}
			::libmaus::suffixsort::BwtMergeZBlockRequest const & operator[](uint64_t const i) const
			{
				return requests.at(i);
			}
		};
	}
}
#endif

