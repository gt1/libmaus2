/**
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
**/

#if ! defined(RLDECODERINDEXBASE_HPP)
#define RLDECODERINDEXBASE_HPP

#include <fstream>
#include <libmaus/bitio/BitIOInput.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bitio/readElias.hpp>
#include <libmaus/huffman/CanonicalEncoder.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/util/IntervalTree.hpp>
#include <libmaus/huffman/IndexLoader.hpp>

namespace libmaus
{
	namespace huffman
	{
		struct RLDecoderIndexBase : public IndexLoader
		{
			typedef RLDecoderIndexBase this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			std::vector<std::string> filenames;
			// (blockpos,kcnt,numsyms in block)
			::libmaus::autoarray::AutoArray < libmaus::autoarray::AutoArray< IndexEntry > > const index;
			// total symbols in file
			uint64_t const n;
			
			::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > const symaccu;
			::libmaus::util::GenericIntervalTree const symtree;
			::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > const segaccu;
			::libmaus::util::GenericIntervalTree const segtree;

			void printIndex() const
			{
				for ( uint64_t i = 0; i < index.size(); ++i )
				{
					std::cerr << "file " << i << " " << filenames[i] << std::endl;
					for ( uint64_t j = 0; j < index[i].size(); ++j )
						std::cerr << "(" << index[i][j].pos << "," << index[i][j].kcnt << "," << index[i][j].vcnt << ")" << std::endl;
				}
			}

			::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > computeSymAccu() const
			{
				uint64_t numint = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
					numint += index[i].size();
				::libmaus::autoarray::AutoArray<uint64_t> preaccu(numint+1);
				uint64_t outptr = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
					for ( uint64_t j = 0; j < index[i].size(); ++j )
						preaccu[outptr++] = index[i][j].vcnt;
				preaccu.prefixSums();
				::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > symaccu(numint);
				for ( uint64_t i = 1; i < preaccu.size(); ++i )
					symaccu[i-1] = std::pair<uint64_t,uint64_t>(preaccu[i-1],preaccu[i]);
	
				#if 0
				std::cerr << "presymaccu:" << std::endl;
				for ( uint64_t i = 0; i < preaccu.size(); ++i )
					std::cerr << preaccu[i] << std::endl;

				std::cerr << "symaccu:" << std::endl;
				for ( uint64_t i = 0; i < symaccu.size(); ++i )
					std::cerr << "[" << i << "]=[" << symaccu[i].first << "," << symaccu[i].second << ")" << std::endl;
				#endif
					
				return symaccu;
			}
			
			::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > computeSegAccu() const
			{
				::libmaus::autoarray::AutoArray<uint64_t> preaccu(index.size()+1);
				for ( uint64_t i = 0; i < index.size(); ++i )
					preaccu[i] = index[i].size();
				preaccu.prefixSums();
				::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > accu(index.size());
				for ( uint64_t i = 1; i < preaccu.size(); ++i )
					accu[i-1] = std::pair<uint64_t,uint64_t>(preaccu[i-1],preaccu[i]);
				return accu;
			}
		
			static uint64_t getLength(std::vector<std::string> const & filenames)
			{
				uint64_t n = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					n += getLength(filenames[i]);
				return n;
			}
			static uint64_t getLength(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				if ( ! istr.is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "RLDecoder::getLength(): Failed to open file " << filename << std::endl;
					se.finish();
					throw se;
				}
				::libmaus::bitio::StreamBitInputStream SBIS(istr);	
				return ::libmaus::bitio::readElias2(SBIS);
			}

			RLDecoderIndexBase(std::vector<std::string> const & rfilenames)
			: 
			  filenames(rfilenames),
			  index(loadIndex(filenames)),
			  n( getLength(filenames) ),
			  symaccu(computeSymAccu()),
			  symtree(symaccu),
			  segaccu(computeSegAccu()),
			  segtree(segaccu)
			{
			}
		};
	}
}
#endif
