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
			typedef SparseGammaGapFileIndexMultiDecoder this_type;
			typedef SparseGammaGapFileIndexDecoder::value_type value_type;
		
			std::vector<std::string> filenames;
			libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > H;
			libmaus::util::IntervalTree::unique_ptr_type I;
			libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > BC;
			libmaus::util::IntervalTree::unique_ptr_type B;
			uint64_t numentries;

			typedef libmaus::util::ConstIterator<this_type,value_type> const_iterator;

			const_iterator begin() const
			{
				return const_iterator(this,0);
			}

			const_iterator end() const
			{
				return const_iterator(this,numentries);
			}
			
			std::vector<std::string> filterFilenames(std::vector<std::string> const & filenames)
			{
				std::vector<std::string> outfilenames;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					if ( ! SparseGammaGapFileIndexDecoder(filenames[i]).isEmpty() )
						outfilenames.push_back(filenames[i]);
				return outfilenames;
			}
			
			SparseGammaGapFileIndexMultiDecoder(std::vector<std::string> const & rfilenames) 
			: filenames(filterFilenames(rfilenames)), H(filenames.size()), BC(filenames.size())
			{
				if ( BC.size() )
					BC[0].first = 0;
					
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					SparseGammaGapFileIndexDecoder dec(filenames[i]);
					H[i].first = dec.getMinKey();
					
					if ( i > 0 )
						BC[i].first = BC[i-1].second;

					BC[i].second = BC[i].first + dec.numentries;
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
				
				if ( H.size() )
				{
					libmaus::util::IntervalTree::unique_ptr_type tI(new libmaus::util::IntervalTree(H,0,H.size()));
					I = UNIQUE_PTR_MOVE(tI);
				}

				if ( BC.size() )
				{
					libmaus::util::IntervalTree::unique_ptr_type tB(new libmaus::util::IntervalTree(BC,0,BC.size()));
					B = UNIQUE_PTR_MOVE(tB);
				}
				
				numentries = BC.size() ? BC[BC.size()-1].second : 0;
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
			
			SparseGammaGapFileIndexDecoder::value_type get(uint64_t const i) const
			{
				if ( i >= numentries )
				{
					libmaus::exception::LibMausException ex;
					ex.getStream() << "SparseGammaGapFileIndexMultiDecoder::get(): index out of range" << std::endl;
					ex.finish();
					throw ex;
				}
				
				uint64_t const fileptr = B->find(i);
				uint64_t const blockptr = i - BC[fileptr].first;
				
				return SparseGammaGapFileIndexDecoder(filenames[fileptr]).get(blockptr);
			}
			
			bool hasPrevKey(uint64_t const ikey) const
			{
				if ( ! numentries )
					return false;
				else
				{
					uint64_t const minkey = get(0).ikey;
					
					if ( ikey <= minkey )
						return false;
					else
						return true;
				}
			}
			
			bool isEmpty() const
			{
				return numentries == 0;
			}
			
			bool hasMaxKey() const
			{
				return numentries;
			}
			
			uint64_t getMaxKey() const
			{
				assert ( hasMaxKey() );
				return SparseGammaGapFileIndexDecoder(filenames[filenames.size()-1]).getMaxKey();
			}
			
			uint64_t getMinKey() const
			{
				assert ( !isEmpty() );
				return get(0).ikey;
			}
						
			bool hasKeysLargerEqual(uint64_t const ikey) const
			{
				return hasMaxKey() && (ikey <= getMaxKey());
			}
			
			struct SparseGammaGapFileIndexMultiDecoderBlockCountAccessor
			{
				SparseGammaGapFileIndexMultiDecoder const * owner;
				
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessor(SparseGammaGapFileIndexMultiDecoder const * rowner) : owner(rowner) {}
				
				uint64_t get(uint64_t const ikey) const
				{
					SparseGammaGapFileIndexMultiDecoder::const_iterator itc =
						std::lower_bound(
							owner->begin(),
							owner->end(),
							ikey	
					);
					
					return itc - owner->begin();
				}			
			};
			
			SparseGammaGapFileIndexMultiDecoderBlockCountAccessor getBlockCountAccessor() const
			{
				return SparseGammaGapFileIndexMultiDecoderBlockCountAccessor(this);
			}
			
			struct SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined
			{
				typedef SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined this_type;
				
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessor accessora;
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessor accessorb;
				uint64_t range;

				typedef libmaus::util::ConstIterator<this_type,value_type> const_iterator;

				const_iterator begin() const
				{
					return const_iterator(this,0);
				}

				const_iterator end() const
				{
					return const_iterator(this,range);
				}
				
				uint64_t totalBlocks() const
				{
					return
						accessora.owner->numentries + 
						accessorb.owner->numentries;
				}
				
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined(
					SparseGammaGapFileIndexMultiDecoderBlockCountAccessor raccessora,
					SparseGammaGapFileIndexMultiDecoderBlockCountAccessor raccessorb,
					uint64_t const rrange
				) : accessora(raccessora), accessorb(raccessorb), range(rrange)
				{
				
				}

				SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined(
					SparseGammaGapFileIndexMultiDecoder const & rA,
					SparseGammaGapFileIndexMultiDecoder const & rB,
					uint64_t const rrange
				) : accessora(rA.getBlockCountAccessor()), accessorb(rB.getBlockCountAccessor()), range(rrange)
				{
				
				}
								
				uint64_t get(uint64_t const ikey) const
				{
					return accessora.get(ikey) + accessorb.get(ikey);
				}
			};
			
			static SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined getCombinedAccessor(
				SparseGammaGapFileIndexMultiDecoder const & A,
				SparseGammaGapFileIndexMultiDecoder const & B,
				uint64_t const range)
			{
				return SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined(A,B,range);
			}
			
			static std::vector<uint64_t> getSplitKeys(
				SparseGammaGapFileIndexMultiDecoder const & A,
				SparseGammaGapFileIndexMultiDecoder const & B,
				uint64_t const range,
				uint64_t const numkeys
			)
			{
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined CA = 
					getCombinedAccessor(A,B,range);
										
				uint64_t const totalblocks = CA.totalBlocks();
				uint64_t const blocksperkey = (totalblocks + numkeys-1)/numkeys;
				std::vector<uint64_t> keys;
				
				for ( uint64_t i = 0; i < numkeys; ++i )
				{
					uint64_t const tblock = std::min(i * blocksperkey, totalblocks);
					uint64_t const ikey = std::lower_bound(CA.begin(),CA.end(),tblock) - CA.begin();
					if ( (! keys.size()) || (ikey != keys.back()) )
						keys.push_back(ikey);
				}
				
				return keys;
			}

			static std::vector<uint64_t> getSplitKeys(
				std::vector<std::string> const & fna,
				std::vector<std::string> const & fnb,
				uint64_t const range,
				uint64_t const numkeys
			)
			{
				SparseGammaGapFileIndexMultiDecoder A(fna);
				SparseGammaGapFileIndexMultiDecoder B(fnb);
				return getSplitKeys(A,B,range,numkeys);
			}

			static std::vector<uint64_t> getSplitKeys(
				std::vector<std::string> const & fna,
				std::vector<std::string> const & fnb,
				uint64_t const numkeys
			)
			{
				bool const aempty = this_type(fna).isEmpty();
				bool const bempty = this_type(fnb).isEmpty();
				
				uint64_t const maxa = aempty ? 0 : this_type(fna).getMaxKey();
				uint64_t const maxb = bempty ? 0 : this_type(fnb).getMaxKey();
				uint64_t const maxv = std::max(maxa,maxb);
				return getSplitKeys(fna,fnb,maxv,numkeys);
			}
		};	
	}
}
#endif
