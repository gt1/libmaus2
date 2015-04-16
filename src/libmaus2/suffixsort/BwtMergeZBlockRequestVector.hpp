/**
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
**/
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTMERGEZBLOCKREQUESTVECTOR_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTMERGEZBLOCKREQUESTVECTOR_HPP

#include <libmaus2/suffixsort/BwtMergeZBlockRequest.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct BwtMergeZBlockRequestVector
		{
			private:
			libmaus2::autoarray::AutoArray< ::libmaus2::suffixsort::BwtMergeZBlockRequest > requests;
			
			public:
			BwtMergeZBlockRequestVector()
			{
			
			}
			
			BwtMergeZBlockRequestVector(BwtMergeZBlockRequestVector const & o) : requests(o.requests.clone()) {}
			
			BwtMergeZBlockRequestVector(std::istream & in)
			{
				uint64_t const siz = ::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				resize(siz);
				for ( uint64_t i = 0; i < siz; ++i )
					(*this)[i] = ::libmaus2::suffixsort::BwtMergeZBlockRequest(in);
			}
			
			BwtMergeZBlockRequestVector & operator=(BwtMergeZBlockRequestVector const & o)
			{
				if ( this != &o )
				{
					requests = o.requests.clone();
				}
				return *this;
			}
			
			template<typename stream_type>
			void serialise(stream_type & stream) const
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,size());
				for ( uint64_t i = 0; i < size(); ++i )
					(*this)[i].serialise(stream);
			}
			
			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}
			
			void resize(uint64_t const n)
			{
				requests.resize(n);
			}
			
			uint64_t size() const
			{
				return requests.size();
			}
			
			::libmaus2::suffixsort::BwtMergeZBlockRequest & operator[](uint64_t const i)
			{
				return requests.at(i);
			}
			::libmaus2::suffixsort::BwtMergeZBlockRequest const & operator[](uint64_t const i) const
			{
				return requests.at(i);
			}
		};
	}
}
#endif

