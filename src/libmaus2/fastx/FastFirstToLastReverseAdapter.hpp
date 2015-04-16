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

#if ! defined(FASTADAPTER_HPP)
#define FASTADAPTER_HPP

namespace libmaus2
{
	namespace fastx
	{
		template<typename _reader_type>
		struct FastFirstToLastReverseAdapter
		{
			typedef _reader_type reader_type;
			typedef typename _reader_type::pattern_type pattern_type;
			typedef typename pattern_type::unique_ptr_type pattern_ptr_type;

			reader_type & reader;
			
			bool havefirst;
			bool usedfirst;
			
			pattern_ptr_type first;
			
			uint64_t patid;
			
			bool const reverse;

			FastFirstToLastReverseAdapter(reader_type & rreader, bool const mapfirsttolast, bool const rreverse)
			: reader(rreader), havefirst(false), usedfirst(false), first(new pattern_type), patid(0), reverse(rreverse)
			{
				if ( mapfirsttolast )
					havefirst = reader.getNextPatternUnlocked(*first);
			
			}

			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				bool r;
			
				if ( reader.getNextPatternUnlocked(pattern) )
				{
					r = true;
				}
				else if ( havefirst && ! usedfirst )
				{
					pattern = *first;
					usedfirst = true;
					r = true;
				}
				else
				{
					r = false;
				}

				if ( r && reverse )
					std::reverse(pattern.spattern.begin(),pattern.spattern.end());

				pattern.pattern = pattern.spattern.c_str();
				pattern.patid = patid++;
				
				return r;
			}
		};
	}
}
#endif
