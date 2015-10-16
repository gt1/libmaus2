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
#if ! defined(LIBMAUS2_LZ_XZDECODER_HPP)
#define LIBMAUS2_LZ_XZDECODER_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

#if defined(LIBMAUS2_HAVE_LZMA)
#include <lzma.h>
#endif

namespace libmaus2
{
	namespace lz
	{
		struct XzDecoder
		{
			std::istream & in;
			libmaus2::autoarray::AutoArray<uint8_t> Ainbuf;
			libmaus2::autoarray::AutoArray<uint8_t> Aoutbuf;

			uint8_t const * pa;
			uint8_t const * pc;
			uint8_t const * pe;

			#if defined(LIBMAUS2_HAVE_LZMA)
			lzma_stream lstr;
			lzma_ret lastret;
			#endif
			
			void initDecoder()
			{
				#if defined(LIBMAUS2_HAVE_LZMA)
				#if LZMA_VERSION <= UINT32_C(49990030)
				int const ret = lzma_auto_decoder(&lstr, NULL, NULL);
				#else
				lzma_ret const ret = lzma_auto_decoder(&lstr, UINT64_MAX, 0);
				#endif
				
				if ( ret != LZMA_OK )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "XzDecoder::initDecoder(): lzma_auto_decoder failed: " << ret << std::endl;
					lme.finish();
					throw lme;
				}
				#else
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "XzDecoder::initDecoder(): libmaus2 is compiled without liblzma support" << std::endl;
				lme.finish();
				throw lme;
				#endif
			}

			XzDecoder(std::istream & rin, size_t const inbufsize = 64*1024, size_t const outbufsize = 64*1024)
			: in(rin), Ainbuf(inbufsize), Aoutbuf(outbufsize), pa(Aoutbuf.end()), pc(Aoutbuf.end()), pe(Aoutbuf.end())
				#if defined(LIBMAUS2_HAVE_LZMA)	
				, lstr(LZMA_STREAM_INIT), lastret(LZMA_STREAM_END)
				#endif
			{
				initDecoder();
			}

			~XzDecoder()
			{
				#if defined(LIBMAUS2_HAVE_LZMA)	
				lzma_end(&lstr);
				#endif
			}

			size_t fillInputBuffer()
			{
				in.read(reinterpret_cast<char *>(Ainbuf.begin()),Ainbuf.size());
				
				if ( in.bad() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "XzDecoder::fillInputBuffer(): stream bad" << std::endl;
					lme.finish();
					throw lme;	
				}
				
				return in.gcount();
			}

			size_t read(
				uint8_t * 
					#if defined(LIBMAUS2_HAVE_LZMA)	
					A
					#endif
					, 
				size_t 
					#if defined(LIBMAUS2_HAVE_LZMA)	
					s
					#endif
			)
			{
				size_t r = 0;
				
				#if defined(LIBMAUS2_HAVE_LZMA)	
				while ( s )
				{
					// if output data buffer is empty
					if ( pc == pe )
					{
						// try to fill the input buffer
						if ( ! lstr.avail_in )
						{
							lstr.avail_in = fillInputBuffer();
							lstr.next_in = Ainbuf.begin();
						}
						
						// still no data, end of the file
						if ( ! lstr.avail_in )
						{
							s = 0;
												
							if ( lastret != LZMA_STREAM_END )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "XzDecoder::read(): file is truncated" << std::endl;
								lme.finish();
								throw lme;
							}
						}
						// we have data, try to decompress it
						else
						{
							lstr.next_out = Aoutbuf.begin();
							lstr.avail_out = Aoutbuf.size();

							// call decompression
							lzma_ret const ret = lzma_code(&lstr,LZMA_RUN);
							lastret = ret;

							pa = Aoutbuf.begin();
							pc = Aoutbuf.begin();
							pe = Aoutbuf.begin() + Aoutbuf.size() - lstr.avail_out;
							
							switch ( ret )
							{
								/*
								 * nothing special
								 */
								case LZMA_OK:
								{
									break;
								}
								/*
								 * end of compressed stream. set up new decoder in case another compressed stream follows this one
								 */
								case LZMA_STREAM_END:
								{
									// move rest of input data to front of buffer
									memmove(Ainbuf.begin(),lstr.next_in,lstr.avail_in);
									size_t const g = lstr.avail_in;
									lzma_end(&lstr);
									
									initDecoder();

									lstr.next_in = Ainbuf.begin();
									lstr.avail_in = g;

									break;
								}
								/*
								 * an actual error
								 */
								default:
								{
									libmaus2::exception::LibMausException lme;
									lme.getStream() << "XzDecoder::read(): liblzma error " << ret << std::endl;
									lme.finish();
									throw lme;
									break;
								}
							}
						}
					}
					
					size_t const tocopy = std::min(s,static_cast<size_t>(pe-pc));
					std::copy(pc,pc+tocopy,A);
					A += tocopy;
					s -= tocopy;
					pc += tocopy;
					r += tocopy;
				}
				#endif
				
				return r;
			}
		};
	}
}
#endif
