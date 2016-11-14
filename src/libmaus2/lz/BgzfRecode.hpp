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
#if ! defined(LIBMAUS2_LZ_BGZFRECODE_HPP)
#define LIBMAUS2_LZ_BGZFRECODE_HPP

#include <libmaus2/lz/BgzfDeflateBase.hpp>
#include <libmaus2/lz/BgzfInflateBase.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfRecode
		{
			libmaus2::lz::BgzfInflateBase inflatebase;
			libmaus2::lz::BgzfDeflateBase deflatebase;

			std::istream & in;
			std::ostream & out;

			libmaus2::lz::BgzfInflateBase::BaseBlockInfo P;

			std::vector< ::libmaus2::lz::BgzfDeflateOutputCallback *> blockoutputcallbacks;

			BgzfRecode(std::istream & rin, std::ostream & rout, int const level = Z_DEFAULT_COMPRESSION)
			: inflatebase(), deflatebase(level,true,BgzfConstants::getBgzfMaxBlockSize()), in(rin), out(rout)
			{

			}

			std::pair<uint64_t,bool> getBlockPlusEOF()
			{
				P = inflatebase.readBlock(in);

				if ( (! P.uncompdatasize) && (in.get() == std::istream::traits_type::eof()) )
					return std::pair<uint64_t,bool>(P.uncompdatasize,true);

				inflatebase.decompressBlock(reinterpret_cast<char *>(deflatebase.pa),P);
				deflatebase.pc = deflatebase.pa + P.uncompdatasize;

				return std::pair<uint64_t,bool>(P.uncompdatasize,false);
			}

			uint64_t getBlock()
			{
				return !(getBlockPlusEOF().second);
			}

			void registerBlockOutputCallback(::libmaus2::lz::BgzfDeflateOutputCallback * cb)
			{
				blockoutputcallbacks.push_back(cb);
			}

			void streamWrite(
				uint8_t const * inp,
				uint64_t const incnt,
				uint8_t const * outp,
				uint64_t const outcnt
			)
			{
				out.write(reinterpret_cast<char const *>(outp),outcnt);

				if ( ! out )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "failed to write compressed data to bgzf stream." << std::endl;
					se.finish();
					throw se;
				}

				for ( uint64_t i = 0; i < blockoutputcallbacks.size(); ++i )
					(*(blockoutputcallbacks[i]))(inp,incnt,outp,outcnt);
			}

			void streamWrite(
				uint8_t const * inp,
				uint8_t const * outp,
				BgzfDeflateZStreamBaseFlushInfo const & BDZSBFI
			)
			{
				assert ( BDZSBFI.blocks == 1 || BDZSBFI.blocks == 2 );

				if ( BDZSBFI.blocks == 1 )
				{
					/* write data to stream, one block */
					streamWrite(inp, BDZSBFI.block_a_u, outp, BDZSBFI.block_a_c);
				}
				else
				{
					assert ( BDZSBFI.blocks == 2 );
					/* write data to stream, two blocks */
					streamWrite(inp                    , BDZSBFI.block_a_u, outp                    , BDZSBFI.block_a_c);
					streamWrite(inp + BDZSBFI.block_a_u, BDZSBFI.block_b_u, outp + BDZSBFI.block_a_c, BDZSBFI.block_b_c);
				}
			}

			void putBlock()
			{
				BgzfDeflateZStreamBaseFlushInfo const BDZSBFI = deflatebase.flush(true);
				assert ( ! BDZSBFI.movesize );
				#if 0
				uint64_t const writesize = BDZSBFI.getCompressedSize();
				out.write(reinterpret_cast<char const *>(deflatebase.outbuf.begin()), writesize);
				#endif

				streamWrite(deflatebase.inbuf.begin(),deflatebase.outbuf.begin(),BDZSBFI);
			}

			void addEOFBlock()
			{
				deflatebase.deflatereinit();
				deflatebase.pc = deflatebase.pa;
				BgzfDeflateZStreamBaseFlushInfo const BDZSBFI = deflatebase.flush(true);
				assert ( ! BDZSBFI.movesize );
				#if 0
				uint64_t const writesize = BDZSBFI.getCompressedSize();
				out.write(reinterpret_cast<char const *>(deflatebase.outbuf.begin()), writesize);
				#endif

				streamWrite(deflatebase.inbuf.begin(),deflatebase.outbuf.begin(),BDZSBFI);
			}
		};
	}
}
#endif
