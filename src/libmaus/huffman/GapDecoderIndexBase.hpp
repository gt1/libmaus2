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

#if ! defined(GAPDECODERINDEXBASE_HPP)
#define GAPDECODERINDEXBASE_HPP

#include <fstream>
#include <libmaus/bitio/BitIOInput.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bitio/readElias.hpp>
#include <libmaus/huffman/CanonicalEncoder.hpp>
#include <libmaus/util/IntervalTree.hpp>
#include <libmaus/huffman/RLDecoder.hpp>

namespace libmaus
{
	namespace huffman
	{
		struct GapDecoderIndexBase : ::libmaus::huffman::IndexLoader
		{
			typedef GapDecoderIndexBase this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef ::libmaus::huffman::BitInputBuffer4 sbis_type;

			std::vector < std::string > const filenames;
			::libmaus::autoarray::AutoArray < ::libmaus::autoarray::AutoArray< IndexEntry > > const index;
			::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const symaccu;
			::libmaus::util::GenericIntervalTree const symtree;
			::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const segaccu;
			::libmaus::util::GenericIntervalTree const segtree;
			::libmaus::autoarray::AutoArray < ::libmaus::autoarray::AutoArray < uint64_t > > const blocksizes;
			::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > const blockintervals;
			::libmaus::util::GenericIntervalTree const blockintervaltree;
			uint64_t const n;

			::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > computeSymAccu()
			{
				uint64_t numint = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
					numint += index[i].size();
				::libmaus::autoarray::AutoArray<uint64_t> preaccu(numint+1);
				uint64_t k = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
					for ( uint64_t j = 0; j < index[i].size(); ++j )
						preaccu[k++] = index[i][j].vcnt;
						
				preaccu.prefixSums();
				::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > accu(numint);
				for ( uint64_t i = 1; i < preaccu.size(); ++i )
					accu[i-1] = std::pair<uint64_t,uint64_t>(
						std::pair<uint64_t,uint64_t>(preaccu[i-1],preaccu[i])
						);
				return accu;
			}

			::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > computeSegmentAccu()
			{
				uint64_t const numint = index.size();
				::libmaus::autoarray::AutoArray<uint64_t> preaccu(numint+1);
				uint64_t k = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
					preaccu[k++] = index[i].size();
				preaccu.prefixSums();
				::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > accu(numint);
				for ( uint64_t i = 1; i < preaccu.size(); ++i )
					accu[i-1] = std::pair<uint64_t,uint64_t>(
						std::pair<uint64_t,uint64_t>(preaccu[i-1],preaccu[i])
						);
				return accu;
			}

			// get length of file in symbols
			static uint64_t getLength(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				assert ( istr.is_open() );
				::libmaus::bitio::StreamBitInputStream SBIS(istr);	
				SBIS.readBit(); // need escape
				return ::libmaus::bitio::readElias2(SBIS);
			}
			
			// get length of vector of files in symbols
			static uint64_t getLength(std::vector<std::string> const & filenames)
			{
				uint64_t s = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					s += getLength(filenames[i]);
				return s;
			}

			::libmaus::autoarray::AutoArray < ::libmaus::autoarray::AutoArray < uint64_t > > getBlockSizes() const
			{
				::libmaus::autoarray::AutoArray < ::libmaus::autoarray::AutoArray < uint64_t > > blocksizes(index.size());
				
				for ( uint64_t fileptr = 0; fileptr < index.size(); ++fileptr )
				{
					blocksizes[fileptr] = ::libmaus::autoarray::AutoArray<uint64_t>(index[fileptr].size(),false);
					std::ifstream istr(filenames[fileptr].c_str(),std::ios::binary);
					assert ( istr.is_open() );
					
					for ( uint64_t blockptr = 0; blockptr < index[fileptr].size(); ++blockptr )
					{
						istr.clear();
						istr.seekg(index[fileptr][blockptr].pos,std::ios::beg);
						sbis_type::raw_input_ptr_type ript(new sbis_type::raw_input_type(istr));
						sbis_type SBIS(ript,1024);
						uint64_t const blocksize = ::libmaus::bitio::readElias2(SBIS);
						blocksizes[fileptr][blockptr] = blocksize;
					}
				}
				
				
				return blocksizes;
			}
			
			::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > computeBlockIntervals() const
			{
				uint64_t numblocks = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
					numblocks += index[i].size();
				::libmaus::autoarray::AutoArray < uint64_t > lblocksizes = ::libmaus::autoarray::AutoArray < uint64_t >(numblocks+1);
				uint64_t * outptr = lblocksizes.begin();
				for ( uint64_t i = 0; i < blocksizes.size(); ++i )
					for ( uint64_t j = 0; j < blocksizes[i].size(); ++j )
						*(outptr++) = blocksizes[i][j];
				lblocksizes.prefixSums();
				::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > blockintervals(numblocks);
				for ( uint64_t i = 1; i < lblocksizes.size(); ++i )
					blockintervals[i-1] = std::pair<uint64_t,uint64_t>(lblocksizes[i-1],lblocksizes[i]);
				return blockintervals;
			}
			
			static std::vector<std::string> filterFilenamesByLength(std::vector<std::string> const & rfilenames)
			{
				std::vector<std::string> filtered;
				
				for ( uint64_t i = 0; i < rfilenames.size(); ++i )
					if ( getLength(rfilenames[i]) )
						filtered.push_back(rfilenames[i]);
						
				return filtered;
			}

			GapDecoderIndexBase(std::vector<std::string> const & rfilenames)
			: 
			  filenames(filterFilenamesByLength(rfilenames)),
			  index(loadIndex(filenames)),
			  symaccu(computeSymAccu()),
			  symtree(symaccu),
			  segaccu(computeSegmentAccu()),
			  segtree(segaccu),
			  blocksizes(getBlockSizes()),
			  blockintervals(computeBlockIntervals()),
			  blockintervaltree(blockintervals),
			  n ( getLength(filenames) )
			{}
		};
	}
}
#endif
