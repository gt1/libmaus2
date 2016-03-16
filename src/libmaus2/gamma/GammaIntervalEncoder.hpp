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
#if ! defined(LIBMAUS2_GAMMA_GAMMAINTERVALENCODER_HPP)
#define LIBMAUS2_GAMMA_GAMMAINTERVALENCODER_HPP

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
		struct GammaIntervalEncoder
		{
			typedef GammaIntervalEncoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef uint64_t gamma_data_type;
			typedef libmaus2::aio::SynchronousGenericOutput<gamma_data_type> stream_type;

			libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI;
			std::ostream & OSI;
			typename stream_type::unique_ptr_type PSGO;
			stream_type & SGO;

			libmaus2::gamma::GammaEncoder<stream_type>::unique_ptr_type PG;
			libmaus2::gamma::GammaEncoder<stream_type> & G;

			libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > B;
			std::pair<uint64_t,uint64_t> * const pa;
			std::pair<uint64_t,uint64_t> * pc;
			std::pair<uint64_t,uint64_t> * const pe;

			std::vector<libmaus2::huffman::IndexEntry> Vindex;
			uint64_t total;

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
					G.encodeSlow(pa->first);
					// non empty interval
					assert ( pa->second-pa->first );
					// encode interval length
					G.encodeSlow((pa->second - pa->first)-1);

					// sum over interval lengths
					uint64_t vsum = pa[0].second-pa[0].first;
					// encode other intervals
					for ( std::pair<uint64_t,uint64_t> * pp = pa+1; pp != pc; ++pp )
					{
						// non empty
						assert ( pp[0].second-pp[0].first );
						// starts after prev interval
						assert ( pp[0].first >= pp[-1].second );
						// offset from previous interval start
						G.encodeSlow( ( pp[0].first - pp[-1].first ) - 1 );
						// length of interval (assumed to be non empty)
						G.encodeSlow( ( pp[0].second - pp[0].first ) - 1 );
						// update count
						vsum += pp[0].second-pp[0].first;
					}

					// push index entry
					Vindex.push_back(libmaus2::huffman::IndexEntry(offset,numintv,vsum));
					// update total interval size
					total += vsum;

					// reset buffer
					pc = pa;
				}
			}

			GammaIntervalEncoder(std::string const & fn, uint64_t const bufsize = 8*1024, uint64_t const indexblocksize = 1024)
			: POSI(new libmaus2::aio::OutputStreamInstance(fn)), OSI(*POSI), PSGO(new stream_type(OSI,bufsize)), SGO(*PSGO),
			  PG(new libmaus2::gamma::GammaEncoder<stream_type>(SGO)), G(*PG),
			  B(indexblocksize), pa(B.begin()), pc(pa), pe(B.end()),
			  total(0)
			{
				assert ( bufsize );
				assert ( indexblocksize );
			}

			~GammaIntervalEncoder()
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
					libmaus2::huffman::HuffmanEncoderFileStd::unique_ptr_type HEF(new libmaus2::huffman::HuffmanEncoderFileStd(OSI));
					libmaus2::huffman::IndexWriter::writeIndex(*HEF,Vindex,indexpos,total);
					HEF.reset();

					OSI.flush();
					POSI.reset();
				}
			}

			void put(std::pair<uint64_t,uint64_t> const & P)
			{
				*(pc++) = P;
				if ( pc == pe )
					flushInternal();
			}
		};
	}
}
#endif
