/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_GAMMA_GAMMADIFFERENCEOUTPUTBUFFER_HPP)
#define LIBMAUS2_GAMMA_GAMMADIFFERENCEOUTPUTBUFFER_HPP

#include <libmaus2/gamma/GammaDifferenceMerge.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type, int mindif = 1>
		struct GammaDifferenceOutputBuffer
		{
			typedef _data_type data_type;
			typedef GammaDifferenceOutputBuffer<data_type, mindif> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::gamma::GammaDifferenceMerge<data_type, mindif> merge;
			libmaus2::autoarray::AutoArray<data_type> A;
			uint64_t f;

			GammaDifferenceOutputBuffer(std::string const & rprefix, uint64_t const n)
			: merge(rprefix), A(n,false), f(0)
			{
			}

			~GammaDifferenceOutputBuffer()
			{
			}

			void flush()
			{
				if ( f )
				{
					merge.add(A.begin(),A.begin()+f);
					f = 0;
				}
			}

			void flush(std::string const & ofn)
			{
				flush();
				merge.merge(ofn);
			}

			void put(data_type const & v)
			{
				assert ( f != A.size() );
				A[f++] = v;
				if ( f == A.size() )
					flush();
			}
		};
	}
}
#endif
