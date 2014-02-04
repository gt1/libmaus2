/*
    libmaus
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
#if ! defined(LIBMAUS_LZ_SIMPLECOMPRESSEDCONCATINPUTSTREAMFRAGMENT_HPP)
#define LIBMAUS_LZ_SIMPLECOMPRESSEDCONCATINPUTSTREAMFRAGMENT_HPP

#include <map>
#include <vector>

namespace libmaus
{
	namespace lz
	{
		template<typename _stream_type>
		struct SimpleCompressedConcatInputStreamFragment
		{
			typedef _stream_type stream_type;
		
			std::pair<uint64_t,uint64_t> low;
			std::pair<uint64_t,uint64_t> high;
			stream_type * stream;
			
			SimpleCompressedConcatInputStreamFragment()
			: low(0,0), high(0,0)
			{
			
			}
			
			SimpleCompressedConcatInputStreamFragment(
				std::pair<uint64_t,uint64_t> const rlow,
				std::pair<uint64_t,uint64_t> const rhigh,
				stream_type * rstream
			) : low(rlow), high(rhigh), stream(rstream)
			{
			
			}
			
			bool empty() const
			{
				return low == high;
			}
			
			static std::vector<SimpleCompressedConcatInputStreamFragment> filter(
				std::vector<SimpleCompressedConcatInputStreamFragment> const & fragments
			)
			{
				std::vector<SimpleCompressedConcatInputStreamFragment> outfragments;
				for ( uint64_t i = 0; i < fragments.size(); ++i )
					if ( ! fragments[i].empty() )
						outfragments.push_back(fragments[i]);
				return outfragments;
			}
		};
	}
}
#endif
