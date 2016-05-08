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
#if ! defined(LIBMAUS2_GAMMA_GAMMAFLAGGEDPARTITIONENCODER_HPP)
#define LIBMAUS2_GAMMA_GAMMAFLAGGEDPARTITIONENCODER_HPP

#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/huffman/IndexEntry.hpp>
#include <libmaus2/huffman/HuffmanEncoderFile.hpp>
#include <libmaus2/huffman/RLEncoder.hpp>
#include <libmaus2/gamma/FlaggedInterval.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct GammaFlaggedPartitionEncoder
		{
			typedef GammaFlaggedPartitionEncoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef uint64_t gamma_data_type;
			typedef libmaus2::aio::SynchronousGenericOutput<gamma_data_type> stream_type;

			libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI;
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
			uint64_t total;

			// write one block to disk
			void flushInternal()
			{
				// number of elements in block
				ptrdiff_t const numintv = pc-pa;

				// if block is not empty
				if ( numintv )
				{
					// data bit offset
					uint64_t const offset = G.getOffset();

					// check intervals are non empty
					for ( FlaggedInterval * pp = pa; pp != pc; ++pp )
						assert( pp->to > pp->from );
					// check intervals are touching
					for ( FlaggedInterval * pp = pa+1; pp != pc; ++pp )
						assert( pp[0].from == pp[-1].to );

					// number of elements in block
					G.encodeSlow(numintv-1);

					// first interval low, store absolute
					G.encodeSlow(pa->from);
					// non empty interval
					assert ( pa->to-pa->from );

					// sum over interval lengths
					uint64_t vsum = 0;

					// encode intervals
					for ( FlaggedInterval * pp = pa; pp != pc; ++pp )
					{
						// interval length
						uint64_t const intvsize = pp[0].to-pp[0].from;
						// encode
						G.encodeSlow( intvsize - 1 );
						// update count
						vsum += intvsize;
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

					// push index entry
					Vindex.push_back(libmaus2::huffman::IndexEntry(offset,numintv,vsum));
					// update total interval size
					total += vsum;

					pc = pa;
				}
			}

			GammaFlaggedPartitionEncoder(std::string const & fn, uint64_t const bufsize = 8*1024, uint64_t const indexblocksize = 1024)
			: POSI(new libmaus2::aio::OutputStreamInstance(fn)), OSI(*POSI), PSGO(new stream_type(OSI,bufsize)), SGO(*PSGO),
			  PG(new libmaus2::gamma::GammaEncoder<stream_type>(SGO)), G(*PG),
			  B(indexblocksize), pa(B.begin()), pc(pa), pe(B.end()),
			  total(0)
			{
				assert ( bufsize );
				assert ( indexblocksize );
			}

			~GammaFlaggedPartitionEncoder()
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
					libmaus2::huffman::HuffmanEncoderFileStd::unique_ptr_type PHEF(new libmaus2::huffman::HuffmanEncoderFileStd(OSI));
					libmaus2::huffman::IndexWriter::writeIndex(*PHEF,Vindex,indexpos,total);
					PHEF.reset();

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
