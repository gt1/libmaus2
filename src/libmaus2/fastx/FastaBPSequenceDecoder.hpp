/*
    libmaus2
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
#if ! defined(LIBMAUS2_FASTX_FASTABPSEQUENCEDECODER_HPP)
#define LIBMAUS2_FASTX_FASTABPSEQUENCEDECODER_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>
#include <libmaus2/fastx/FastABPConstants.hpp>
#include <libmaus2/hashing/Crc32.hpp>
#include <libmaus2/util/utf8.hpp>

namespace libmaus2
{
	namespace fastx
	{
		template<typename _remap_type>
		struct FastaBPSequenceDecoderTemplate
		{
			typedef _remap_type remap_type;
			typedef FastaBPSequenceDecoderTemplate<remap_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			std::istream & in;
			uint64_t const bs;
			libmaus2::autoarray::AutoArray<char> Bin;
			bool eof;
			bool checkcrc;

			FastaBPSequenceDecoderTemplate(std::istream & rin, uint64_t const rbs, bool rcheckcrc = false)
			: in(rin), bs(rbs), Bin((bs+1)/2,false), eof(false), checkcrc(rcheckcrc)
			{

			}

			ssize_t read(char * p, uint64_t const m)
			{
				if ( eof )
					return 0;

				uint64_t bytesread = 0;

				int const flags = in.get();
				if ( flags < 0 )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FastaBPSequenceDecoder::read(): EOF/error while reading block flags" << std::endl;
					lme.finish();
					throw lme;
				}
				bytesread += 1;

				// check whether this is the last block
				eof = ( flags & ::libmaus2::fastx::FastABPConstants::base_block_last );
				// determine number of bytes to be produced from this block
				uint64_t const toread = eof ? libmaus2::util::UTF8::decodeUTF8(in,bytesread) : bs;

				if ( m < toread )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FastaBPSequenceDecoder::read(): buffer is too small for block data" << std::endl;
					lme.finish();
					throw lme;
				}

				switch ( flags & ::libmaus2::fastx::FastABPConstants::base_block_mask )
				{
					case ::libmaus2::fastx::FastABPConstants::base_block_4:
					{
						in.read(Bin.begin(),(toread+3)/4);
						if ( in.gcount() != static_cast<int64_t>((toread+3)/4) )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "FastaBPSequenceDecoder::read(): input failure" << std::endl;
							lme.finish();
							throw lme;
						}
						bytesread += in.gcount();
						uint64_t k = 0;

						for ( uint64_t j = 0; j < toread/4; ++j )
						{
							uint8_t const u = static_cast<uint8_t>(Bin[j]);

							p[k++] = remap_type::remapChar((u >> 6) & 3);
							p[k++] = remap_type::remapChar((u >> 4) & 3);
							p[k++] = remap_type::remapChar((u >> 2) & 3);
							p[k++] = remap_type::remapChar((u >> 0) & 3);
						}

						if ( (toread) % 4 )
						{
							uint8_t const u = static_cast<uint8_t>(Bin[toread/4]);

							for ( uint64_t j = 0; j < ((toread)%4); ++j )
								p[k++] = remap_type::remapChar((u >> (6-2*j)) & 3);
						}
						break;
					}
					case ::libmaus2::fastx::FastABPConstants::base_block_5:
					{
						in.read(Bin.begin(),(toread+2)/3);
						if ( in.gcount() != static_cast<int64_t>((toread+2)/3) )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "FastaBPSequenceDecoder::read(): input failure" << std::endl;
							lme.finish();
							throw lme;
						}
						bytesread += in.gcount();
						uint64_t k = 0;

						for ( uint64_t j = 0; j < (toread)/3; ++j )
						{
							uint8_t const u = Bin[j];

							p[k++] = remap_type::remapChar((u/(5*5))%5);
							p[k++] = remap_type::remapChar((u/(5*1))%5);
							p[k++] = remap_type::remapChar((u/(1*1))%5);
						}
						if ( toread % 3 )
						{
							uint8_t const u = Bin[toread/3];

							switch ( toread % 3 )
							{
								case 1:
									p[k++] = remap_type::remapChar((u/(5*5))%5);
									break;
								case 2:
									p[k++] = remap_type::remapChar((u/(5*5))%5);
									p[k++] = remap_type::remapChar((u/(5*1))%5);
									break;
							}
						}

						assert ( k == toread );
						break;
					}
					case ::libmaus2::fastx::FastABPConstants::base_block_16:
					{
						in.read(Bin.begin(),(toread+1)/2);
						if ( in.gcount() != static_cast<int64_t>((toread+1)/2) )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "FastaBPSequenceDecoder::read(): input failure" << std::endl;
							lme.finish();
							throw lme;
						}
						bytesread += in.gcount();
						uint64_t k = 0;

						for ( uint64_t j = 0; j < toread/2; ++j )
						{
							uint8_t const u = Bin[j];

							p[k++] = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(u >> 4 );
							p[k++] = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(u & 0xF);
						}
						if ( toread&1 )
						{
							uint8_t const u = Bin[toread/2];
							p[k++] = libmaus2::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(u >> 4);
						}

						assert ( k == toread );
						break;
					}
					default:
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "FastaBPSequenceDecoder::read(): EOF/error while reading block type" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				uint64_t const inputcount = in.gcount();

				// align file
				if ( bytesread % 8 )
				{
					uint64_t const toskip = 8-(bytesread % 8);
					in.ignore(toskip);
					if ( in.gcount() != static_cast<int64_t>(toskip) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "FastaBPSequenceDecoder::read(): EOF/error while aligning file to 8 byte boundary" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				// read crc
				uint8_t crcbytes[8];
				in.read(reinterpret_cast<char *>(&crcbytes[0]),sizeof(crcbytes));
				if ( in.gcount() != static_cast<int64_t>(sizeof(crcbytes)) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FastaBPSequenceDecoder::read(): EOF/error while reading block crc" << std::endl;
					lme.finish();
					throw lme;
				}

				uint32_t const crcin =
					(static_cast<uint32_t>(crcbytes[0]) << 24) |
					(static_cast<uint32_t>(crcbytes[1]) << 16) |
					(static_cast<uint32_t>(crcbytes[2]) <<  8) |
					(static_cast<uint32_t>(crcbytes[3]) <<  0);
				uint32_t const crcout =
					(static_cast<uint32_t>(crcbytes[4]) << 24) |
					(static_cast<uint32_t>(crcbytes[5]) << 16) |
					(static_cast<uint32_t>(crcbytes[6]) <<  8) |
					(static_cast<uint32_t>(crcbytes[7]) <<  0);

				if ( checkcrc )
				{
					uint32_t const crcincomp = libmaus2::hashing::Crc32::crc32_8bytes(p,toread,0 /*prev*/);
					uint32_t const crcoutcomp = libmaus2::hashing::Crc32::crc32_8bytes(Bin.begin(),inputcount,0 /*prev*/);

					if ( crcin != crcincomp )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "FastaBPSequenceDecoder::read(): crc error on uncompressed data" << std::endl;
						lme.finish();
						throw lme;
					}
					if ( crcout != crcoutcomp )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "FastaBPSequenceDecoder::read(): crc error on compressed data" << std::endl;
						lme.finish();
						throw lme;
					}
				}

				return toread;
			}
		};

		struct FastaBPSequenceDecoderRemap
		{
			static inline char remapChar(char const c)
			{
				return libmaus2::fastx::remapChar(c);
			}
		};

		struct FastaBPSequenceDecoderRemapIdentity
		{
			static inline char remapChar(char const c)
			{
				return c;
			}
		};

		typedef FastaBPSequenceDecoderTemplate<FastaBPSequenceDecoderRemap> FastaBPSequenceDecoder;
		typedef FastaBPSequenceDecoderTemplate<FastaBPSequenceDecoderRemapIdentity> FastaBPSequenceDecoderIdentity;
	}
}
#endif
