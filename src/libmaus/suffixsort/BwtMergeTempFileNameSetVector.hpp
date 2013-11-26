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
#if ! defined(LIBMAUS_SUFFIXSORT_BWTMERGETEMPFILENAMESETVECTOR_HPP)
#define LIBMAUS_SUFFIXSORT_BWTMERGETEMPFILENAMESETVECTOR_HPP

#include <libmaus/suffixsort/BwtMergeTempFileNameSet.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		struct BwtMergeTempFileNameSetVector
		{
			private:
			std::vector < ::libmaus::suffixsort::BwtMergeTempFileNameSet > V;
			
			public:
			BwtMergeTempFileNameSetVector(std::string const & tmpfilenamebase, uint64_t const num, uint64_t const numbwt)
			: V(num)
			{
				for ( uint64_t i = 0; i < num; ++i )
					V[i] = ::libmaus::suffixsort::BwtMergeTempFileNameSet(tmpfilenamebase,i,numbwt);
			}
			
			::libmaus::suffixsort::BwtMergeTempFileNameSet const & operator[](size_t i) const
			{
				return V[i];
			}
		};
	}
}
#endif

