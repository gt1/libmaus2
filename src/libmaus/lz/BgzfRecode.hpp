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
#if ! defined(LIBMAUS_LZ_BGZFRECODE_HPP)
#define LIBMAUS_LZ_BGZFRECODE_HPP

#include <libmaus/lz/BgzfDeflateBase.hpp>
#include <libmaus/lz/BgzfInflateBase.hpp>

namespace libmaus
{
	namespace lz
	{
		struct BgzfRecode
		{
			libmaus::lz::BgzfInflateBase inflatebase;
			libmaus::lz::BgzfDeflateBase deflatebase;
			
			std::istream & in;
			std::ostream & out;
			
			std::pair<uint64_t,uint64_t> P;
			
			BgzfRecode(std::istream & rin, std::ostream & rout, int const level = Z_DEFAULT_COMPRESSION)
			: inflatebase(), deflatebase(level,true), in(rin), out(rout)
			{
			
			}
			
			uint64_t getBlock()
			{
				P = inflatebase.readBlock(in);
				
				if ( ! P.second )
					return false;
				
				inflatebase.decompressBlock(reinterpret_cast<char *>(deflatebase.pa),P);
				deflatebase.pc = deflatebase.pa + P.second;
				
				return P.second;
			}
			
			void putBlock()
			{
				uint64_t const writesize = deflatebase.flush(true);
				out.write(reinterpret_cast<char const *>(deflatebase.outbuf.begin()), writesize);
			}
			
			void addEOFBlock()
			{
				deflatebase.deflatereinit();
				deflatebase.pc = deflatebase.pa;
				uint64_t const writesize = deflatebase.flush(true);
				out.write(reinterpret_cast<char const *>(deflatebase.outbuf.begin()), writesize);		
			}
		};
	}
}
#endif
