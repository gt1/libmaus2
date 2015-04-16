/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_LZ_SNAPPYINPUTSTREAMARRAY_HPP)
#define LIBMAUS_LZ_SNAPPYINPUTSTREAMARRAY_HPP

#include <libmaus2/lz/SnappyInputStream.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct SnappyInputStreamArray
		{
			::libmaus2::autoarray::AutoArray<SnappyInputStream::unique_ptr_type> A;

			template<typename iterator>			
			SnappyInputStreamArray(std::istream & in, iterator offa, iterator offe)
			: A( (offe-offa) ? ((offe-offa)-1) : 0 )
			{
				uint64_t i = 0;
				for ( iterator offc = offa; offc != offe && offc+1 != offe; ++offc, ++i )
				{
					SnappyInputStream::unique_ptr_type ptr(
                                                        new SnappyInputStream(in,*offc,true,*(offc+1))
                                                );
					assert ( i < A.size() );
					A [ i ] = UNIQUE_PTR_MOVE(ptr);
				}
			}
			
			SnappyInputStream & operator[](uint64_t const i)
			{
				return *(A[i]);
			}

			SnappyInputStream const & operator[](uint64_t const i) const
			{
				return *(A[i]);
			}

			void setEofThrow(bool const v)
			{
				for ( uint64_t i = 0; i < A.size(); ++i )
					A[i]->setEofThrow(v);
			}
		};		
	}
}
#endif
