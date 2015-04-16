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

#if ! defined(LIBMAUS2_LZ_BGZFINFLATESTREAM_HPP)
#define LIBMAUS2_LZ_BGZFINFLATESTREAM_HPP

#include <libmaus2/lz/BgzfInflateWrapper.hpp>
#include <libmaus2/lz/StreamWrapper.hpp>
#include <libmaus2/lz/BgzfStreamWrapper.hpp>
#include <libmaus2/lz/BgzfVirtualOffset.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateStream : 
			public ::libmaus2::lz::BgzfInflateWrapper, 
			public ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflate<std::istream> >
		{
			typedef BgzfInflateStream this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			BgzfInflateStream(std::istream & in)
			: ::libmaus2::lz::BgzfInflateWrapper(in), 
			  ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflate<std::istream> >(::libmaus2::lz::BgzfInflateWrapper::bgzf,64*1024,0)
			{
				exceptions(std::ios::badbit);
			}

			BgzfInflateStream(std::istream & in, std::ostream & out)
			: ::libmaus2::lz::BgzfInflateWrapper(in,out), 
			  ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflate<std::istream> >(::libmaus2::lz::BgzfInflateWrapper::bgzf,64*1024,0)
			{
				exceptions(std::ios::badbit);
			}
			
			BgzfInflateStream(std::istream & in, libmaus2::lz::BgzfVirtualOffset const & start, libmaus2::lz::BgzfVirtualOffset const & end)
			: ::libmaus2::lz::BgzfInflateWrapper(in,start,end), 
			  ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflate<std::istream> >(::libmaus2::lz::BgzfInflateWrapper::bgzf,64*1024,0)
			{
				exceptions(std::ios::badbit);
			}
		};
	}
}
#endif
