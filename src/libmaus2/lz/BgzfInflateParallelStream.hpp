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


#if ! defined(LIBMAUS_LZ_BGZFINFLATEPARALLELSTREAM_HPP)
#define LIBMAUS_LZ_BGZFINFLATEPARALLELSTREAM_HPP

#include <libmaus2/lz/StreamWrapper.hpp>
#include <libmaus2/lz/BgzfStreamWrapper.hpp>
#include <libmaus2/lz/BgzfInflateParallelWrapper.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateParallelStream : public ::libmaus2::lz::BgzfInflateParallelWrapper, public ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflateParallel >
		{
			typedef BgzfInflateParallelStream this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			BgzfInflateParallelStream(std::istream & in)
			: ::libmaus2::lz::BgzfInflateParallelWrapper(in), 
			  ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflateParallel >(::libmaus2::lz::BgzfInflateParallelWrapper::bgzf,64*1024,0)
			{
				exceptions(std::ios::badbit);
			}
			BgzfInflateParallelStream(std::istream & in, uint64_t const numthreads)
			: ::libmaus2::lz::BgzfInflateParallelWrapper(in,numthreads), 
			  ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflateParallel >(::libmaus2::lz::BgzfInflateParallelWrapper::bgzf,64*1024,0)
			{
				exceptions(std::ios::badbit);			
			}
			BgzfInflateParallelStream(std::istream & in, uint64_t const numthreads, uint64_t const numblocks)
			: ::libmaus2::lz::BgzfInflateParallelWrapper(in,numthreads,numblocks), 
			  ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflateParallel >(::libmaus2::lz::BgzfInflateParallelWrapper::bgzf,64*1024,0)
			{
				exceptions(std::ios::badbit);			
			}	

			BgzfInflateParallelStream(std::istream & in, std::ostream & out)
			: ::libmaus2::lz::BgzfInflateParallelWrapper(in,out), 
			  ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflateParallel >(::libmaus2::lz::BgzfInflateParallelWrapper::bgzf,64*1024,0)
			{
				exceptions(std::ios::badbit);			
			}
			
			BgzfInflateParallelStream(std::istream & in, std::ostream & out, uint64_t const numthreads)
			: ::libmaus2::lz::BgzfInflateParallelWrapper(in,out,numthreads), 
			  ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflateParallel >(::libmaus2::lz::BgzfInflateParallelWrapper::bgzf,64*1024,0)
			{
				exceptions(std::ios::badbit);			
			}
			BgzfInflateParallelStream(std::istream & in, std::ostream & out, uint64_t const numthreads, uint64_t const numblocks)
			: ::libmaus2::lz::BgzfInflateParallelWrapper(in,out,numthreads,numblocks), 
			  ::libmaus2::lz::BgzfStreamWrapper< ::libmaus2::lz::BgzfInflateParallel >(::libmaus2::lz::BgzfInflateParallelWrapper::bgzf,64*1024,0)
			{
				exceptions(std::ios::badbit);			
			}	
		};
	}
}
#endif
