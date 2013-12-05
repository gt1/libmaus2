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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPFILEINDEXMULTIDECODER_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPFILEINDEXMULTIDECODER_HPP

#include <libmaus/gamma/SparseGammaGapFileIndexDecoder.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct SparseGammaGapFileIndexMultiDecoder
		{
			std::vector<std::string> filenames;
			libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > H;
			libmaus::util::IntervalTree::unique_ptr_type I;
			
			std::vector<std::string> filterFilenames(std::vector<std::string> const & filenames)
			{
				std::vector<std::string> outfilenames;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					if ( ! SparseGammaGapFileIndexDecoder(filenames[i]).isEmpty() )
						outfilenames.push_back(filenames[i]);
				return outfilenames;
			}
			
			SparseGammaGapFileIndexMultiDecoder(std::vector<std::string> const & rfilenames) 
			: filenames(filterFilenames(rfilenames)), H(filenames.size())
			{
				#if 0
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					std::cerr << "[" << i << "] : " << SparseGammaGapFileIndexDecoder(filenames[i]).numentries << std::endl;
				}
				#endif

				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					H[i].first = SparseGammaGapFileIndexDecoder(filenames[i]).getMinKey();
				}
				for ( uint64_t i = 0; i+1 < filenames.size(); ++i )
				{
					H[i].second = H[i+1].first;
				}
				if ( filenames.size() )
				{
					H[0].first           = 0;
					H[H.size()-1].second = std::numeric_limits<uint64_t>::max();
				}
				
				libmaus::util::IntervalTree::unique_ptr_type tI(new libmaus::util::IntervalTree(H,0,H.size()));
				I = UNIQUE_PTR_MOVE(tI);
			}
			
			std::pair<uint64_t,uint64_t> getBlockIndex(uint64_t const ikey)
			{
				if ( ! H.size() )
				{
					return std::pair<uint64_t,uint64_t>(0,0);
				}
				else if ( ikey >= H[H.size()-1].second )
				{
					return std::pair<uint64_t,uint64_t>(H.size(),0);
				}
				else
				{
					uint64_t const findex = I->find(ikey);
					assert ( ikey >= H[findex].first );
					assert ( ikey  < H[findex].second );
					SparseGammaGapFileIndexDecoder dec(filenames[findex]);
					return std::pair<uint64_t,uint64_t>(findex,dec.getBlockIndex(ikey));
				}
			}
		};
	}
}
#endif
