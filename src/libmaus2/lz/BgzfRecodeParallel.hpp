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
#if ! defined(LIBMAUS_LZ_BGZFRECODEPARALLEL_HPP)
#define LIBMAUS_LZ_BGZFRECODEPARALLEL_HPP

#include <libmaus2/lz/BgzfInflateDeflateParallel.hpp>
#include <libmaus2/lz/BgzfParallelRecodeDeflateBase.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfRecodeParallel : public ::libmaus2::lz::BgzfConstants
		{
			libmaus2::lz::BgzfInflateDeflateParallel BIDP; // (std::cin,std::cout,Z_DEFAULT_COMPRESSION,32,128);
			libmaus2::lz::BgzfParallelRecodeDeflateBase deflatebase;
			std::pair<uint64_t,uint64_t> P;

			BgzfRecodeParallel(
				std::istream & in, std::ostream & out,
				int const level, // = Z_DEFAULT_COMPRESSION,
				uint64_t const numthreads,
				uint64_t const numbuffers
			) : BIDP(in,out,level,numthreads,numbuffers), deflatebase(), P(0,0)
			{
			
			}
			
			~BgzfRecodeParallel()
			{
				BIDP.flush();
			}

			void registerBlockOutputCallback(::libmaus2::lz::BgzfDeflateOutputCallback * cb)
			{
				BIDP.registerBlockOutputCallback(cb);
			}

			
			bool getBlock()
			{
				BgzfInflateInfo const info = 
					BIDP.readAndInfo(reinterpret_cast<char *>(deflatebase.B.begin()),deflatebase.B.size());
				P.second = info.uncompressed;
				deflatebase.pc = deflatebase.pa + P.second;
				
				return !((info.uncompressed==0) && info.streameof);
			}
			
			void putBlock()
			{
				BIDP.write(reinterpret_cast<char const *>(deflatebase.B.begin()),P.second);
			}
			
			void addEOFBlock()
			{
				BIDP.flush();
			}
		};
	}
}
#endif
