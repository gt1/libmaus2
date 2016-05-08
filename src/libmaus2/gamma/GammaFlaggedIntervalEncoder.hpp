/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_GAMMA_GAMMAFLAGGEDINTERVALENCODER_HPP)
#define LIBMAUS2_GAMMA_GAMMAFLAGGEDINTERVALENCODER_HPP

#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/huffman/IndexEntry.hpp>
#include <libmaus2/huffman/HuffmanEncoderFile.hpp>
#include <libmaus2/huffman/RLEncoder.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct GammaFlaggedIntervalEncoder
		{
			typedef GammaFlaggedIntervalEncoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef uint64_t gamma_data_type;
			typedef libmaus2::aio::SynchronousGenericOutput<gamma_data_type> stream_type;

			std::string const fn;
			std::string const metafn;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type PMETAOSI;
			std::ostream & OSI;
			stream_type::unique_ptr_type PSGO;
			stream_type & SGO;

			libmaus2::gamma::GammaEncoder<stream_type>::unique_ptr_type PG;
			libmaus2::gamma::GammaEncoder<stream_type> & G;

			libmaus2::autoarray::AutoArray< FlaggedInterval > B;
			FlaggedInterval * const pa;
			FlaggedInterval * pc;
			FlaggedInterval * const pe;

			std::vector<libmaus2::huffman::IndexEntry> Vindex;

			void flushInternal()
			{
				// number of elements in block
				ptrdiff_t const numintv = pc-pa;

				if ( numintv )
				{
					uint64_t const offset = G.getOffset();

					// number of elements in block
					G.encodeSlow(numintv-1);

					// first interval low, store absolute
					G.encodeSlow(pa->from);
					// encode interval length
					G.encodeSlow(pa->to - pa->from);

					// sum over interval lengths
					uint64_t vsum = pa[0].to-pa[0].from;
					// encode other intervals
					for ( FlaggedInterval * pp = pa+1; pp != pc; ++pp )
					{
						// starts after prev interval
						assert ( pp[0].from >= pp[-1].to );
						// offset from previous interval start
						G.encodeSlow( pp[0].from - pp[-1].to );
						// length of interval
						G.encodeSlow( pp[0].to - pp[0].from );
						// update count
						vsum += pp[0].to-pp[0].from;
					}

					FlaggedInterval * pp = pa;
					while ( pp != pc )
					{
						FlaggedInterval * pup = pp;
						while ( pup != pc && pup->type == pp->type )
							++pup;

						G.encodeWord(static_cast<int>(pp->type),2);
						G.encodeSlow((pup-pp)-1);

						pp = pup;
					}

					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETAOSI,pa[0].from);
					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETAOSI,pc[-1].to);
					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETAOSI,offset);

					// reset buffer
					pc = pa;
				}
			}

			GammaFlaggedIntervalEncoder(
				std::string const & rfn,
				uint64_t const bufsize = 8*1024,
				uint64_t const indexblocksize = 1024
			)
			: fn(rfn),
			  metafn(fn+".meta"),
			  POSI(new libmaus2::aio::OutputStreamInstance(fn)),
			  PMETAOSI(new libmaus2::aio::OutputStreamInstance(metafn)),
			  OSI(*POSI),
			  PSGO(new stream_type(OSI,bufsize)),
			  SGO(*PSGO),
			  PG(new libmaus2::gamma::GammaEncoder<stream_type>(SGO)),
			  G(*PG),
			  B(indexblocksize), pa(B.begin()), pc(pa), pe(B.end())
			{
				assert ( bufsize );
				assert ( indexblocksize );
			}

			~GammaFlaggedIntervalEncoder()
			{
				flush();
			}

			void flush()
			{
				if ( PG )
				{
					// flush
					flushInternal();
					G.flush();
					PG.reset();
					SGO.flush();
					PSGO.reset();
					OSI.flush();

					// write index
					uint64_t const indexpos = OSI.tellp();

					PMETAOSI->flush();
					PMETAOSI.reset();

					libmaus2::aio::InputStreamInstance::unique_ptr_type METAISI(new libmaus2::aio::InputStreamInstance(metafn));
					uint64_t numblocks = 0;
					while ( METAISI->peek() != std::istream::traits_type::eof() )
					{
						uint64_t const low = libmaus2::util::NumberSerialisation::deserialiseNumber(*METAISI);
						uint64_t const high = libmaus2::util::NumberSerialisation::deserialiseNumber(*METAISI);
						uint64_t const offset = libmaus2::util::NumberSerialisation::deserialiseNumber(*METAISI);

						libmaus2::util::NumberSerialisation::serialiseNumber(OSI,low);
						libmaus2::util::NumberSerialisation::serialiseNumber(OSI,high);
						libmaus2::util::NumberSerialisation::serialiseNumber(OSI,offset);

						numblocks += 1;
					}

					METAISI.reset();

					libmaus2::aio::FileRemoval::removeFile(metafn);

					libmaus2::util::NumberSerialisation::serialiseNumber(OSI,numblocks);
					libmaus2::util::NumberSerialisation::serialiseNumber(OSI,indexpos);

					OSI.flush();
					POSI.reset();
				}
			}

			void put(FlaggedInterval const & P)
			{
				*(pc++) = P;
				if ( pc == pe )
					flushInternal();
			}
		};
	}
}
#endif
