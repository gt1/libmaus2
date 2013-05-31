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

#if ! defined(LIBMAUS_LZ_BGZFINFLATESTREAM_HPP)
#define LIBMAUS_LZ_BGZFINFLATESTREAM_HPP

#include <libmaus/lz/BgzfInflateWrapper.hpp>
#include <libmaus/lz/StreamWrapper.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfInflateStream : public ::libmaus::lz::BgzfInflateWrapper, public ::libmaus::lz::StreamWrapper< ::libmaus::lz::BgzfInflate<std::istream> >
		{
			typedef BgzfInflateStream this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			BgzfInflateStream(std::istream & in)
			: ::libmaus::lz::BgzfInflateWrapper(in), 
			  ::libmaus::lz::StreamWrapper< ::libmaus::lz::BgzfInflate<std::istream> >(::libmaus::lz::BgzfInflateWrapper::bgzf,64*1024,0)
			{
			
			}	
			BgzfInflateStream(std::istream & in, std::ostream & out)
			: ::libmaus::lz::BgzfInflateWrapper(in,out), 
			  ::libmaus::lz::StreamWrapper< ::libmaus::lz::BgzfInflate<std::istream> >(::libmaus::lz::BgzfInflateWrapper::bgzf,64*1024,0)
			{
			
			}	
		};
	}
}
#endif
