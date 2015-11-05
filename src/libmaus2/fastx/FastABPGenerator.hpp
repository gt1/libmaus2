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
#if ! defined(LIBMAUS2_FASTX_FASTABPGENERATOR_HPP)
#define LIBMAUS2_FASTX_FASTABPGENERATOR_HPP

#include <libmaus2/bambam/BamSeqEncodeTable.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>
#include <libmaus2/fastx/FastABPConstants.hpp>
#include <libmaus2/fastx/FastADefinedBasesTable.hpp>
#include <libmaus2/fastx/FastAInfo.hpp>
#include <libmaus2/fastx/FastAStreamSet.hpp>
#include <libmaus2/hashing/Crc32.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FastABPGenerator
		{
			static void fastAToFastaBP(
				std::istream & fain,
				std::ostream & finalout,
				std::ostream * verboseout = 0,
				uint64_t const bs = 64*1024
			)
			{
				// to upper case table
				libmaus2::autoarray::AutoArray<uint8_t> toup(256,false);
				for ( uint64_t i = 0; i < 256; ++i )
					if ( isalpha(i) )
						toup[i] = toupper(i);
					else
						toup[i] = i;

				// base check table
				libmaus2::fastx::FastADefinedBasesTable deftab;
				// histogram
				libmaus2::autoarray::AutoArray<uint64_t> H(256,false);
				// bam encoding table
				libmaus2::bambam::BamSeqEncodeTable const bamencodetab;

				// write file magic
				char cmagic[] = { 'F', 'A', 'S', 'T', 'A', 'B', 'P', '\0' };
				std::string const magic(&cmagic[0],&cmagic[sizeof(cmagic)]);
				uint64_t filepos = 0;
				for ( unsigned int i = 0; i < magic.size(); ++i, ++filepos )
					finalout.put(magic[i]);
				// write block size
				filepos += libmaus2::util::NumberSerialisation::serialiseNumber(finalout,bs);

				std::vector<uint64_t> seqmetaposlist;
				libmaus2::fastx::FastAStreamSet streamset(fain);
				std::pair<std::string,libmaus2::fastx::FastAStream::shared_ptr_type> P;
				libmaus2::autoarray::AutoArray<char> Bin(bs,false);
				libmaus2::autoarray::AutoArray<char> Bout((bs+1)/2,false);
				uint64_t rcnt = 0;
				uint64_t rncnt = 0;
				uint64_t ncnt = 0;
				uint64_t totalseqlen = 0;
				while ( streamset.getNextStream(P) )
				{
					std::string const & sid = P.first;
					std::istream & in = *(P.second);
					std::vector<uint64_t> blockstarts;
					uint64_t prevblocksize = bs;
					uint64_t seqlen = 0;

					if ( ! sid.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "FastABPGenerator::fastAToFastaBP: Empty sequence names are not supported." << std::endl;
						lme.finish();
						throw lme;
					}

					// length of name
					filepos += libmaus2::util::UTF8::encodeUTF8(sid.size(),finalout);
					// name
					finalout.write(sid.c_str(),sid.size());
					filepos += sid.size();

					while ( filepos % sizeof(uint64_t) )
					{
						finalout.put(0);
						filepos++;
					}

					while ( in )
					{
						in.read(Bin.begin(),bs);
						uint64_t const n = in.gcount();

						if ( n )
						{
							// check block size consistency
							assert ( prevblocksize == bs );
							prevblocksize = n;

							bool const lastblock = (n != bs);
							uint8_t const lastblockflag = lastblock ? ::libmaus2::fastx::FastABPConstants::base_block_last : 0;

							//
							seqlen += n;

							// block start
							blockstarts.push_back(filepos);

							// clear histogram table
							std::fill(H.begin(),H.end(),0ull);

							// convert to upper case and fill histogram
							for ( uint64_t i = 0; i < n; ++i )
							{
								uint8_t const v = toup[static_cast<uint8_t>(Bin[i])];
								H[v]++;
								Bin[i] = static_cast<char>(v);
							}

							uint64_t const regcnt = H['A'] + H['C'] + H['G'] + H['T'];
							uint64_t const regncnt = regcnt + H['N'];

							// output block pointer
							uint8_t * outp = reinterpret_cast<uint8_t *>(Bout.begin());

							if ( regcnt == n )
							{
								// block type
								finalout.put(::libmaus2::fastx::FastABPConstants::base_block_4 | lastblockflag);
								filepos++;

								// store length if last block
								if ( lastblockflag )
									filepos += libmaus2::util::UTF8::encodeUTF8(n,finalout);

								char const * inp = Bin.begin();

								while ( inp != Bin.begin() + (n/4)*4 )
								{
									uint8_t v = 0;
										 v |= libmaus2::fastx::mapChar(*(inp++));
									v <<= 2; v |= libmaus2::fastx::mapChar(*(inp++));
									v <<= 2; v |= libmaus2::fastx::mapChar(*(inp++));
									v <<= 2; v |= libmaus2::fastx::mapChar(*(inp++));
									*(outp++) = v;
								}
								uint64_t const rest = n % 4;

								if ( rest )
								{
									uint8_t v = 0;
									for ( uint64_t i = 0; i < rest; ++i )
										v |= libmaus2::fastx::mapChar(*(inp++)) << (6-2*i);
									*(outp++) = v;
								}

								assert ( outp == reinterpret_cast<uint8_t *>(Bout.begin() + (n+3)/4) );

								++rcnt;
							}
							else if ( regncnt == n )
							{
								// block type
								finalout.put(::libmaus2::fastx::FastABPConstants::base_block_5 | lastblockflag);
								filepos++;

								if ( lastblockflag )
									filepos += libmaus2::util::UTF8::encodeUTF8(n,finalout);

								char const * inp = Bin.begin();
								while ( inp != Bin.begin() + (n/3)*3 )
								{
									uint8_t v = 0;
										v += libmaus2::fastx::mapChar(*(inp++));
									v *= 5; v += libmaus2::fastx::mapChar(*(inp++));
									v *= 5; v += libmaus2::fastx::mapChar(*(inp++));
									*(outp++) = v;
								}
								uint64_t const rest = n%3;
								if ( rest == 2 )
								{
									uint8_t v = 0;
										v += libmaus2::fastx::mapChar(*(inp++));
									v *= 5; v += libmaus2::fastx::mapChar(*(inp++));
									v *= 5;
									*(outp++) = v;
								}
								else if ( rest == 1 )
								{
									*(outp++) = libmaus2::fastx::mapChar(*(inp++)) * 5 * 5;
								}

								assert ( outp == reinterpret_cast<uint8_t *>(Bout.begin() + (n+2)/3) );

								++rncnt;
							}
							else
							{
								// block type
								finalout.put(::libmaus2::fastx::FastABPConstants::base_block_16 | lastblockflag);
								filepos++;

								if ( lastblockflag )
									filepos += libmaus2::util::UTF8::encodeUTF8(n,finalout);

								// check whether all symbols are valid
								for ( uint64_t i = 0; i < n; ++i )
									if ( (! deftab[static_cast<uint8_t>(Bin[i])]) && (Bin[i] != 'N') )
									{
										libmaus2::exception::LibMausException lme;
										lme.getStream() << "FastABPGenerator::fastAToFastaBP: Sequence " << sid << " contains unknown symbol " << Bin[i] << std::endl;
										lme.finish();
										throw lme;
									}


								char const * inp = Bin.begin();
								while ( inp != Bin.begin() + (n/2)*2 )
								{
									uint8_t  v = 0;
										 v |= bamencodetab[*(inp++)];
									v <<= 4; v |= bamencodetab[*(inp++)];
									*(outp++) = v;
								}
								uint64_t const rest = n % 2;

								if ( rest )
									*(outp++) = bamencodetab[*(inp++)] << 4;

								assert ( outp == reinterpret_cast<uint8_t *>(Bout.begin() + (n+1)/2) );

								++ncnt;
							}

							uint64_t const outbytes = outp-reinterpret_cast<uint8_t *>(Bout.begin());
							finalout.write(Bout.begin(),outbytes);
							filepos += outbytes;

							while ( filepos%8 != 0 )
							{
								finalout.put(0);
								filepos++;
							}

							// input crc
							uint32_t const crcin = libmaus2::hashing::Crc32::crc32_8bytes(Bin.begin(),n,0 /*prev*/);
							for ( unsigned int i = 0; i < sizeof(uint32_t); ++i )
							{
								finalout.put((crcin >> (24-i*8)) & 0xFF);
								filepos += 1;
							}
							// output crc
							uint32_t const crcout = libmaus2::hashing::Crc32::crc32_8bytes(Bout.begin(),outbytes,0 /*prev*/);
							for ( unsigned int i = 0; i < sizeof(uint32_t); ++i )
							{
								finalout.put((crcout >> (24-i*8)) & 0xFF);
								filepos += 1;
							}
						}
					}

					if ( verboseout )
						(*verboseout) << "[V] processed " << sid << " length " << seqlen << std::endl;

					uint64_t const seqmetapos = filepos;
					seqmetaposlist.push_back(seqmetapos);

					libmaus2::fastx::FastAInfo const fainfo(sid,seqlen);
					filepos += fainfo.serialise(finalout);

					// write block pointers
					for ( uint64_t i = 0; i < blockstarts.size(); ++i )
						filepos += libmaus2::util::NumberSerialisation::serialiseNumber(finalout,blockstarts[i]);

					totalseqlen += seqlen;
				}

				for ( unsigned int i = 0; i < sizeof(uint64_t); ++i )
				{
					finalout.put(0);
					filepos += 1;
				}

				uint64_t const metapos = filepos;
				// number of sequences
				filepos += libmaus2::util::NumberSerialisation::serialiseNumber(finalout,seqmetaposlist.size());
				for ( uint64_t i = 0; i < seqmetaposlist.size(); ++i )
					// sequence meta data pointers
					filepos += libmaus2::util::NumberSerialisation::serialiseNumber(finalout,seqmetaposlist[i]);

				// meta data pointer
				filepos += libmaus2::util::NumberSerialisation::serialiseNumber(finalout,metapos);

				if ( verboseout )
				{
					(*verboseout) << "[V] number of sequences        " << seqmetaposlist.size() << std::endl;
					(*verboseout) << "[V] number of A,C,G,T   blocks " << rcnt << std::endl;
					(*verboseout) << "[V] number of A,C,G,T,N blocks " << rncnt << std::endl;
					(*verboseout) << "[V] number of ambiguity blocks " << ncnt << std::endl;
					(*verboseout) << "[V] file size                  " << filepos << std::endl;
					(*verboseout) << "[V] total number of bases      " << totalseqlen << std::endl;
					(*verboseout) << "[V] bits per base              " << static_cast<double>(filepos*8)/totalseqlen << std::endl;
				}
			}
		};
	}
}
#endif
